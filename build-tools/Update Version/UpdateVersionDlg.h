#pragma once

#include "VersionedFile.h"
#include <zUtilO/ResizableDlg.h>
#include <zUtilF/LoggingListBox.h>
#include <regex>


class UpdateVersionDlg : public ResizableDlg
{
public:
    UpdateVersionDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    LRESULT OnReadVersionNumbers(WPARAM wParam, LPARAM lParam);

    void OnOK() override;

private:
    static std::vector<VersionedFile> GetVersionedFiles();
    static VersionedFile ParseVersionedFile(const std::string& file_path);
    static std::tuple<std::vector<int>, char> SplitVersionText(const std::string& version_text);
    static VersionType ParseVersionText(const std::string& version_text);
    static void ParseFile(VersionedFile& versioned_file, std::string_view file_content_sv,
                          const std::regex& regex1, const std::regex* regex2);
    static void ParseResourceFile(VersionedFile& versioned_file, std::string_view file_content_sv);
    static void ParseAssemblyInfo(VersionedFile& versioned_file, std::string_view file_content_sv);
    static void SaveUpdatedVersionedFile(const VersionedFile& versioned_file, const std::vector<int>& version_numbers);

private:
    std::string m_version;
    LoggingListBox m_log;
    std::vector<VersionedFile> m_versionedFiles;
};
