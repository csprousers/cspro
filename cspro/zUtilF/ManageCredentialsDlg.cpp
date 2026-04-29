#include "StdAfx.h"
#include "ManageCredentialsDlg.h"
#include <zToolsO/VariantVisitOverload.h>
#include <zUtilO/CredentialStore.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zMessageO/Messages.h>
#include <zDataO/CSWebRepositoryCacheCredential.h>
#include <zDataO/EncryptedSQLiteRepositoryCredential.h>
#include <zNetwork/CSWebUser.h>
#include <afxmenubutton.h>
#include <wincred.h>


BEGIN_MESSAGE_MAP(ManageCredentialsDlg, ResizableDlgEx)
    ON_NOTIFY(TVN_SELCHANGED, IDC_CREDENTIALS, OnCredentialSelectionChanged)
    ON_BN_CLICKED(IDC_CLEAR, OnClear)
    ON_BN_CLICKED(IDC_CLEAR_ALL, OnClearAll)
END_MESSAGE_MAP()


namespace
{
    constexpr std::wstring_view CredentialPrefixSync_sv       = L"CSPro_sync_";
    constexpr std::wstring_view CredentialPrefixData_sv       = L"CSPro_data_";
    constexpr std::wstring_view CredentialPrefixCSWebCache_sv = L"CSPro_csweb_cache";
    constexpr std::wstring_view CredentialPrefixLocation_sv   = L"CSPro_location";
}


ManageCredentialsDlg::ManageCredentialsDlg(CWnd* const pParent /* = nullptr*/)
    :   ResizableDlgEx(IDD_MANAGE_CREDENTIALS, pParent),
        m_populatingCredentialsTreeCtrl(false),
        m_clearAllButton(std::make_unique<CMFCMenuButton>())
{
    SerializeDialogSize("ManageCredentialsDlg");
}


ManageCredentialsDlg::~ManageCredentialsDlg()
{
}


void ManageCredentialsDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_CREDENTIALS, m_credentialsTreeCtrl);
    DDX_Control(pDX, IDC_DETAILS, m_detailsText);
    DDX_Control(pDX, IDC_CLEAR_ALL, *m_clearAllButton);
}


BOOL ManageCredentialsDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    // set up the Clear All button
    m_clearAllMenu.LoadMenu(IDR_CLEAR_CREDENTIALS);
    m_clearAllButton->m_hMenu = m_clearAllMenu.GetSubMenu(0)->GetSafeHmenu();

    // populate the credentials
    GetCredentials();
    PopulateCredentialsTree(nullptr);

    return result;
}


const wchar_t* ManageCredentialsDlg::ToString(const CredentialType credential_type, const bool for_details)
{
    constexpr const wchar_t* CredentialTypeStrings[][2] =
    {
        { L"Synchronization Services", L"Credentials for synchronization services." },
        { L"Encrypted Data",           L"Credentials for Encrypted CSPro DB data sources." },
        { L"CSWeb Data Cache",         L"Credentials for cached data from CSWeb data sources (used internally by CSPro)." },
        { L"Locations",                L"Cached locations used for mapping." }
    };

    ASSERT(static_cast<size_t>(credential_type) < _countof(CredentialTypeStrings));

    return CredentialTypeStrings[static_cast<size_t>(credential_type)][for_details ? 1 : 0];
}


struct ManageCredentialsDlg::Credential
{
    std::wstring target_name;
    std::string credential;

    CredentialType type;
    std::wstring display_name;
    std::string details;
};


void ManageCredentialsDlg::GetCredentials()
{
    DWORD number_credentials;
    PCREDENTIAL* win_credentials;

    if( CredEnumerate(L"CSPro*", 0, &number_credentials, &win_credentials) )
    {
        for( DWORD i = 0; i < number_credentials; ++i )
        {
            try
            {
                const PCREDENTIAL win_credential = win_credentials[i];

                std::unique_ptr<Credential> credential(new Credential
                {
                    win_credential->TargetName,
                    CredentialStore::ParseCredentialBlob(*win_credential)
                });

                if( SO::StartsWith(credential->target_name, CredentialPrefixSync_sv) )
                {
                    SetUpCredentialSync(*credential);
                }

                else if( SO::StartsWith(credential->target_name, CredentialPrefixData_sv) )
                {
                    SetUpCredentialData(*credential);
                }

                else if( SO::StartsWith(credential->target_name, CredentialPrefixCSWebCache_sv) )
                {
                    SetUpCredentialCSWebCache(*credential);
                }

                else if( SO::StartsWith(credential->target_name, CredentialPrefixLocation_sv) )
                {
                    SetUpCredentialLocation(*credential);
                }

                else
                {
                    ASSERT(false);
                    continue;
                }

                m_credentials[credential->type].emplace_back(std::move(credential));
            }
            catch(...) { ASSERT(false); }
        }
    }

    // sort the credentials by the display name
    for( auto& [type, credentials] : m_credentials )
    {
        std::sort(credentials.begin(), credentials.end(),
                  [&](const auto& c1, const auto& c2) { return ( SO::CompareNoCase(c1->display_name, c2->display_name) < 0 ); });
    }
}


void ManageCredentialsDlg::PopulateCredentialsTree(const Credential* const credential_to_select)
{
    m_populatingCredentialsTreeCtrl = true;

    m_credentialsTreeCtrl.DeleteAllItems();

    HTREEITEM tree_item_to_select = nullptr;

    for( auto& [type, credentials] : m_credentials )
    {
        TV_INSERTSTRUCT tvi { };
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvi.hInsertAfter = TVI_LAST;

        tvi.item.pszText = const_cast<wchar_t*>(ToString(type, false));
        tvi.hParent = m_credentialsTreeCtrl.InsertItem(&tvi);

        for( std::unique_ptr<Credential>& credential : credentials )
        {
            tvi.item.pszText = credential->display_name.data();
            tvi.item.lParam = reinterpret_cast<LPARAM>(credential.get());
            const HTREEITEM tree_item = m_credentialsTreeCtrl.InsertItem(&tvi);

            if( credential_to_select == credential.get() )
                tree_item_to_select = tree_item;
        }
    }

    TreeCtrlHelpers::ExpandAllNodes(m_credentialsTreeCtrl);

    m_populatingCredentialsTreeCtrl = false;

    // select a specific credential...
    if( tree_item_to_select != nullptr )
    {
        m_credentialsTreeCtrl.SelectItem(tree_item_to_select);
    }

    // ...or the first credential...
    else if( !m_credentials.empty() )
    {
        m_credentialsTreeCtrl.SelectItem(m_credentialsTreeCtrl.GetRootItem());
    }

    // ...or set the text that indicates that there are no credentials
    else
    {
        SetDetailsText();
    }
}


void ManageCredentialsDlg::SetUpCredentialSync(Credential& credential)
{
    credential.type = CredentialType::Sync;

    const std::wstring_view url_sv = std::wstring_view(credential.target_name).substr(CredentialPrefixSync_sv.length());

    try
    {
        const JsonNode json_node = Json::Parse(credential.credential);

        if( url_sv == L"DropboxV2" )
        {
            credential.display_name = L"Dropbox";
            credential.details = "Dropbox";
            SetUpCredentialSync_OAuth2Token(credential, json_node);
        }

        else
        {
            credential.display_name = url_sv;
            credential.details = "URL: " + TC::ToUtf8(url_sv);

            // FTP
            if( json_node.Contains(JK::username) )
            {
                credential.details.append("\n\nUsername: ").append(json_node.Get<std::string_view>(JK::username));
            }

            // CSWeb
            else if( json_node.Contains(JK::user) )
            {
                const CSWebUser user = json_node.Get<CSWebUser>(JK::user);
                credential.details.append("\n\nUser ID: ").append(user.id);
                credential.details.append("\n\nRole: ").append(user.role_name);
            }
        }
    }

    catch(...)
    {
        // default to simply showing the URL
        credential.display_name = url_sv;
        credential.details = "Synchronization service: " + TC::ToUtf8(url_sv);
    }
}


void ManageCredentialsDlg::SetUpCredentialSync_OAuth2Token(Credential& credential, const JsonNode& json_node)
{
    std::vector<std::string> emails;

    for( const JsonNode& oauth2_token_json_node : json_node.GetArray() )
        emails.emplace_back(oauth2_token_json_node.Get<std::string>(JK::email));

    if( emails.size() == 1 )
    {
        credential.display_name.append(L" (").append(TC::ToWide(emails.front())).append(L")");
        credential.details.append("\n\nEmail: ").append(emails.front());
    }

    else if( !emails.empty() )
    {
        credential.details.append("\n\nEmails:");

        for( const std::string& email : emails )
            credential.details.append("\n    ").append(email);
    }
}


void ManageCredentialsDlg::SetUpCredentialData(Credential& credential)
{
    const EncryptedSQLiteRepositoryCredential data_credential(credential.credential);

    credential.type = CredentialType::Data;

    credential.display_name = data_credential.GetFilePath().empty() ? credential.target_name.substr(CredentialPrefixData_sv.length()) :
                                                                      TC::ToWide(Path::GetFilename(data_credential.GetFilePath()));

    credential.details = "Credential date: " + DateTime::LocalDateTimeString(static_cast<int64_t>(data_credential.GetStorageTimestamp()));

    if( !data_credential.GetDictionaryName().empty() )
        credential.details.append("\n\nDictionary name: ").append(data_credential.GetDictionaryName());

    if( !data_credential.GetFilePath().empty() )
        credential.details.append("\n\nData file path: ").append(data_credential.GetFilePath());
}


void ManageCredentialsDlg::SetUpCredentialCSWebCache(Credential& credential)
{
    credential.type = CredentialType::CSWebCache;
    credential.display_name = L"Credentials";

    try
    {
        const JsonNode json_node = Json::Parse(credential.credential);
        const std::vector<CSWebRepositoryCacheCredential> csweb_cache_credentials = json_node.GetArray().GetVector<CSWebRepositoryCacheCredential>();

        // list details about each credential
        for( const CSWebRepositoryCacheCredential& csweb_cache_credential : csweb_cache_credentials )
        {
            if( !credential.details.empty() )
                credential.details.append("\n\n");

            credential.details.append("Server device ID: ").append(csweb_cache_credential.server_device_id)
                              .append("\nUser ID: ").append(csweb_cache_credential.user.id)
                              .append("\nRole: ").append(csweb_cache_credential.user.role_name)
                              .append("\nDictionary name: ").append(csweb_cache_credential.dictionary_name)
                              .append("\nCache file path: ").append(csweb_cache_credential.cache_file_path)
                              .append("\nEncrypted cache: ").append(csweb_cache_credential.cache_password.has_value() ? "true" : "false");
        }
    }
    catch(...) { ASSERT(false); }
}


void ManageCredentialsDlg::SetUpCredentialLocation(Credential& credential)
{
    credential.type = CredentialType::Location;
    credential.display_name = L"Approximate Location";

    const JsonNode json_node = Json::Parse(credential.credential);

    credential.details = FormatText("Cache date: %s\n\nLatitude: %0.6f\nLongitude: %0.6f",
                                    DateTime::LocalDateTimeString(json_node.Get<int64_t>(JK::timestamp)).c_str(),
                                    json_node.Get<double>(JK::latitude),
                                    json_node.Get<double>(JK::longitude));
}


std::variant<std::monostate,
             ManageCredentialsDlg::CredentialType,
             const ManageCredentialsDlg::Credential*> ManageCredentialsDlg::GetCurrentSelection() const
{
    ASSERT(!m_populatingCredentialsTreeCtrl);

    const HTREEITEM selected_node = m_credentialsTreeCtrl.GetSelectedItem();

    if( selected_node == nullptr )
        return std::monostate();

    LPARAM selected_data = m_credentialsTreeCtrl.GetItemData(selected_node);

    if( selected_data != 0 )
        return reinterpret_cast<const Credential*>(selected_data);

    // if here, the selected item is a credential heading so get the first credential to find the type
    selected_data = m_credentialsTreeCtrl.GetItemData(m_credentialsTreeCtrl.GetChildItem(selected_node));

    if( selected_data != 0 )
        return reinterpret_cast<const Credential*>(selected_data)->type;

    return ReturnProgrammingError(std::monostate());
}


void ManageCredentialsDlg::SetDetailsText()
{
    std::visit(
        overload
        {
            [&](std::monostate)
            {
                m_detailsText.SetWindowText(TC::ToWide(MGF::GetMessageText(94331, "There are no saved credentials.").GetString()).c_str());
            },
            [&](const CredentialType credential_type)
            {
                m_detailsText.SetWindowText(ToString(credential_type, true));
            },
            [&](const Credential* const credential)
            {
                m_detailsText.SetWindowText(TC::ToWide(credential->details).c_str());
            }

        }, GetCurrentSelection());
}


void ManageCredentialsDlg::OnCredentialSelectionChanged(NMHDR* const /*pNMHDR*/, LRESULT* const pResult)
{
    if( !m_populatingCredentialsTreeCtrl )
        SetDetailsText();

    *pResult = 0;
}


void ManageCredentialsDlg::OnClear()
{
    std::visit(
        overload
        {
            [&](std::monostate)                       { },
            [&](const CredentialType credential_type) { ClearCredentials(credential_type); },
            [&](const Credential* const credential)   { ClearCredential(credential); }

        }, GetCurrentSelection());
}


void ManageCredentialsDlg::OnClearAll()
{
    switch( m_clearAllButton->m_nMenuResult )
    {
        case ID_CREDENTIALS_CLEAR_SYNC:
            ClearCredentials(CredentialType::Sync);
            break;

        case ID_CREDENTIALS_CLEAR_DATA:
            ClearCredentials(CredentialType::Data);
            break;

        default:
            ASSERT(m_clearAllButton->m_nMenuResult == 0);
            ClearCredentialsAll();
            break;
    }
}


bool ManageCredentialsDlg::ConfirmClear(const size_t number_credentials)
{
    if( number_credentials == 0 )
        return false;

    const SharableString formatter = MGF::GetMessageText(94332, "Are you sure that you want to delete %d credential(s)?");
    const std::string message = FormatText(formatter->c_str(), static_cast<int>(number_credentials));

    return ( AfxMessageBox(message, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES );
}


void ManageCredentialsDlg::ClearCredentialsAll()
{
    size_t number_credentials = 0;

    for( const auto& [type, credentials] : m_credentials )
        number_credentials += credentials.size();

    if( !ConfirmClear(number_credentials) )
        return;

    for( const auto& [type, credentials] : m_credentials )
    {
        for( const std::unique_ptr<Credential>& credential : credentials )
            CredDelete(credential->target_name.c_str(), CRED_TYPE_GENERIC, 0);
    }

    m_credentials.clear();

    PopulateCredentialsTree(nullptr);
}


void ManageCredentialsDlg::ClearCredentials(const CredentialType credential_type)
{
    const auto& lookup = m_credentials.find(credential_type);

    if( lookup == m_credentials.cend() || !ConfirmClear(lookup->second.size()) )
        return;

    for( const std::unique_ptr<Credential>& credential : lookup->second )
        CredDelete(credential->target_name.c_str(), CRED_TYPE_GENERIC, 0);

    m_credentials.erase(lookup);

    PopulateCredentialsTree(nullptr);
}


void ManageCredentialsDlg::ClearCredential(const Credential* const credential)
{
    std::vector<std::unique_ptr<Credential>>& credentials = m_credentials[credential->type];
    auto lookup = std::find_if(credentials.begin(), credentials.end(),
                               [&](const std::unique_ptr<Credential>& this_credential) { return ( credential == this_credential.get() ); });

    if( lookup == credentials.cend() )
    {
        ASSERT(false);
        return;
    }

    CredDelete(credential->target_name.c_str(), CRED_TYPE_GENERIC, 0);

    const Credential* credential_to_select;

    // if there are no more credentials of this type, remove the type
    if( credentials.size() == 1 )
    {
        m_credentials.erase(credential->type);
        credential_to_select = nullptr;
    }

    // otherwise select the next credential of this type
    else
    {
        lookup = credentials.erase(lookup);

        credential_to_select = ( lookup != credentials.end() ) ? lookup->get() :
                               ( !credentials.empty() )        ? credentials.back().get() :
                                                                 nullptr;
    }

    PopulateCredentialsTree(credential_to_select);
}
