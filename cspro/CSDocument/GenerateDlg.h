#pragma once

#include <zUtilF/LoggingListBox.h>


class GenerateDlg : public CDialog, public GenerateTask::Interface
{
public:
    GenerateDlg(GenerateTask& generate_task, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCancel() override;

    void OnClose();

    void OnOutputsClick(NMHDR* pNMHDR, LRESULT* pResult);

    LRESULT OnUpdateText(WPARAM wParam, LPARAM lParam);
    LRESULT OnGenerateTaskComplete(WPARAM wParam, LPARAM lParam);

    // GenerateTask::Interface overrides
    // the methods will post messages to update values as they will be called from a non-UI thread
    void SetTitle(const std::string& title) override;
    void LogText(SharableString text) override;
    void UpdateProgress(double percent) override;
    void SetOutputText(const std::string& text) override;
    void OnCreatedOutput(std::string output_title, std::string path) override;
    void OnException(const CSProException& exception) override;
    void OnCompletion(GenerateTask::Status status) override;
    const GlobalSettings& GetGlobalSettings() override;

private:
    void PostTextForUpdate(CWnd* pWnd, std::string_view text_sv);

private:
    GlobalSettings& m_globalSettings;
    GenerateTask& m_generateTask;
    std::vector<std::tuple<std::string, std::string>> m_finalOutputs;

    LoggingListBox m_loggingListBox;
    CProgressCtrl m_progressCtrl;
    int m_closeDialogOnCompletion;

    std::vector<std::unique_ptr<std::wstring>> m_postedTextUpdates;
    std::mutex m_postedTextUpdatesMutex;
};
