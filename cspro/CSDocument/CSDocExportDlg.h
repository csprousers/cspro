#pragma once

class DynamicLayoutControlResizer;


class CSDocExportDlg : public CDialog
{
public:
    CSDocExportDlg(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, std::wstring csdoc_filename, CLogicCtrl& logic_ctrl, CWnd* pParent = nullptr);
    ~CSDocExportDlg();

    std::unique_ptr<GenerateTask> ReleaseGenerateTask() { return std::move(m_generateTask); }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSize(UINT nType, int cx, int cy);

    void OnOK() override;

    void OnOutputFilenameBrowse();

    void OnSettingsChange();
    void OnBuildChange();
    void OnSettingsFileBrowse();

    void OnSettingsTextChange();

private:
    void SetSettingsText();

    std::optional<DocBuildSettings> GetDocBuildSettingsFromSettingsText(bool use_logic_ctrl_text, bool throw_exceptions);

    bool SyncOutputFilenameWithSettings(bool use_logic_ctrl_text);

    void UpdateNamedBuildSettings(const DocSetSettings& doc_set_settings);

private:
    SettingsDb m_settingsDb;

    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
    const std::wstring m_csdocFilename;
    CLogicCtrl& m_logicCtrl;

    DocSetCompiler m_docSetCompiler;
    JsonReaderInterface m_jsonReaderInterface;

    std::unique_ptr<GenerateTask> m_generateTask;

    std::wstring m_outputFilename;
    std::optional<std::wstring> m_lastSuggestedOutputFilename;

    int m_settingsButton;

    CComboBox m_docSetBuildsComboBox;
    std::vector<std::tuple<std::wstring, std::variant<DocBuildSettings, std::wstring>>> m_evaluatedBuildSettings;
    std::wstring m_selectedBuildSettingName;
    std::wstring m_buildSettingsSourceFilename;

    std::wstring m_settingsText;
    CLogicCtrl m_settingsLogicCtrl;

    bool m_settingsTextIsBeingPrefilled;
    std::unique_ptr<DynamicLayoutControlResizer> m_dynamicLayoutControlResizer;
};
