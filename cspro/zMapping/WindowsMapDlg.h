#pragma once

#include <zMapping/WindowsMapUI.h>
#include <zHtml/HtmlViewDlg.h>


class WindowsMapDlg : public HtmlViewDlg
{
public:
    WindowsMapDlg(WindowsMapUI& map_ui, CWnd* pParent = nullptr);
    ~WindowsMapDlg();

    void SaveSnapshot(const std::string& file_path);

    void SetWindowTitle(const std::string& title);

protected:
    DECLARE_MESSAGE_MAP()

    LRESULT OnPostActionMessage(WPARAM wParam, LPARAM lParam);
    LRESULT OnSaveSnapshot(WPARAM wParam, LPARAM lParam);

private:
    struct SnapshotData;
    WindowsMapUI& m_mapUI;
};
