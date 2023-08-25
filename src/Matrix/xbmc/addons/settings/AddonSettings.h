/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/SettingsBase.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/SettingDependency.h"
#include "settings/lib/SettingSection.h"
#include "utils/logtypes.h"

#include <map>
#include <memory>
#include <string>

class CXBMCTinyXML;
struct StringSettingOption;

namespace ADDON
{
  class IAddon;

  class CAddonSettings : public CSettingsBase, public CSettingCreator, public CSettingControlCreator, public ISettingCallback
  {
  public:
    explicit CAddonSettings(const std::shared_ptr<const IAddon>& addon);
    ~CAddonSettings() override = default;

    // specialization of CSettingsBase
    bool Initialize() override { return false; }

    // implementations of CSettingsBase
    bool Load() override { return false; }
    bool Save() override { return false; }

    // specialization of CSettingCreator
    std::shared_ptr<CSetting> CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = NULL) const override;

    // implementation of ISettingCallback
    void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

    std::shared_ptr<const IAddon> GetAddon() { return m_addon.lock(); }

    bool Initialize(const CXBMCTinyXML& doc, bool allowEmpty = false);
    bool Load(const CXBMCTinyXML& doc);
    bool Save(CXBMCTinyXML& doc) const;

    bool HasSettings() const;

    std::string GetSettingLabel(int label) const;

    std::shared_ptr<CSetting> AddSetting(const std::string& settingId, bool value);
    std::shared_ptr<CSetting> AddSetting(const std::string& settingId, int value);
    std::shared_ptr<CSetting> AddSetting(const std::string& settingId, double value);
    std::shared_ptr<CSetting> AddSetting(const std::string& settingId, const std::string& value);

  protected:
    // specializations of CSettingsBase
    void InitializeSettingTypes() override;
    void InitializeControls() override;
    void InitializeConditions() override;

    // implementation of CSettingsBase
    bool InitializeDefinitions() override { return false; }

  private:
    bool InitializeDefinitions(const CXBMCTinyXML& doc);

    bool ParseSettingVersion(const CXBMCTinyXML& doc, uint32_t& version) const;

    std::shared_ptr<CSettingGroup> ParseOldSettingElement(
        const TiXmlElement* categoryElement,
        const std::shared_ptr<CSettingCategory>& category,
        std::set<std::string>& settingIds);

    std::shared_ptr<CSettingCategory> ParseOldCategoryElement(uint32_t &categoryId, const TiXmlElement * categoryElement, std::set<std::string>& settingIds);

    bool InitializeFromOldSettingDefinitions(const CXBMCTinyXML& doc);
    std::shared_ptr<CSetting> InitializeFromOldSettingAction(const std::string& settingId,
                                                             const TiXmlElement* settingElement,
                                                             const std::string& defaultValue);
    std::shared_ptr<CSetting> InitializeFromOldSettingLabel();
    std::shared_ptr<CSetting> InitializeFromOldSettingBool(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue);
    std::shared_ptr<CSetting> InitializeFromOldSettingTextIpAddress(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingNumber(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingPath(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingDate(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingTime(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingSelect(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel, const std::string& settingValues, const std::vector<std::string>& settingLValues);
    std::shared_ptr<CSetting> InitializeFromOldSettingAddon(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel);
    std::shared_ptr<CSetting> InitializeFromOldSettingEnums(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const std::string& settingValues, const std::vector<std::string>& settingLValues);
    std::shared_ptr<CSetting> InitializeFromOldSettingFileEnum(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const std::string& settingValues);
    std::shared_ptr<CSetting> InitializeFromOldSettingRangeOfNum(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue);
    std::shared_ptr<CSetting> InitializeFromOldSettingSlider(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue);
    std::shared_ptr<CSetting> InitializeFromOldSettingFileWithSource(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, std::string source);

    bool LoadOldSettingValues(const CXBMCTinyXML& doc, std::map<std::string, std::string>& settings) const;

    struct ConditionExpression
    {
      SettingDependencyOperator m_operator;
      bool m_negated;
      int32_t m_relativeSettingIndex;
      std::string m_value;
    };

    bool ParseOldLabel(const TiXmlElement* element, const std::string& settingId, int& labelId);
    bool ParseOldCondition(const std::shared_ptr<const CSetting>& setting,
                           const std::vector<std::shared_ptr<const CSetting>>& settings,
                           const std::string& condition,
                           CSettingDependency& dependeny) const;
    static bool ParseOldConditionExpression(std::string str, ConditionExpression& expression);

    static void FileEnumSettingOptionsFiller(const std::shared_ptr<const CSetting>& setting,
                                             std::vector<StringSettingOption>& list,
                                             std::string& current,
                                             void* data);

    std::weak_ptr<const IAddon> m_addon;
    // store these values so that we don't always have to access the weak pointer
    const std::string m_addonId;
    const std::string m_addonPath;
    const std::string m_addonProfile;

    uint32_t m_unidentifiedSettingId;
    int m_unknownSettingLabelId;
    std::map<int, std::string> m_unknownSettingLabels;

    Logger m_logger;
  };
}
