#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF SelectFileDlg : public CSHtmlDlgRunner
{
public:
    SelectFileDlg();

    // inputs
    void SetTitle(std::wstring title) { m_title = std::move(title); }

    void SetShowDirectories(bool show_directories) { m_showDirectories = show_directories; }

    void SetFilter(std::wstring filter) { m_filter = std::move(filter); }

    void SetStartDirectory(SpecialDirectoryLister::SpecialDirectory directory) { m_startDirectory = std::move(directory); }
    void SetRootDirectory(SpecialDirectoryLister::SpecialDirectory directory)  { m_rootDirectory = std::move(directory); }

    // results
    const std::wstring& GetSelectedPath() const { return m_selectedPath; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::optional<std::wstring> m_title;
    bool m_showDirectories;
    std::optional<std::wstring> m_filter;
    SpecialDirectoryLister::SpecialDirectory m_startDirectory;
    std::optional<SpecialDirectoryLister::SpecialDirectory> m_rootDirectory;

    std::wstring m_selectedPath;
};
