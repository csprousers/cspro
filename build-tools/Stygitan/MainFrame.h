#pragma once


class MainFrame : public CMDIFrameWnd
{
public:
    MainFrame();

    SettingsDb& GetGlobalSettingsDb() { return m_globalSettingsDb; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnClose();

    void OnProperties();

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);

private:
    SettingsDb m_globalSettingsDb;
};
