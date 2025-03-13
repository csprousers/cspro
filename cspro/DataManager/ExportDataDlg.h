#pragma once

#include <DataManager/ExportDataSettings.h>
#include <zUtilO/ResizableDlg.h>


class ExportDataDlg : public ResizableDlg
{
public:
    ExportDataDlg(const CaseHoldingDoc& case_holding_doc, ExportDataSettings settings,
                  std::shared_ptr<CaseProvider> case_provider, CWnd* pParent = nullptr);

    const ExportDataSettings& GetSettings() const { return m_settings; }

    std::vector<ConnectionString> ReleaseExportConnectionStrings() { return std::move(m_exportConnectionStrings); }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnFormatChange(UINT nID);
    void OnChangeBaseFilePath();
    void OnSelectBaseFilePath();
    void OnOneFilePerRecordChange();

    LRESULT OnUpdateDialogControls(WPARAM wParam, LPARAM lParam);

private:
    template<typename OT, typename IT>
    static OT ConvertResourceIdDataRepositoryType(IT input);

    void ConstructExportConnectionStrings();
    void AddExportConnectionString(std::string_view connection_string_text_sv);

private:
    const CaseHoldingDoc& m_caseHoldingDoc;
    ExportDataSettings m_settings;
    std::shared_ptr<CaseProvider> m_caseProvider;
    std::vector<std::string> m_recordNames;

    CWnd* m_baseFilePathWnd;
    CListBox* m_outputsListBox;
    CWnd* m_okWnd;

    std::optional<std::string> m_lastValidCheckedBaseFilePath;
    std::vector<ConnectionString> m_exportConnectionStrings;
};
