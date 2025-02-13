#pragma once


class GlobalSettingsDlg : public CDialog
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
    void OnBrowseFile(std::wstring& path, const TCHAR* path_type);

private:
    GlobalSettings m_globalSettings;
    int m_automaticallyAssociateDocumentsWithDocSets;
#ifdef HELP_TODO_RESTORE_FOR_CSPRO81 
    int m_buildDocumentsOnOpen;
    std::wstring m_automaticCompilationSeconds;
#endif
};
