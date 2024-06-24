/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireRegistry.h"

#include "cores/AudioEngine/Sinks/pipewire/PipewireCore.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireNode.h"
#include "utils/log.h"

#include <stdexcept>

#include <pipewire/keys.h>
#include <pipewire/node.h>
#include <pipewire/type.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireRegistry::CPipewireRegistry(CPipewireCore& core)
  : m_core(core), m_registryEvents(CreateRegistryEvents())
{
  m_registry.reset(pw_core_get_registry(core.Get(), PW_VERSION_REGISTRY, 0));
  if (!m_registry)
  {
    CLog::Log(LOGERROR, "CPipewireRegistry: failed to create registry: {}", strerror(errno));
    throw std::runtime_error("CPipewireRegistry: failed to create registry");
  }
}

void CPipewireRegistry::AddListener()
{
  pw_registry_add_listener(m_registry.get(), &m_registryListener, &m_registryEvents, this);
}

void CPipewireRegistry::OnGlobalAdded(void* userdata,
                                      uint32_t id,
                                      uint32_t permissions,
                                      const char* type,
                                      uint32_t version,
                                      const struct spa_dict* props)
{
  auto registry = reinterpret_cast<CPipewireRegistry*>(userdata);

  if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0)
  {
    const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!mediaClass)
      return;

    if (strcmp(mediaClass, "Audio/Sink") != 0)
      return;

    const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (!name)
      return;

    const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (!desc)
      return;

    auto& globals = registry->GetGlobals();

    globals[id] = std::make_unique<global>();
    globals[id]->name = std::string(name);
    globals[id]->description = std::string(desc);
    globals[id]->id = id;
    globals[id]->permissions = permissions;
    globals[id]->type = std::string(type);
    globals[id]->version = version;
    globals[id]->properties.reset(pw_properties_new_dict(props));
    globals[id]->proxy = std::make_unique<CPipewireNode>(*registry, id, type);
  }
}

void CPipewireRegistry::OnGlobalRemoved(void* userdata, uint32_t id)
{
  auto registry = reinterpret_cast<CPipewireRegistry*>(userdata);
  auto& globals = registry->GetGlobals();

  auto global = globals.find(id);
  if (global != globals.end())
  {
    CLog::Log(LOGDEBUG, "CPipewireRegistry::{} - id={} type={}", __FUNCTION__, id,
              global->second->type);

    globals.erase(global);
  }
}

pw_registry_events CPipewireRegistry::CreateRegistryEvents()
{
  pw_registry_events registryEvents = {};
  registryEvents.version = PW_VERSION_REGISTRY_EVENTS;
  registryEvents.global = OnGlobalAdded;
  registryEvents.global_remove = OnGlobalRemoved;

  return registryEvents;
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
