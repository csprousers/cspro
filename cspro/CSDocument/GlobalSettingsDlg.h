#pragma once

#include <zUtilO/ResizableDlg.h>


class GlobalSettingsDlg : public ResizableDlg
{
public:
    GlobalSettingsDlg(GlobalSettings global_settings, CWnd* pParent = NULL);

    GlobalSettings ReleaseGlobalSettings() { return std::move(m_globalSettings); }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnBrowseHtmlHelpCompiler();
    void OnBrowseWkhtmltopdf();
    void OnBrowseCSProCode();

private:
    void OnBrowseFile(std::string& path, const char* path_type);

private:
    GlobalSettings m_globalSettings;
    int m_automaticallyAssociateDocumentsWithDocSets;
#ifdef HELP_TODO_RESTORE_FOR_CSPRO8X
    int m_buildDocumentsOnOpen;
    std::string m_automaticCompilationSeconds;
#endif
};
