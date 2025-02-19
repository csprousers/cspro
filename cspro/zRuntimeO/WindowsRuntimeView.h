#pragma once

#include <zRuntimeO/zRuntimeO.h>
#include <zHtml/HtmlViewerView.h>


class ZRUNTIMEO_API WindowsRuntimeView : public HtmlViewerView
{
    friend class WindowsRuntimeHost;

protected:
    WindowsRuntimeView();

public:
    ~WindowsRuntimeView();

    WindowsRuntimeHost& GetRuntimeHost() { return *m_runtimeHost; }

protected:
    // methods that can be overriden by subclasses
    virtual void SetRuntimeTitle(SharableString description);
    virtual void AllRuntimesClosed();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;

    LRESULT OnRunUiThreadAction(WPARAM wParam, LPARAM lParam);
    LRESULT OnProcessMessages(WPARAM wParam, LPARAM lParam);
    LRESULT OnAllRuntimesClosed(WPARAM wParam, LPARAM lParam);

private:
    std::unique_ptr<WindowsRuntimeHost> m_runtimeHost;
};
