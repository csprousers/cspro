#include "stdafx.h"
#include "SyncServiceSelectorDlg.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/WinSettings.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/FileUtil.h>
#include <zNetwork/SyncConnectionStringProperties.h>


namespace
{
    constexpr int SyncServiceCSWeb       = 0;
    constexpr int SyncServiceDropbox     = 1;
    constexpr int SyncServiceFtp         = 2;
    constexpr int SyncServiceLocalFiles  = 3;
    constexpr int SyncServiceCustom      = 4;
}


BEGIN_MESSAGE_MAP(SyncServiceSelectorDlg, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SERVICE_CSWEB, IDC_SERVICE_CUSTOM, OnSyncServiceChange)
    ON_BN_CLICKED(IDC_DIRECTORY_SELECT, OnDirectorySelect)
    ON_MESSAGE(UWM::Sync::UpdateDialogUI, OnUpdateDialogUI)
END_MESSAGE_MAP()


SyncServiceSelectorDlg::SyncServiceSelectorDlg(cs::cref_optional<SyncConnectionString> sync_connection_string, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_SYNC_SERVICE_SELECTOR, pParent)
{
    // if the sync connection string is not defined, use one from the registry
    if( !sync_connection_string.has_value() || !sync_connection_string->IsDefined() )
        sync_connection_string.emplace(WinSettings::Read<std::string>(WinSettings::Type::LastSyncConnectionString));

    ToForm(*sync_connection_string, std::nullopt);
}


BOOL SyncServiceSelectorDlg::Create(CDialog* const parent_dlg, const UINT nID)
{
    CWnd* const placeholder_ctrl = parent_dlg->GetDlgItem(nID);

    if( placeholder_ctrl == nullptr )
        return ReturnProgrammingError(FALSE);

    CRect rect;
    placeholder_ctrl->GetWindowRect(&rect);
    parent_dlg->ScreenToClient(&rect);

    placeholder_ctrl->DestroyWindow();

    // create this dialog where the destroyed placeholder was
    if( !__super::Create(m_lpszTemplateName, parent_dlg) )
        return FALSE;

    SetWindowPos(nullptr, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);
    ShowWindow(SW_SHOW);

    SetDlgCtrlID(nID);

    PostMessage(UWM::Sync::UpdateDialogUI);

    return TRUE;
}


void SyncServiceSelectorDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_SERVICE_CSWEB, m_syncServiceType);
    DDX_CBIndex(pDX, IDC_DROPBOX_LOCAL_COMBO, m_dropboxLocalType);
    DDX_Text(pDX, IDC_URL, m_text, true);
}


void SyncServiceSelectorDlg::OnSyncServiceChange(UINT /*nID*/)
{
    const int previous_sync_service_type = m_syncServiceType;

    UpdateData(TRUE);

    // return if the user clicked on the same sync service type
    if( previous_sync_service_type == m_syncServiceType )
        return;

    SyncConnectionString previous_service_sync_connection_string = FromForm(previous_sync_service_type);

    // when the user changes the sync service type, restore any sync connection string specified using
    // that sync service type, except for when changing to Custom; in this case, we'll show the current
    // sync connection string so that the user can modify it as desired
    const SyncConnectionString* sync_connection_string_to_restore;

    if( m_syncServiceType == SyncServiceCustom )
    {
        sync_connection_string_to_restore = &previous_service_sync_connection_string;
    }

    else
    {
        const auto& lookup = m_syncConnectionStringsPerServiceType.find(m_syncServiceType);
        sync_connection_string_to_restore = ( lookup != m_syncConnectionStringsPerServiceType.cend() ) ? &lookup->second :
                                                                                                         nullptr;
    }

    if( sync_connection_string_to_restore != nullptr )
    {
        ToForm(*sync_connection_string_to_restore, m_syncServiceType);
    }

    else
    {
        ToForm(SyncConnectionString(), m_syncServiceType);
    }

    UpdateData(FALSE);

    m_syncConnectionStringsPerServiceType[previous_sync_service_type] = std::move(previous_service_sync_connection_string);

    PostMessage(UWM::Sync::UpdateDialogUI);
}


void SyncServiceSelectorDlg::OnDirectorySelect()
{
    UpdateData(TRUE);

    const SyncConnectionString sync_connection_string = FromForm(std::nullopt);

    const std::string& current_directory =
        ( sync_connection_string.IsDefined() &&
          sync_connection_string.GetType() == SyncServiceType::LocalFiles ) ? sync_connection_string.GetEvaluatedDirectoryPath() :
                                                                              SO::Empty_string;

    std::optional<std::string> directory = SelectFolderDialog(GetSafeHwnd(), "Select Directory for Local Files", current_directory);

    if( !directory.has_value() )
        return;

    ToForm(SyncConnectionString::CreateLocalFilesSyncConnectionString(std::move(*directory)), std::nullopt);

    UpdateData(FALSE);
}


LRESULT SyncServiceSelectorDlg::OnUpdateDialogUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    UpdateData(TRUE);

    auto update_dialog_ui = [&](const wchar_t* const text_description,
                                const bool show_dropbox_local_combo,
                                const bool show_directory_select_button)
    {
        SetDlgItemText(IDC_URL_DESCRIPTION, text_description);
        GetDlgItem(IDC_DROPBOX_LOCAL_COMBO)->ShowWindow(show_dropbox_local_combo ? SW_SHOW : SW_HIDE);
        GetDlgItem(IDC_DIRECTORY_SELECT)->ShowWindow(show_directory_select_button ? SW_SHOW : SW_HIDE);
    };

    switch( m_syncServiceType )
    {
        case SyncServiceCSWeb:
        case SyncServiceFtp:
        case SyncServiceCustom:
            update_dialog_ui(L"URL:", false, false);
            break;

        case SyncServiceDropbox:
            update_dialog_ui(L"Email:", true, false);
            break;

        case SyncServiceLocalFiles:
            update_dialog_ui(L"Directory:", false, true);
            break;

        default:
            ASSERT(false);
            break;
    }

    return 1;
}


template<typename T>
static auto SyncServiceSelectorDlg::ConvertSyncServiceType(const T& value)
{
    if constexpr(std::is_same_v<T, int>)
    {
        switch( value )
        {
            case SyncServiceCSWeb:      return std::make_optional(SyncServiceType::CSWeb);
            case SyncServiceDropbox:    return std::make_optional(SyncServiceType::Dropbox);
            case SyncServiceFtp:        return std::make_optional(SyncServiceType::Ftp);
            case SyncServiceLocalFiles: return std::make_optional(SyncServiceType::LocalFiles);
            case SyncServiceCustom:     return std::optional<SyncServiceType>();
            default:                    return ReturnProgrammingError(std::optional<SyncServiceType>());
        }
    }

    else
    {
        if( !value.IsDefined() )
            return SyncServiceCSWeb;

        switch( value.GetType() )
        {
            case SyncServiceType::CSWeb:      return SyncServiceCSWeb;
            case SyncServiceType::Dropbox:    return SyncServiceDropbox;
            case SyncServiceType::Ftp:        return SyncServiceFtp;
            case SyncServiceType::LocalFiles: return SyncServiceLocalFiles;
            default:                          return SyncServiceCustom;
        }
    }
}


void SyncServiceSelectorDlg::ToForm(const SyncConnectionString& sync_connection_string, const std::optional<int> sync_service_type_override)
{
    m_syncServiceType = ( sync_service_type_override.has_value() ) ? *sync_service_type_override :
                                                                     ConvertSyncServiceType(sync_connection_string);

    m_dropboxLocalType = 0;

    size_t ui_based_properties_count = 0;

    if( m_syncServiceType == SyncServiceCSWeb )
    {
        m_text = sync_connection_string.ToString();

        if( m_text.empty() )
            m_text = "https://";
    }

    else if( m_syncServiceType == SyncServiceDropbox )
    {
        const std::string* const use_local_property = sync_connection_string.GetProperty(SCSProperty::useLocal);

        if( use_local_property != nullptr )
        {
            ++ui_based_properties_count;

            if( SO::EqualsNoCase(*use_local_property, SCSValue::true_) )
            {
                m_dropboxLocalType = 1;
            }

            else if( SO::EqualsNoCase(*use_local_property, SCSValue::try_) )
            {
                m_dropboxLocalType = 2;
            }

            else if( !SO::EqualsNoCase(*use_local_property, SCSValue::false_) )
            {
                --ui_based_properties_count;
            }
        }

        const std::string* const email_property = sync_connection_string.GetProperty(SCSProperty::email);

        if( email_property != nullptr )
        {
            ++ui_based_properties_count;
            m_text = *email_property;
        }

        else
        {
            m_text.clear();
        }
    }

    else if( m_syncServiceType == SyncServiceFtp )
    {
        m_text = sync_connection_string.ToString();

        if( m_text.empty() )
            m_text = "ftp://";
    }

    else
    {
        ASSERT(m_syncServiceType == SyncServiceLocalFiles ||
               m_syncServiceType == SyncServiceCustom);

        m_text = sync_connection_string.ToString();
    }

    // when not overridden, if a sync connection string has properties that cannot be represented
    // using the dialog UI, the sync connection string will be shown using the custom option
    if( !sync_service_type_override.has_value() && ui_based_properties_count != sync_connection_string.GetProperties().size() )
    {
        m_syncServiceType = SyncServiceCustom;
        m_text = sync_connection_string.ToString();
    }
}


SyncConnectionString SyncServiceSelectorDlg::FromForm(const std::optional<int> sync_service_type_override) const
{
    switch( sync_service_type_override.value_or(m_syncServiceType) )
    {
        case SyncServiceCSWeb:
        case SyncServiceFtp:
        case SyncServiceLocalFiles:
        case SyncServiceCustom:
        {
            return m_text;
        }

        case SyncServiceDropbox:
        {
            SyncConnectionString sync_connection_string = SyncConnectionString::CreateDropboxSyncConnectionString();

            if( m_dropboxLocalType == 1 )
            {
                sync_connection_string.SetProperty(SCSProperty::useLocal, SCSValue::true_);
            }

            else if( m_dropboxLocalType == 2 )
            {
                sync_connection_string.SetProperty(SCSProperty::useLocal, SCSValue::try_);
            }

            if( !m_text.empty() )
                sync_connection_string.SetProperty(SCSProperty::email, m_text);

            return sync_connection_string;
        }

        default:
        {
            return ReturnProgrammingError(SyncConnectionString());
        }
    }
}


SyncConnectionString SyncServiceSelectorDlg::ValidateSyncConnectionString(const bool save_url_to_registry/* = true*/) const
{
    SyncConnectionString sync_connection_string = GetSyncConnectionString(false);

    sync_connection_string.Validate(ConvertSyncServiceType(m_syncServiceType));

    if( save_url_to_registry )
        SaveUrlToRegistry(sync_connection_string);

    return sync_connection_string;
}


SyncConnectionString SyncServiceSelectorDlg::GetSyncConnectionString(const bool save_url_to_registry/* = true*/) const
{
     SyncConnectionString sync_connection_string = FromForm(std::nullopt);

    if( save_url_to_registry )
        SaveUrlToRegistry(sync_connection_string);

     return sync_connection_string;
}


void SyncServiceSelectorDlg::SaveUrlToRegistry(const SyncConnectionString& sync_connection_string)
{
     if( !sync_connection_string.IsDefined() )
         return;

     const std::string safe_sync_connection_string = sync_connection_string.ToSafeString();

     WinSettings::Write(WinSettings::Type::LastSyncConnectionString, safe_sync_connection_string);

     if( sync_connection_string.GetType() == SyncServiceType::CSWeb )
         WinSettings::Write(WinSettings::Type::LastCSWebUrl, safe_sync_connection_string);
}
