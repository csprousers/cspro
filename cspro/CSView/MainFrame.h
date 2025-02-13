#pragma once

#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/MFCMenuBarWithoutSerializableState.h>

class EngineUIProcessor;
class ObjectTransporter;


class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();
    ~CMainFrame();

    SharedHtmlLocalFileServer& GetSharedHtmlLocalFileServer() { return m_fileServer; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);

    void ActivateFrame(int nCmdShow) override;

    void OnUpdateFrameTitle(BOOL bAddToTitle) override;

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

private:
    MFCMenuBarWithoutSerializableState m_wndMenuBar;

    bool m_onInitialActivateFrame;

    SharedHtmlLocalFileServer m_fileServer;
    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    std::unique_ptr<EngineUIProcessor> m_engineUIProcessor;
    ApplicationShutdownRunner m_applicationShutdownRunner;
};
