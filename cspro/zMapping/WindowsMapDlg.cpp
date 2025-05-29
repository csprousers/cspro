#include "stdafx.h"
#include "WindowsMapDlg.h"
#include <zToolsO/Utf8.h>


BEGIN_MESSAGE_MAP(WindowsMapDlg, HtmlViewDlg)
    ON_MESSAGE(UWM::Mapping::PostActionMessage, OnPostActionMessage)
    ON_MESSAGE(UWM::Mapping::SaveSnapshot, OnSaveSnapshot)
END_MESSAGE_MAP()


WindowsMapDlg::WindowsMapDlg(WindowsMapUI& map_ui, CWnd* const pParent/*= nullptr*/)
    :   HtmlViewDlg(pParent),
        m_mapUI(map_ui)
{
    m_htmlViewCtrl.AddWebEventObserver(
        [&](const std::wstring_view message_sv)
        {
            m_mapUI.OnWebMessageReceived(TC::ToUtf8(message_sv));
        });

    SetInitialUrl(m_mapUI.GetUrlOfMapHtml());

    // set the default dialog title (if not overridden already)
    if( !m_viewerOptions.title.IsSet() )
        m_viewerOptions.title = "CSPro Map";
}


WindowsMapDlg::~WindowsMapDlg()
{
    m_mapUI.NotifyEvent(IMapUI::EventCode::MapClosed);
}


LRESULT WindowsMapDlg::OnPostActionMessage(const WPARAM wParam, LPARAM /*lParam*/)
{
    const SharableString message = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);
    ASSERT(message.IsSet());

    m_htmlViewCtrl.PostWebMessageAsString(*message);

    return 1;
}


struct WindowsMapDlg::SnapshotData
{
    const std::string& file_path;
    std::optional<std::string> exception_message;
};


void WindowsMapDlg::SaveSnapshot(const std::string& file_path)
{
    // send a message to save the snapshot on the UI thread
    SnapshotData snapshot_data { file_path };
    SendMessage(UWM::Mapping::SaveSnapshot, reinterpret_cast<WPARAM>(&snapshot_data));

    if( snapshot_data.exception_message.has_value() )
        throw CSProException(*snapshot_data.exception_message);
}


LRESULT WindowsMapDlg::OnSaveSnapshot(const WPARAM wParam, LPARAM /*lParam*/)
{
    SnapshotData& snapshot_data = *reinterpret_cast<SnapshotData*>(wParam);
    ASSERT(!snapshot_data.exception_message.has_value());

    try
    {
        m_htmlViewCtrl.SaveScreenshot(snapshot_data.file_path);
    }

    catch( const CSProException& exception )
    {
        snapshot_data.exception_message.emplace(exception.what());
    }

    return 0;
}


void WindowsMapDlg::SetWindowTitle(const std::string& title)
{
    ASSERT(m_viewerOptions.title.IsSet());
    SetWindowText(TC::ToWide(title.empty() ? *m_viewerOptions.title : title).c_str());
}
