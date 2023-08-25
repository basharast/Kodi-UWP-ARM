/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/HttpRangeUtils.h"

#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <microhttpd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#if MHD_VERSION >= 0x00097002
using MHD_RESULT = MHD_Result;
#else
using MHD_RESULT = int;
#endif

class CDateTime;
class CWebServer;

enum HTTPMethod
{
  UNKNOWN,
  POST,
  GET,
  HEAD
};

HTTPMethod GetHTTPMethod(const char *method);
std::string GetHTTPMethod(HTTPMethod method);

typedef enum HTTPResponseType
{
  HTTPNone,
  // creates and returns a HTTP error
  HTTPError,
  // creates and returns a HTTP redirect response
  HTTPRedirect,
  // creates a HTTP response with the content from a file
  HTTPFileDownload,
  // creates a HTTP response from a buffer without copying or freeing the buffer
  HTTPMemoryDownloadNoFreeNoCopy,
  // creates a HTTP response from a buffer by copying but not freeing the buffer
  HTTPMemoryDownloadNoFreeCopy,
  // creates a HTTP response from a buffer without copying followed by freeing the buffer
  // the buffer must have been malloc'ed and not new'ed
  HTTPMemoryDownloadFreeNoCopy,
  // creates a HTTP response from a buffer by copying followed by freeing the buffer
  // the buffer must have been malloc'ed and not new'ed
  HTTPMemoryDownloadFreeCopy
} HTTPResponseType;

typedef struct HTTPRequest
{
  CWebServer *webserver;
  struct MHD_Connection *connection;
  std::string pathUrlFull;
  std::string pathUrl;
  HTTPMethod method;
  std::string version;
  CHttpRanges ranges;
} HTTPRequest;

typedef struct HTTPResponseDetails {
  HTTPResponseType type;
  int status;
  std::multimap<std::string, std::string> headers;
  std::string contentType;
  uint64_t totalLength;
} HTTPResponseDetails;

class IHTTPRequestHandler
{
public:
  virtual ~IHTTPRequestHandler() = default;

  /*!
   * \brief Creates a new HTTP request handler for the given request.
   *
   * \details This call is responsible for doing some preparation work like -
   * depending on the supported features - determining whether the requested
   * entity supports ranges, whether it can be cached and what it's last
   * modified date is.
   *
   * \param request HTTP request to be handled
   */
  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) const = 0;

  /*!
   * \brief Returns the priority of the HTTP request handler.
   *
   * \details The higher the priority the more important is the HTTP request
   * handler.
   */
  virtual int GetPriority() const { return 0; }

  /*!
  * \brief Checks if the HTTP request handler can handle the given request.
  *
  * \param request HTTP request to be handled
  * \return True if the given HTTP request can be handled otherwise false.
  */
  virtual bool CanHandleRequest(const HTTPRequest &request) const = 0;

  /*!
   * \brief Handles the HTTP request.
   *
   * \return MHD_NO if a severe error has occurred otherwise MHD_YES.
   */
  virtual MHD_RESULT HandleRequest() = 0;

  /*!
   * \brief Whether the HTTP response could also be provided in ranges.
   */
  virtual bool CanHandleRanges() const { return false; }

  /*!
  * \brief Whether the HTTP response can be cached.
  */
  virtual bool CanBeCached() const { return false; }

  /*!
  * \brief Returns the maximum age (in seconds) for which the response can be cached.
  *
  * \details This is only used if the response can be cached.
  */
  virtual int GetMaximumAgeForCaching() const { return 0; }

  /*!
  * \brief Returns the last modification date of the response data.
  *
  * \details This is only used if the response can be cached.
  */
  virtual bool GetLastModifiedDate(CDateTime &lastModified) const { return false; }

  /*!
   * \brief Returns the ranges with raw data belonging to the response.
   *
   * \details This is only used if the response type is one of the HTTPMemoryDownload types.
   */
  virtual HttpResponseRanges GetResponseData() const { return HttpResponseRanges(); };

  /*!
  * \brief Returns the URL to which the request should be redirected.
  *
  * \details This is only used if the response type is HTTPRedirect.
  */
  virtual std::string GetRedirectUrl() const { return ""; }

  /*!
  * \brief Returns the path to the file that should be returned as the response.
  *
  * \details This is only used if the response type is HTTPFileDownload.
  */
  virtual std::string GetResponseFile() const { return ""; }

  /*!
  * \brief Returns the HTTP request handled by the HTTP request handler.
  */
  const HTTPRequest& GetRequest() const { return m_request; }

  /*!
  * \brief Returns true if the HTTP request is ranged, otherwise false.
  */
  bool IsRequestRanged() const { return m_ranged; }

  /*!
  * \brief Sets whether the HTTP request contains ranges or not
  */
  void SetRequestRanged(bool ranged) { m_ranged = ranged; }

  /*!
   * \brief Sets the response status of the HTTP response.
   *
   * \param status HTTP status of the response
   */
  void SetResponseStatus(int status) { m_response.status = status; }

  /*!
  * \brief Checks if the given HTTP header field is part of the response details.
  *
  * \param field HTTP header field name
  * \return True if the header field is set, otherwise false.
  */
  bool HasResponseHeader(const std::string &field) const;

  /*!
   * \brief Adds the given HTTP header field and value to the response details.
   *
   * \param field HTTP header field name
   * \param value HTTP header field value
   * \param allowMultiple Whether the same header is allowed multiple times
   * \return True if the header field was added, otherwise false.
   */
  bool AddResponseHeader(const std::string &field, const std::string &value, bool allowMultiple = false);

  /*!
  * \brief Returns the HTTP response header details.
  */
  const HTTPResponseDetails& GetResponseDetails() const { return m_response; }

  /*!
   * \brief Adds the given key-value pair extracted from the HTTP POST data.
   *
   * \param key Key of the HTTP POST field
   * \param value Value of the HTTP POST field
   */
  void AddPostField(const std::string &key, const std::string &value);
  /*!
  * \brief Adds the given raw HTTP POST data.
  *
  * \param data Raw HTTP POST data
  * \param size Size of the raw HTTP POST data
  */
  bool AddPostData(const char *data, size_t size);

protected:
  IHTTPRequestHandler();
  explicit IHTTPRequestHandler(const HTTPRequest &request);

  virtual bool appendPostData(const char *data, size_t size)
  { return true; }

  bool GetRequestedRanges(uint64_t totalLength);
  bool GetHostnameAndPort(std::string& hostname, uint16_t &port);

  HTTPRequest m_request;
  HTTPResponseDetails m_response;

  std::map<std::string, std::string> m_postFields;

private:
  bool m_ranged = false;
};
