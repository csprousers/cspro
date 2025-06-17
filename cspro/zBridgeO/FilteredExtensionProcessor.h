#pragma once

#include <zToolsO/WinSettings.h>

class FilteredExtensionProcessor
{
public:
    void AddType(const CString& type, std::vector<CString> extensions, bool set_as_type_for_unknown_extension = false);

    std::vector<const TCHAR*> GetTypes() const;

    CString GetTypeFromFilename(wstring_view filename) const;

    CString GetFilenameFromType(wstring_view type_text, const CString& filename) const;

    void SetWinSettingsType(WinSettings::Type type);

    void UpdateWinSettings(wstring_view filename) const;

private:
    std::vector<std::tuple<CString, std::vector<CString>>> m_typesAndExtensions;
    std::optional<size_t> m_unknownExtensionIndex;
    std::optional<WinSettings::Type> m_winSettingsType;
};

std::unique_ptr<FilteredExtensionProcessor> CreateListingFilteredExtensionProcessor();
std::unique_ptr<FilteredExtensionProcessor> CreateFrequencyFilteredExtensionProcessor();
