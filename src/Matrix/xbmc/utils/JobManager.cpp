/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JobManager.h"

#include "threads/SingleLock.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

bool CJob::ShouldCancel(unsigned int progress, unsigned int total) const
{
  if (m_callback)
    return m_callback->OnJobProgress(progress, total, this);
  return false;
}

CJobWorker::CJobWorker(CJobManager *manager) : CThread("JobWorker")
{
  m_jobManager = manager;
  Create(true); // start work immediately, and kill ourselves when we're done
}

CJobWorker::~CJobWorker()
{
  // while we should already be removed from the job manager, if an exception
  // occurs during processing that we haven't caught, we may skip over that step.
  // Thus, before we go out of scope, ensure the job manager knows we're gone.
  m_jobManager->RemoveWorker(this);
  if(!IsAutoDelete())
    StopThread();
}

void CJobWorker::Process()
{
  SetPriority( GetMinPriority() );
  while (true)
  {
    // request an item from our manager (this call is blocking)
    CJob *job = m_jobManager->GetNextJob(this);
    if (!job)
      break;

    bool success = false;
    try
    {
      success = job->DoWork();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s error processing job %s", __FUNCTION__, job->GetType());
    }
    m_jobManager->OnJobComplete(success, job);
  }
}

void CJobQueue::CJobPointer::CancelJob()
{
  CJobManager::GetInstance().CancelJob(m_id);
  m_id = 0;
}

CJobQueue::CJobQueue(bool lifo, unsigned int jobsAtOnce, CJob::PRIORITY priority)
: m_jobsAtOnce(jobsAtOnce), m_priority(priority), m_lifo(lifo)
{
}

CJobQueue::~CJobQueue()
{
  CancelJobs();
}

void CJobQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lock(m_section);
  // check if this job is in our processing list
  Processing::iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
    m_processing.erase(i);
  // request a new job be queued
  QueueNextJob();
}

void CJobQueue::CancelJob(const CJob *job)
{
  CSingleLock lock(m_section);
  Processing::iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
  {
    i->CancelJob();
    m_processing.erase(i);
    return;
  }
  Queue::iterator j = find(m_jobQueue.begin(), m_jobQueue.end(), job);
  if (j != m_jobQueue.end())
  {
    j->FreeJob();
    m_jobQueue.erase(j);
  }
}

bool CJobQueue::AddJob(CJob *job)
{
  CSingleLock lock(m_section);
  // check if we have this job already.  If so, we're done.
  if (find(m_jobQueue.begin(), m_jobQueue.end(), job) != m_jobQueue.end() ||
      find(m_processing.begin(), m_processing.end(), job) != m_processing.end())
  {
    delete job;
    return false;
  }

  if (m_lifo)
    m_jobQueue.push_back(CJobPointer(job));
  else
    m_jobQueue.push_front(CJobPointer(job));
  QueueNextJob();

  return true;
}

void CJobQueue::QueueNextJob()
{
  CSingleLock lock(m_section);
  if (m_jobQueue.size() && m_processing.size() < m_jobsAtOnce)
  {
    CJobPointer &job = m_jobQueue.back();
    job.m_id = CJobManager::GetInstance().AddJob(job.m_job, this, m_priority);
    m_processing.push_back(job);
    m_jobQueue.pop_back();
  }
}

void CJobQueue::CancelJobs()
{
  CSingleLock lock(m_section);
  for_each(m_processing.begin(), m_processing.end(), [](CJobPointer& jp) { jp.CancelJob(); });
  for_each(m_jobQueue.begin(), m_jobQueue.end(), [](CJobPointer& jp) { jp.FreeJob(); });
  m_jobQueue.clear();
  m_processing.clear();
}

bool CJobQueue::IsProcessing() const
{
  return CJobManager::GetInstance().m_running && (!m_processing.empty() || !m_jobQueue.empty());
}

bool CJobQueue::QueueEmpty() const
{
  CSingleLock lock(m_section);
  return m_jobQueue.empty();
}

CJobManager &CJobManager::GetInstance()
{
  static CJobManager sJobManager;
  return sJobManager;
}

CJobManager::CJobManager()
{
  m_jobCounter = 0;
  m_running = true;
  m_pauseJobs = false;
}

void CJobManager::Restart()
{
  CSingleLock lock(m_section);

  if (m_running)
    throw std::logic_error("CJobManager already running");
  m_running = true;
}

void CJobManager::CancelJobs()
{
  CSingleLock lock(m_section);
  m_running = false;

  // clear any pending jobs
  for (unsigned int priority = CJob::PRIORITY_LOW_PAUSABLE; priority <= CJob::PRIORITY_DEDICATED; ++priority)
  {
    for_each(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), [](CWorkItem& wi) { wi.FreeJob(); });
    m_jobQueue[priority].clear();
  }

  // cancel any callbacks on jobs still processing
  for_each(m_processing.begin(), m_processing.end(), [](CWorkItem& wi) { wi.Cancel(); });

  // tell our workers to finish
  while (m_workers.size())
  {
    lock.Leave();
    m_jobEvent.Set();
    std::this_thread::yield(); // yield after setting the event to give the workers some time to die
    lock.Enter();
  }
}

unsigned int CJobManager::AddJob(CJob *job, IJobCallback *callback, CJob::PRIORITY priority)
{
  CSingleLock lock(m_section);

  if (!m_running)
    return 0;

  // increment the job counter, ensuring 0 (invalid job) is never hit
  m_jobCounter++;
  if (m_jobCounter == 0)
    m_jobCounter++;

  // create a work item for this job
  CWorkItem work(job, m_jobCounter, priority, callback);
  m_jobQueue[priority].push_back(work);

  StartWorkers(priority);
  return work.m_id;
}

void CJobManager::CancelJob(unsigned int jobID)
{
  CSingleLock lock(m_section);

  // check whether we have this job in the queue
  for (unsigned int priority = CJob::PRIORITY_LOW_PAUSABLE; priority <= CJob::PRIORITY_DEDICATED; ++priority)
  {
    JobQueue::iterator i = find(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), jobID);
    if (i != m_jobQueue[priority].end())
    {
      delete i->m_job;
      m_jobQueue[priority].erase(i);
      return;
    }
  }
  // or if we're processing it
  Processing::iterator it = find(m_processing.begin(), m_processing.end(), jobID);
  if (it != m_processing.end())
    it->m_callback = NULL; // job is in progress, so only thing to do is to remove callback
}

void CJobManager::StartWorkers(CJob::PRIORITY priority)
{
  CSingleLock lock(m_section);

  // check how many free threads we have
  if (m_processing.size() >= GetMaxWorkers(priority))
    return;

  // do we have any sleeping threads?
  if (m_processing.size() < m_workers.size())
  {
    m_jobEvent.Set();
    return;
  }

  // everyone is busy - we need more workers
  m_workers.push_back(new CJobWorker(this));
}

CJob *CJobManager::PopJob()
{
  CSingleLock lock(m_section);
  for (int priority = CJob::PRIORITY_DEDICATED; priority >= CJob::PRIORITY_LOW_PAUSABLE; --priority)
  {
    // Check whether we're pausing pausable jobs
    if (priority == CJob::PRIORITY_LOW_PAUSABLE && m_pauseJobs)
      continue;

    if (m_jobQueue[priority].size() && m_processing.size() < GetMaxWorkers(CJob::PRIORITY(priority)))
    {
      // pop the job off the queue
      CWorkItem job = m_jobQueue[priority].front();
      m_jobQueue[priority].pop_front();

      // add to the processing vector
      m_processing.push_back(job);
      job.m_job->m_callback = this;
      return job.m_job;
    }
  }
  return NULL;
}

void CJobManager::PauseJobs()
{
  CSingleLock lock(m_section);
  m_pauseJobs = true;
}

void CJobManager::UnPauseJobs()
{
  CSingleLock lock(m_section);
  m_pauseJobs = false;
}

bool CJobManager::IsProcessing(const CJob::PRIORITY &priority) const
{
  CSingleLock lock(m_section);

  if (m_pauseJobs)
    return false;

  for(Processing::const_iterator it = m_processing.begin(); it < m_processing.end(); ++it)
  {
    if (priority == it->m_priority)
      return true;
  }
  return false;
}

int CJobManager::IsProcessing(const std::string &type) const
{
  int jobsMatched = 0;
  CSingleLock lock(m_section);

  if (m_pauseJobs)
    return 0;

  for(Processing::const_iterator it = m_processing.begin(); it < m_processing.end(); ++it)
  {
    if (type == std::string(it->m_job->GetType()))
      jobsMatched++;
  }
  return jobsMatched;
}

CJob *CJobManager::GetNextJob(const CJobWorker *worker)
{
  CSingleLock lock(m_section);
  while (m_running)
  {
    // grab a job off the queue if we have one
    CJob *job = PopJob();
    if (job)
      return job;
    // no jobs are left - sleep for 30 seconds to allow new jobs to come in
    lock.Leave();
    bool newJob = m_jobEvent.WaitMSec(30000);
    lock.Enter();
    if (!newJob)
      break;
  }
  // ensure no jobs have come in during the period after
  // timeout and before we held the lock
  CJob *job = PopJob();
  if (job)
    return job;
  // have no jobs
  RemoveWorker(worker);
  return NULL;
}

bool CJobManager::OnJobProgress(unsigned int progress, unsigned int total, const CJob *job) const
{
  CSingleLock lock(m_section);
  // find the job in the processing queue, and check whether it's cancelled (no callback)
  Processing::const_iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
  {
    CWorkItem item(*i);
    lock.Leave(); // leave section prior to call
    if (item.m_callback)
    {
      item.m_callback->OnJobProgress(item.m_id, progress, total, job);
      return false;
    }
  }
  return true; // couldn't find the job, or it's been cancelled
}

void CJobManager::OnJobComplete(bool success, CJob *job)
{
  CSingleLock lock(m_section);
  // remove the job from the processing queue
  Processing::iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
  {
    // tell any listeners we're done with the job, then delete it
    CWorkItem item(*i);
    lock.Leave();
    try
    {
      if (item.m_callback)
        item.m_callback->OnJobComplete(item.m_id, success, item.m_job);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s error processing job %s", __FUNCTION__, item.m_job->GetType());
    }
    lock.Enter();
    Processing::iterator j = find(m_processing.begin(), m_processing.end(), job);
    if (j != m_processing.end())
      m_processing.erase(j);
    lock.Leave();
    item.FreeJob();
  }
}

void CJobManager::RemoveWorker(const CJobWorker *worker)
{
  CSingleLock lock(m_section);
  // remove our worker
  Workers::iterator i = find(m_workers.begin(), m_workers.end(), worker);
  if (i != m_workers.end())
    m_workers.erase(i); // workers auto-delete
}

unsigned int CJobManager::GetMaxWorkers(CJob::PRIORITY priority)
{
  static const unsigned int max_workers = 5;
  if (priority == CJob::PRIORITY_DEDICATED)
    return 10000; // A large number..
  return max_workers - (CJob::PRIORITY_HIGH - priority);
}
