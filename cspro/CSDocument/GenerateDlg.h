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
    void SetTitle(const std::wstring& title) override;
    void LogText(std::wstring text) override;
    void UpdateProgress(double percent) override;
    void SetOutputText(const std::wstring& text) override;
    void OnCreatedOutput(std::wstring output_title, std::wstring path) override;
    void OnException(const CSProException& exception) override;
    void OnCompletion(GenerateTask::Status status) override;
    const GlobalSettings& GetGlobalSettings() override;

private:
    void PostTextForUpdate(CWnd* pWnd, std::wstring text);

private:
    GlobalSettings& m_globalSettings;
    GenerateTask& m_generateTask;
    std::vector<std::tuple<std::wstring, std::wstring>> m_finalOutputs;

    LoggingListBox m_loggingListBox;
    CProgressCtrl m_progressCtrl;
    int m_closeDialogOnCompletion;

    std::vector<std::unique_ptr<std::wstring>> m_postedTextUpdates;
    std::mutex m_postedTextUpdatesMutex;
};
