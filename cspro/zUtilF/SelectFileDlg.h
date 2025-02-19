#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF SelectFileDlg : public CSHtmlDlgRunner
{
public:
    SelectFileDlg();

    // inputs
    void SetTitle(SharableString title) { m_title = std::move(title); }

    void SetShowDirectories(bool show_directories) { m_showDirectories = show_directories; }

    void SetFilter(SharableString filter) { m_filter = std::move(filter); }

    void SetStartDirectory(SpecialDirectoryLister::SpecialDirectory directory) { m_startDirectory = std::move(directory); }
    void SetRootDirectory(SpecialDirectoryLister::SpecialDirectory directory)  { m_rootDirectory = std::move(directory); }

    // results
    const SharableString& GetSelectedPath() const { return m_selectedPath; }

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    SharableString m_title;
    bool m_showDirectories;
    SharableString m_filter;
    SpecialDirectoryLister::SpecialDirectory m_startDirectory;
    std::optional<SpecialDirectoryLister::SpecialDirectory> m_rootDirectory;

    SharableString m_selectedPath;
};
