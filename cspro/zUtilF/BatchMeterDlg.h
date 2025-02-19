#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilF/ProcessSummaryReporter.h>
#include <mutex>


class CLASS_DECL_ZUTILF BatchMeterDlg : public CDialog, public ProcessSummaryReporter
{
public:
    BatchMeterDlg(CWnd* pParent = nullptr);

    void Initialize(InterfaceString title, std::shared_ptr<ProcessSummary> process_summary, CancelFlag* cancel_flag) override;
    void SetSource(InterfaceString source_text) override;
    void SetKey(const std::string& case_key) override;

    std::shared_ptr<ProcessSummary> GetProcessSummary() const { return m_processSummary; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnCancel() override;

    void OnDetails();

    void ToggleDetails(bool show_details);

    LRESULT OnUpdateDlg(WPARAM wParam, LPARAM lParam);

    void UpdateProcessSummary();

    void SetCompleted() { m_completedFlag = true; }

private:
    void UpdateText(std::wstring& destination_text, std::wstring source_text, WPARAM update_type);

    void SetCanceled();

private:
    bool m_initialized;
    bool m_cancelationPending;

    std::shared_ptr<ProcessSummary> m_processSummary;
    CancelFlag* m_cancelFlag;
    bool m_completedFlag;

    bool m_showingDetails;
    std::optional<std::tuple<int, int, int>> m_dialogWidthAndHeightDetailsNoDetails;

    std::mutex m_memberAccessMutex;
    std::wstring m_dialogTitle;
    std::wstring m_sourceText;
    std::wstring m_caseKey;

    CTime m_startTime;

    CButton* m_dlgItemDetailsButton;

    CWnd* m_dlgItemSource;
    CWnd* m_dlgItemCaseKey;

    CProgressCtrl* m_dlgItemPercentBar;
    CWnd* m_dlgItemPercentRead;

    CWnd* m_dlgItemRecordsRead;

    CWnd* m_dlgItemElapsedTime;
    CWnd* m_dlgItemRemainingTime;

    CWnd* m_dlgItemMessagesUser;
    CWnd* m_dlgItemMessagesWarning;
    CWnd* m_dlgItemMessagesError;
    CWnd* m_dlgItemMessagesTotal;

    CWnd* m_dlgItemAttibutesUnknown;
    CWnd* m_dlgItemAttibutesErased;
    CWnd* m_dlgItemAttibutesIgnored;

    CListCtrl* m_levelDetails;
};
