#pragma once

#include <zMapping/WindowsMapDlg.h>
#include <zUtilF/UIThreadRunner.h>


class WindowsMapUIThreadRunner : public UIThreadRunner
{
public:
    WindowsMapUIThreadRunner(WindowsMapUI& map_ui)
        :   m_mapUI(map_ui)
    {
    }

    void Execute() override
    {
        m_dlg = std::make_unique<WindowsMapDlg>(m_mapUI);
        m_dlg->DoModal();
        m_dlg.reset();
    }

    WindowsMapDlg* GetMapDlg()
    {
        ASSERT(m_dlg != nullptr);
        return m_dlg.get();
    }

private:
    WindowsMapUI& m_mapUI;
    std::unique_ptr<WindowsMapDlg> m_dlg;
};
