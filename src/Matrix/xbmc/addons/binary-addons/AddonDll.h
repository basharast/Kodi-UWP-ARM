/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BinaryAddonManager.h"
#include "DllAddon.h"
#include "addons/Addon.h"
#include "utils/XMLUtils.h"

// Global addon callback handle classes
#include "addons/interfaces/AddonBase.h"

namespace ADDON
{

/*!
 * Addon instance handler, used as identify for std::map to find related
 * addon instance. This class itself not accessed here.
 *
 * @todo As long game addon system use CAddonDll itself and not
 * IAddonInstanceHandler as parent, is the set of this as "void*" needed.
 * After game system is changed should by this also changed to
 * "const IAddonInstanceHandler*" or direct in map below.
 */
using ADDON_INSTANCE_HANDLER = const void*;

/*!
 * @brief Information class for use on addon type managers.
 *
 * This to query via @ref CAddonDll the manager so that work can be performed.
 * If there are multiple instances it be harder to be informed about any changes.
 */
class CAddonDllInformer
{
public:
  virtual bool IsInUse(const std::string& id) = 0;
};

class CAddonDll : public CAddon
{
public:
  CAddonDll(const AddonInfoPtr& addonInfo, BinaryAddonBasePtr addonBase);
  CAddonDll(const AddonInfoPtr& addonInfo, TYPE addonType);
  ~CAddonDll() override;

  // Implementation of IAddon via CAddon
  std::string LibPath() const override;

  // addon settings
  void SaveSettings() override;

  bool DllLoaded(void) const;

  /*!
   * @brief Get api version of moduleType type
   *
   * @return The version of requested type, if dll is loaded and supported by addon.
   *         If one of both do not match, an empty version is returned.
   *
   * @note This should only be called if the associated dll is loaded.
   * Otherwise use @ref CAddonInfo::DependencyVersion(...)
   */
  AddonVersion GetTypeVersionDll(int type) const;

  /*!
   * @brief Get api min version of moduleType type
   *
   * @return The version of requested type, if dll is loaded and supported by addon.
   *         If one of both do not match, an empty version is returned.
   *
   * @note This should only be called if the associated dll is loaded.
   * Otherwise use @ref CAddonInfo::DependencyMinVersion(...)
   */
  AddonVersion GetTypeMinVersionDll(int type) const;

  /*!
   * @brief Function to create a addon instance class
   *
   * @param[in] instanceType The wanted instance type class to open on addon
   * @param[in] instanceClass The from Kodi used class for active instance
   * @param[in] instanceID The from addon used ID string of active instance
   * @param[in] instance Pointer where the interface functions from addon
   *                     becomes stored during his instance creation.
   * @param[in] parentInstance In case the instance class is related to another
   *                           addon instance class becomes with the pointer
   *                           given to addon. Is optional and most addon types
   *                           not use it.
   * @return The status of addon after the creation.
   */
  ADDON_STATUS CreateInstance(ADDON_TYPE instanceType,
                              ADDON_INSTANCE_HANDLER instanceClass,
                              const std::string& instanceID,
                              KODI_HANDLE instance,
                              KODI_HANDLE parentInstance = nullptr);

  /*!
   * @brief Function to destroy a on addon created instance class
   *
   * @param[in] instanceClass The from Kodi used class for active instance
   */
  void DestroyInstance(ADDON_INSTANCE_HANDLER instanceClass);

  bool IsInUse() const override;
  void RegisterInformer(CAddonDllInformer* informer);
  AddonPtr GetRunningInstance() const override;

  void OnPreInstall() override;
  void OnPostInstall(bool update, bool modal) override;
  void OnPreUnInstall() override;
  void OnPostUnInstall() override;

  bool Initialized() const { return m_initialized; }

protected:
  static std::string GetDllPath(const std::string& strFileName);

  std::string m_parentLib;

private:
  /*!
   * @brief Main addon creation call function
   *
   * This becomes called only one time before a addon instance becomes created.
   * If another instance becomes requested is this Create no more used. To see
   * like a "int main()" on exe.
   *
   * @param[in] firstKodiInstance The first instance who becomes used.
   *                              In case addon supports only one instance
   *                              and not multiple together can addon use
   *                              only one complete class for everything.
   *                              This is used then to interact on interface.
   * @return The status of addon after the creation.
   */
  ADDON_STATUS Create(KODI_HANDLE firstKodiInstance);

  /*!
   * @brief Main addon destroying call function
   *
   * This becomes called only one time after the last addon instance becomes destroyed.
   */
  void Destroy();

  bool CheckAPIVersion(int type);

  BinaryAddonBasePtr m_binaryAddonBase = nullptr;
  DllAddon* m_pDll = nullptr;
  bool m_initialized = false;
  bool LoadDll();
  std::map<ADDON_INSTANCE_HANDLER, std::pair<ADDON_TYPE, KODI_HANDLE>> m_usedInstances;
  CAddonDllInformer* m_informer = nullptr;

  virtual ADDON_STATUS TransferSettings();

  /*!
   * This structure, which is fixed to the addon headers, makes use of the at
   * least supposed parts for the interface.
   * This structure is defined in:
   * /xbmc/addons/kodi-dev-kit/include/kodi/AddonBase.h
   */
  AddonGlobalInterface m_interface = {0};
};

} /* namespace ADDON */
