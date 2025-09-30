#include "StdAfx.h"
#include "ManageFilesDlg.h"
#include "AppFileTypeImageList.h"
#include "ApplicationPropertiesPageDlg.h"
#include "AppMessageFilePropertiesPageDlg.h"
#include "CodeFilePropertiesPageDlg.h"
#include "DictionaryPropertiesPageDlg.h"
#include "FormFilePropertiesPageDlg.h"
#include "NewFileCreator.h"
#include "ReportPropertiesPageDlg.h"
#include "ResourcePropertiesPageDlg.h"
#include "TableSpecPropertiesPageDlg.h"
#include "TextDisplayPageDlg.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/TextSourceEditable.h>
#include <zUtilO/TextSourceExternal.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilF/ChoiceDlg.h>
#include <zUToolO/resource_shared.h>
#include <zMessageO/ExceptionThrowingSystemMessageIssuer.h>
#include <zTableO/Table.h>
#include <zLogicO/ReservedWords.h>
#include <zEngineO/ApplicationLoader.h>
#include <afxpriv.h>


BEGIN_MESSAGE_MAP(ManageFilesDlg, CTreePropertiesDlg)
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_COMMAND(ID_HELP, OnHelp)
    ON_BN_CLICKED(IDC_ADD_FILE, OnAddFile)
    ON_BN_CLICKED(IDC_REMOVE_FILE, OnRemoveFile)
END_MESSAGE_MAP()


ManageFilesDlg::ManageFilesDlg(Application& application, ApplicationLoader& application_loader,
                               std::unique_ptr<AppFileTypeImageList> app_file_type_image_list, CWnd* const pParent/* = nullptr*/)
    :   CTreePropertiesDlg(TC::ToWide("Manage Files for Application: " + application_loader.GetApplication()->GetName()),
                           IDD_MANAGE_FILES, false, pParent),
        m_application(application),
        m_applicationLoader(application_loader),
        m_showNamesInTree(true),
        m_appFileTypeImageList(std::move(app_file_type_image_list)),
        m_initialPageToSelect(nullptr)
{
    ASSERT(m_appFileTypeImageList != nullptr);

    BuildTree();
}


ManageFilesDlg::~ManageFilesDlg()
{
}


void ManageFilesDlg::InitiallySelectPath(const std::string& path)
{
    // when called via the Files tree -> Properties, we can select a certain path when the dialog starts
    const auto& lookup = std::find_if(m_managePages.cbegin(), m_managePages.cend(),
                                      [&](const ManagePage& manage_page) { return SO::EqualsNoCase(path, manage_page.path); });

    if( lookup != m_managePages.cend() )
    {
        m_initialPageToSelect = lookup->dialog.get();
    }

    else
    {
        ASSERT(false);
    }
}


void ManageFilesDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_ADD_FILE, m_addFileButton);
}


BOOL ManageFilesDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // get the dialog size to prevent the user from sizing it smaller than the designed size
    CRect dialog_rect;
    GetWindowRect(dialog_rect);
    m_initialWindowSize = dialog_rect.Size();

    // use icons in the tree control
    m_treeCtrl.SetImageList(m_appFileTypeImageList.get(), TVSIL_NORMAL);

    // set up the add file button's menu
    m_addFileMenu.LoadMenu(IDR_ADD_FILE);

    CMenu* const popup_menu = m_addFileMenu.GetSubMenu(0);

    if( m_application.GetEngineAppType() != EngineAppType::Entry )
    {
        // remove both the Form File entry as well as the separator
        const int form_file_position = WindowHelpers::GetMenuItemPositionByCommand(*popup_menu, ID_ADD_FORM_FILE);

        ASSERT(form_file_position != -1);
        popup_menu->RemoveMenu(form_file_position, MF_BYPOSITION);

        ASSERT(popup_menu->GetMenuItemID(form_file_position) == 0);
        popup_menu->RemoveMenu(form_file_position, MF_BYPOSITION);
    }

    // disable the working storage dictionary option when one exists at the default file path
    if( UsesDefaultWorkingStoringDictionary() )
        popup_menu->EnableMenuItem(ID_ADD_DICTIONARY_WORKING, MF_BYCOMMAND | MF_GRAYED);

    m_addFileButton.m_hMenu = popup_menu->GetSafeHmenu();

    // expand all tree nodes and then select either a specific path, or the first entry in the tree
    TreeCtrlHelpers::ExpandAllNodes(m_treeCtrl);

    const HTREEITEM node_to_select = ( m_initialPageToSelect != nullptr ) ? FindItemByPage(m_initialPageToSelect) :
                                                                            m_treeCtrl.GetRootItem();
    m_treeCtrl.SelectItem(node_to_select);

    return TRUE;
}


void ManageFilesDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    __super::OnGetMinMaxInfo(lpMMI);

    lpMMI->ptMinTrackSize.x = std::max(lpMMI->ptMinTrackSize.x, m_initialWindowSize.cx);
    lpMMI->ptMinTrackSize.y = std::max(lpMMI->ptMinTrackSize.y, m_initialWindowSize.cy);
}


void ManageFilesDlg::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    SizeCurrentDialogToMatchPagePlaceholder();
}


void ManageFilesDlg::SizeCurrentDialogToMatchPagePlaceholder()
{
    if( m_pCurrDlg == nullptr )
        return;

    CWnd* const property_page_wnd = GetDlgItem(IDC_PAGE_PLACEHOLDER);

    if( property_page_wnd == nullptr )
        return;

    CRect rect;
    property_page_wnd->GetWindowRect(&rect);

    m_pCurrDlg->SetWindowPos(nullptr, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


BOOL ManageFilesDlg::PreTranslateMessage(MSG* const pMsg)
{
    // override Ctrl+T (to toggle the tree control)
    if( pMsg->message == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0 && std::toupper(pMsg->wParam) == 'T' )
    {
        m_showNamesInTree = !m_showNamesInTree;
        m_treeCtrl.Invalidate();
        return TRUE;
    }

    return __super::PreTranslateMessage(pMsg);
}


BOOL ManageFilesDlg::OnNotify(const WPARAM wParam, const LPARAM lParam, LRESULT* const pResult)
{
    NMHDR* const pNMHDR = reinterpret_cast<NMHDR*>(lParam);

    // allow the displaying of names or paths in the tree control
    if( pNMHDR->code == TVN_GETDISPINFO )
    {
        NMTVDISPINFO* const tree_view_display_info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);

        if( ( tree_view_display_info->item.mask & TVIF_TEXT ) != 0 )
        {
            const ManagePage* const manage_page = GetManagePage(tree_view_display_info->item.hItem);
            const std::wstring& text = ( manage_page == nullptr ) ? SO::Empty_wstring :
                                       ( m_showNamesInTree )      ? manage_page->tree_name :
                                                                    manage_page->tree_path;

            wcsncpy_s(tree_view_display_info->item.pszText, tree_view_display_info->item.cchTextMax, text.c_str(), _TRUNCATE);
        }

        *pResult = 0;
        return TRUE;
    }

    return __super::OnNotify(wParam, lParam, pResult);
}


void ManageFilesDlg::OnHelp()
{
    // show the help for the currently-selected properties page rather than the generic Manage Files help
    const ManagePage* const manage_page = GetManagePage(m_pCurrDlg);

    if( manage_page != nullptr )
    {
        HtmlHelp(HID_BASE_RESOURCE + manage_page->dialog_context_id);
    }

    else
    {
        __super::OnHelp();
    }
}


void ManageFilesDlg::ResizeDlg(const CRect& /*new_page_rect*/)
{
    // the properties dialog's size should be modified in the resource editor
}


template<typename T>
CDialog* ManageFilesDlg::AddManagePage(std::shared_ptr<T> dialog, const std::variant<CDialog*, AppFileType> parent_page_or_type,
                                       const AppFileType app_file_type, const std::string_view name_sv, const std::string& path,
                                       const bool can_remove_file)
{
    ASSERT(dialog != nullptr);

    const int dialog_context_id = T::GetDialogTemplateId();
    dialog->Create(dialog_context_id, this);

    CDialog* parent_page;
    CDialog* insert_after_page;

    if( std::holds_alternative<CDialog*>(parent_page_or_type) )
    {
        parent_page = std::get<CDialog*>(parent_page_or_type);
        insert_after_page = nullptr;
    }

    else
    {
        ASSERT(can_remove_file);

        parent_page = nullptr;
        insert_after_page = GetManagePageInsertionPoint(std::get<AppFileType>(parent_page_or_type));
    }

    const ManagePage& manage_page = m_managePages.emplace_back(
        ManagePage
        {
            dialog,
            dialog_context_id,
            parent_page,
            app_file_type,
            path,
            can_remove_file,
            TC::ToWide(std::string(name_sv)),
            TC::ToWide(GetRelativePathForDisplay(m_application.GetApplicationFilePath(), path))
        });

    TreePropertiesPageValidator* page_validator;

    if constexpr(std::is_base_of_v<TreePropertiesPageValidator, T>)
    {
        page_validator = dialog.get();
    }

    else
    {
        page_validator = nullptr;
    }

    CTreePropertiesDlg::AddPage(dialog.get(), manage_page.tree_name.c_str(), parent_page, insert_after_page, page_validator);

    return dialog.get();
}


CDialog* ManageFilesDlg::GetManagePageInsertionPoint(const AppFileType app_file_type) const
{
    int app_file_type_order = GetTreeOrder(app_file_type);

    for( ; app_file_type_order >= 1; --app_file_type_order )
    {
        // search for this type in reverse order
        const auto& lookup = std::find_if(m_managePages.crbegin(), m_managePages.crend(),
            [&](const ManagePage& manage_page)
            {
                return ( manage_page.can_remove_file &&
                         app_file_type_order == GetTreeOrder(manage_page.app_file_type) );
            });

        if( lookup != m_managePages.crend() )
            return lookup->dialog.get();
    }

    // if not already found, this is first removable file of this type, having a lower tree order
    // than all other removable files, so insert it after the application
    const auto& application_lookup = std::find_if(m_managePages.cbegin(), m_managePages.cend(),
                                                  [&](const ManagePage& manage_page) { return IsApplicationType(manage_page.app_file_type); });

    return ( application_lookup != m_managePages.cend() ) ? application_lookup->dialog.get() :
                                                            ReturnProgrammingError(nullptr);
}


ManageFilesDlg::ManagePage* ManageFilesDlg::GetManagePage(const CDialog* dialog)
{
    const auto& lookup = std::find_if(m_managePages.begin(), m_managePages.end(),
                                      [&](const ManagePage& manage_page) { return ( dialog == manage_page.dialog.get() ); });

    return ( lookup != m_managePages.end() ) ? &(*lookup) :
                                               nullptr;
}


ManageFilesDlg::ManagePage* ManageFilesDlg::GetManagePage(const HTREEITEM hItem)
{
    return GetManagePage(reinterpret_cast<const CDialog*>(m_treeCtrl.GetItemData(hItem)));
}


void ManageFilesDlg::OnModifyTreeItemBeforeInsert(CDialog* const dlg, TVITEMW& item)
{
    const ManagePage* const manage_page = GetManagePage(dlg);

    const int image_index = ( manage_page != nullptr ) ? m_appFileTypeImageList->GetImageIndex(manage_page->app_file_type, manage_page->path) :
                                                         ReturnProgrammingError(-1);

    item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.iImage = image_index;
    item.iSelectedImage = image_index;

    item.pszText = static_cast<LPTSTR>(LPSTR_TEXTCALLBACK);
}


bool ManageFilesDlg::OnPageChange(CDialog* /*old_page*/, CDialog* const new_page)
{
    const ManagePage* const manage_page = ( new_page != nullptr ) ? GetManagePage(new_page) :
                                                                    nullptr;

    if( manage_page == nullptr )
        return false;

    WindowsUtf8::SetText(this, IDC_PATH, manage_page->path);
    WindowsUtf8::SetText(this, IDC_TYPE, ToString(manage_page->app_file_type));

    GetDlgItem(IDC_REMOVE_FILE)->EnableWindow(manage_page->can_remove_file);

    SizeCurrentDialogToMatchPagePlaceholder();

    return false;
}


void ManageFilesDlg::BuildTree()
{
    ASSERT(m_buildTreeErrors.empty());

    // the application and its dependencies
    BuildTreeApplication();

    // external form files
    if( m_application.GetEngineAppType() == EngineAppType::Entry )
        BuildTreeFormFile(nullptr, AppFileType::Form, false);

    // external dictionaries
    for( const std::string& dictionary_file_path : m_application.GetExternalDictionaryFilePaths() )
    {
        BuildTreeDictionary(nullptr,
                            GetOrCreateDictionaryDescription(dictionary_file_path, SO::Empty_string, DictionaryType::External),
                            nullptr,
                            true);
    }

    // external code files
    BuildTreeCode(nullptr, false);

    // external message files
    BuildTreeMessages(nullptr, false);

    // reports
    for( const ReportFile& report_file : m_application.GetReportFiles() )
        BuildTreeReport(nullptr, report_file);

    // resources
    for( const AppResource& resource : m_application.GetResources() )
        BuildTreeResource(nullptr, resource);

    if( !m_buildTreeErrors.empty() )
    {
        ErrorMessage::Display(SO::CreateSingleString(m_buildTreeErrors, SO::Newline_crlf_sv));
    }
}


void ManageFilesDlg::BuildTreeApplication()
{
    // the application
    CDialog* const application_dialog = AddManagePage(std::make_shared<ApplicationPropertiesPageDlg>(*this, m_application),
                                                      nullptr,
                                                      m_application.GetApplicationAppFileType(),
                                                      m_application.GetName(),
                                                      m_application.GetApplicationFilePath(),
                                                      false);

    // forms
    if( m_application.GetEngineAppType() == EngineAppType::Entry )
    {
        ASSERT(!m_application.GetFormFilePaths().empty());
        BuildTreeFormFile(application_dialog, AppFileType::Form, true);
    }

    // orders
    else if( m_application.GetEngineAppType() == EngineAppType::Batch )
    {
        ASSERT(m_application.GetFormFilePaths().size() == 1);
        BuildTreeFormFile(application_dialog, AppFileType::Order, true);
    }

    // tab specs
    else if( m_application.GetEngineAppType() == EngineAppType::Tabulation )
    {
        BuildTreeTableSpec(application_dialog);
    }

    else
    {
        ASSERT(false);
    }

    // main logic
    BuildTreeCode(application_dialog, true);

    // main message file
    BuildTreeMessages(application_dialog, true);

    // question text
    if( !m_application.GetQuestionTextFilePath().empty() )
    {
        ASSERT(m_application.GetEngineAppType() == EngineAppType::Entry);

        AddManagePage(std::make_shared<TextDisplayPageDlg>("There are no properties that apply to a Question Text file."),
                      application_dialog,
                      AppFileType::QuestionText,
                      PortableFunctions::PathGetFilename(m_application.GetQuestionTextFilePath()),
                      m_application.GetQuestionTextFilePath(),
                      false);
    }
}


void ManageFilesDlg::BuildTreeFormFile(CDialog* const parent_page, const AppFileType app_file_type, const bool main_form_file)
{
    const std::vector<std::string>& form_file_paths = m_application.GetFormFilePaths();
    ASSERT(!form_file_paths.empty());

    const size_t count = main_form_file ? 1 : form_file_paths.size();

    for( size_t i = main_form_file ? 0 : 1; i < count; ++i )
    {
        const std::string& form_file_path = form_file_paths[i];

        try
        {
            std::shared_ptr<CDEFormFile> form_file = m_applicationLoader.GetFormFile(form_file_path);
            ASSERT(form_file != nullptr);

            BuildTreeFormFile(parent_page, app_file_type, main_form_file, form_file_path, std::move(form_file), nullptr);
        }

        catch( const CSProException& exception )
        {
            m_buildTreeErrors.emplace_back(FormatText("Error loading '%s': %s",
                                                      PortableFunctions::PathGetFilename(form_file_path).c_str(),
                                                      exception.what()));
        }
    }
}


CDialog* ManageFilesDlg::BuildTreeFormFile(std::variant<CDialog*, AppFileType> parent_page_or_type, const AppFileType app_file_type, const bool main_form_file,
                                           const std::string& form_file_path,
                                           const std::variant<std::shared_ptr<CDEFormFile>, std::shared_ptr<const CDEFormFile>> form_file,
                                           std::shared_ptr<const CDataDict> dictionary)
{
    const CDEFormFile* const form_file_ptr = std::visit([](const auto& form_file) -> const CDEFormFile* { return form_file.get(); }, form_file);
    ASSERT(form_file_ptr != nullptr);

    CDialog* const form_file_dialog = AddManagePage(std::make_shared<FormFilePropertiesPageDlg>(*this, form_file, form_file_path),
                                                    parent_page_or_type,
                                                    app_file_type,
                                                    UTF8_TODO::GetUtf8(form_file_ptr->GetName()),
                                                    form_file_path,
                                                    !main_form_file);

    // the form file's dictionary
    const DictionaryType dictionary_type_if_creating = main_form_file ? DictionaryType::Input :
                                                                        DictionaryType::External;

    BuildTreeDictionary(form_file_dialog,
                        GetOrCreateDictionaryDescription(UTF8_TODO::GetUtf8(form_file_ptr->GetDictionaryFilename()), form_file_path, dictionary_type_if_creating),
                        std::move(dictionary),
                        false);

    return form_file_dialog;
}


void ManageFilesDlg::BuildTreeTableSpec(CDialog* const parent_page)
{
    ASSERT(m_application.GetTableSpecFilePaths().size() == 1);

    for( const std::string& table_spec_file_path : m_application.GetTableSpecFilePaths() )
    {
        try
        {
            const std::shared_ptr<CTabSet> table_spec = m_applicationLoader.GetTableSpec(table_spec_file_path);

            CDialog* const table_spec_dialog = AddManagePage(std::make_shared<TableSpecPropertiesPageDlg>(*this, table_spec, table_spec_file_path),
                                                             parent_page,
                                                             AppFileType::TableSpec,
                                                             UTF8_TODO::GetUtf8(table_spec->GetName()),
                                                             table_spec_file_path,
                                                             false);

            // the table spec's dictionary
            BuildTreeDictionary(table_spec_dialog,
                                GetOrCreateDictionaryDescription(UTF8_TODO::GetUtf8(table_spec->GetDictFile()), table_spec_file_path, DictionaryType::Input),
                                nullptr,
                                false);
        }

        catch( const CSProException& exception )
        {
            m_buildTreeErrors.emplace_back(FormatText("Error loading '%s': %s",
                                                      PortableFunctions::PathGetFilename(table_spec_file_path).c_str(),
                                                      exception.what()));
        }
    }
}


CDialog* ManageFilesDlg::BuildTreeDictionary(const std::variant<CDialog*, AppFileType> parent_page_or_type, DictionaryDescription dictionary_description,
                                             std::shared_ptr<const CDataDict> dictionary, const bool can_remove_file)
{
    std::string dictionary_file_path = dictionary_description.GetDictionaryFilePath();
    ASSERT(!dictionary_file_path.empty());

    if( dictionary_description.GetDictionaryType() == DictionaryType::Unknown )
    {
        ASSERT(false);
        dictionary_description.SetDictionaryType(DictionaryType::External);
    }

    ASSERT(can_remove_file || ( ( dictionary_description.GetDictionaryType() == DictionaryType::Input ) ||
                              ( ( dictionary_description.GetDictionaryType() == DictionaryType::External && !dictionary_description.GetParentFilePath().empty() ) ) ));

    try
    {
        if( dictionary == nullptr )
        {
            dictionary = m_applicationLoader.GetDictionary(dictionary_file_path);
            ASSERT(dictionary != nullptr);
        }

        return AddManagePage(std::make_shared<DictionaryPropertiesPageDlg>(*this, std::move(dictionary_description), dictionary),
                             parent_page_or_type,
                             AppFileType::Dictionary,
                             dictionary->GetName(),
                             std::move(dictionary_file_path),
                             can_remove_file);
    }

    catch( const CSProException& exception )
    {
        m_buildTreeErrors.emplace_back(FormatText("Error loading '%s': %s",
                                                  PortableFunctions::PathGetFilename(dictionary_file_path).c_str(),
                                                  exception.what()));

        return nullptr;
    }
}


void ManageFilesDlg::BuildTreeCode(CDialog* const parent_page, const bool main_logic)
{
    for( const CodeFile& code_file : m_application.GetCodeFiles() )
    {
        if( main_logic == code_file.IsLogicMain() )
            BuildTreeCode(parent_page, code_file);
    }
}


CDialog* ManageFilesDlg::BuildTreeCode(const std::variant<CDialog*, AppFileType> parent_page_or_type, const CodeFile& code_file)
{
    return AddManagePage(std::make_shared<CodeFilePropertiesPageDlg>(code_file),
                         parent_page_or_type,
                         AppFileType::Code,
                         PortableFunctions::PathGetFilename(code_file.GetFilePath()),
                         code_file.GetFilePath(),
                         !code_file.IsLogicMain());
}


void ManageFilesDlg::BuildTreeMessages(CDialog* const parent_page, const bool main_message_file)
{
    const std::vector<AppMessageFile>& app_message_files = m_application.GetMessageFiles();

    if( app_message_files.empty() )
        return;

    const size_t count = main_message_file ? 1 : app_message_files.size();

    for( size_t i = main_message_file ? 0 : 1; i < count; ++i )
        BuildTreeMessages(parent_page, app_message_files[i], main_message_file);
}


CDialog* ManageFilesDlg::BuildTreeMessages(const std::variant<CDialog*, AppFileType> parent_page_or_type, const AppMessageFile& app_message_file, const bool main_message_file)
{
    return AddManagePage(std::make_shared<AppMessageFilePropertiesPageDlg>(app_message_file, main_message_file),
                         parent_page_or_type,
                         AppFileType::Message,
                         PortableFunctions::PathGetFilename(app_message_file.GetFilePath()),
                         app_message_file.GetFilePath(),
                         !main_message_file);
}


CDialog* ManageFilesDlg::BuildTreeReport(const std::variant<CDialog*, AppFileType> parent_page_or_type, const ReportFile& report_file)
{
    return AddManagePage(std::make_shared<ReportPropertiesPageDlg>(*this, report_file),
                         parent_page_or_type,
                         AppFileType::Report,
                         report_file.GetName(),
                         report_file.GetFilePath(),
                         true);
}


CDialog* ManageFilesDlg::BuildTreeResource(const std::variant<CDialog*, AppFileType> parent_page_or_type, const AppResource& resource)
{
    return AddManagePage(std::make_shared<ResourcePropertiesPageDlg>(resource),
                         parent_page_or_type,
                         AppFileType::Resource,
                         PortableFunctions::PathGetFilename(resource.GetPath()),
                         resource.GetPath(),
                         true);
}


void ManageFilesDlg::OnAddFile()
{
    switch( m_addFileButton.m_nMenuResult )
    {
        case ID_ADD_DICTIONARY_EXTERNAL: return OnAddDictionaryExternal();
        case ID_ADD_DICTIONARY_WORKING:  return OnAddDictionaryWorking();
        case ID_ADD_FORM_FILE:           return OnAddFormFile();
        case ID_ADD_CODE:                return OnAddCode();
        case ID_ADD_MESSAGES:            return OnAddMessages();
        case ID_ADD_REPORT:              return OnAddReport();
        case ID_ADD_RESOURCE_DIRECTORY:  return OnAddResourceDirectory();
        case ID_ADD_RESOURCE_FILE:       return OnAddResourceFile();
        default:                         ASSERT(false);
    }
}


void ManageFilesDlg::OnRemoveFile()
{
    auto manage_page_lookup = std::find_if(m_managePages.begin(), m_managePages.end(),
                                           [&](const ManagePage& manage_page) { return ( m_pCurrDlg == manage_page.dialog.get() ); });
    ASSERT(manage_page_lookup != m_managePages.end());
    ASSERT(manage_page_lookup->can_remove_file);

    const AppFileType app_file_type = manage_page_lookup->app_file_type;

    switch( app_file_type )
    {
        case AppFileType::Form:
            m_application.DropForm(manage_page_lookup->path);
            m_application.DropDictionaryDescription(manage_page_lookup->path, true);
            break;

        case AppFileType::Dictionary:
            OnRemoveDictionary(*manage_page_lookup);
            break;

        case AppFileType::Code:
            m_application.DropCodeFile(manage_page_lookup->path);
            break;

        case AppFileType::Message:
            m_application.DropMessageFile(manage_page_lookup->path);
            break;

        case AppFileType::Report:
            m_application.DropReport(manage_page_lookup->path);
            break;

        case AppFileType::Resource:
            m_application.DropResource(manage_page_lookup->path);
            break;

        default:
            ASSERT(false);
            return;
    }

    // keep track of what is removed (including if a file was added and then removed)
    const auto& file_added_lookup = std::find_if(m_filesAdded.cbegin(), m_filesAdded.cend(),
        [&](const std::tuple<std::string, AppFileType>& path_and_app_file_type)
        {
            return ( std::get<1>(path_and_app_file_type) == app_file_type &&
                     SO::EqualsNoCase(std::get<0>(path_and_app_file_type), manage_page_lookup->path) );
        });

    if( file_added_lookup != m_filesAdded.cend() )
    {
        m_filesAdded.erase(file_added_lookup);
    }

    else
    {
        m_filesRemoved.emplace_back(manage_page_lookup->path, app_file_type);
    }

    // remove the entry and select whatever takes its place
    const HTREEITEM node_to_remove = FindItemByPage(manage_page_lookup->dialog.get());
    ASSERT(node_to_remove != nullptr);

    HTREEITEM node_to_select = m_treeCtrl.GetNextItem(node_to_remove, TVGN_NEXT);

    if( node_to_select == nullptr )
        node_to_select = m_treeCtrl.GetNextItem(node_to_remove, TVGN_PREVIOUS);

    CDialog* const page_to_select = ( node_to_select != nullptr ) ? reinterpret_cast<CDialog*>(m_treeCtrl.GetItemData(node_to_select)) :
                                                                    ReturnProgrammingError(m_managePages.front().dialog.get());

    SetPage(page_to_select);

    m_treeCtrl.DeleteItem(node_to_remove);

    // remove the pages for this entry and any children
    const CDialog* const this_entry_page = manage_page_lookup->dialog.get();

    do
    {
        ASSERT(page_to_select != manage_page_lookup->dialog.get());

        m_removedDialogs.emplace_back(std::move(manage_page_lookup->dialog));
        manage_page_lookup = m_managePages.erase(manage_page_lookup);

    } while( manage_page_lookup != m_managePages.end() && manage_page_lookup->parent_page == this_entry_page );
}


std::unique_ptr<OpenFileDlg> ManageFilesDlg::CreateOpenFileDlg(const wchar_t* const title, const bool allow_create, const bool allow_multi_select,
                                                               const char* const default_extension, std::wstring filter)
{
    const DWORD flags = allow_create ? ( OFN_HIDEREADONLY | OFN_CREATEPROMPT ) :
                                       FileDlg::DefaultOpenFlags;

    const std::string initial_directory = PortableFunctions::PathGetDirectory(m_application.GetApplicationFilePath());

    filter.append(L"All Files (*.*)|*.*||");

    auto open_file_dlg = std::make_unique<OpenFileDlg>(flags,
                                                       ( default_extension != nullptr ) ? FileDlg::StringType(std::string_view(default_extension)) :
                                                                                          FileDlg::StringType(),
                                                       initial_directory,
                                                       filter.c_str(),
                                                       this);
    open_file_dlg->SetTitle(title);

    if( allow_multi_select )
        open_file_dlg->SetMultiSelectBuffer();

    return open_file_dlg;
}


void ManageFilesDlg::OnAddFormFile()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select One or More Form Files", false, true,
                                                                         FileExtensions::Form, L"Data Entry Form Files (*.fmf)|*.fmf||");

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        // only load form files that aren't already added
        if( ContainsStringInVectorNoCase(m_application.GetFormFilePaths(), file_path) )
            continue;

        try
        {
            std::shared_ptr<const CDEFormFile> form_file = NewFileCreator::GetFormFile(file_path);
            std::shared_ptr<const CDataDict> dictionary = NewFileCreator::GetDictionary(UTF8_TODO::GetUtf8(form_file->GetDictionaryFilename()));

            // make sure the dictionary isn't in use elsewhere
            CheckDictionaryIsUnused(UTF8_TODO::GetUtf8(form_file->GetDictionaryFilename()));

            // only add form files where the form file and its dictionary have unique names
            ValidateName(UTF8_TODO::GetUtf8(form_file->GetName()), SO::Empty_string);
            ValidateName(dictionary->GetName(), SO::Empty_string);

            m_application.AddForm(file_path);
            m_application.AddDictionaryDescription(DictionaryDescription(file_path, UTF8_TODO::GetUtf8(form_file->GetDictionaryFilename()), DictionaryType::External));

            // keep track of what is added
            m_filesAdded.emplace_back(file_path, AppFileType::Form);

            last_added_dialog = BuildTreeFormFile(AppFileType::Form, AppFileType::Form, false, file_path, std::move(form_file), std::move(dictionary));

            // expand the node so the dictionary is shown
            const HTREEITEM node_to_expand = FindItemByPage(last_added_dialog);
            m_treeCtrl.Expand(node_to_expand, TVE_EXPAND);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("Error adding form file '%s':\n\n%s",
                                             file_path.c_str(), exception.what()));
        }
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


DictionaryDescription ManageFilesDlg::GetOrCreateDictionaryDescription(const std::string& dictionary_file_path, const std::string& parent_file_path,
                                                                       const DictionaryType dictionary_type_if_creating) const
{
    const DictionaryDescription* const current_dictionary_description = m_application.GetDictionaryDescription(dictionary_file_path, parent_file_path);

    return ( current_dictionary_description != nullptr ) ? *current_dictionary_description :
                                                           DictionaryDescription(dictionary_file_path, parent_file_path, dictionary_type_if_creating);
}


bool ManageFilesDlg::UsesDefaultWorkingStoringDictionary() const
{
    const std::string working_storage_dictionary_file_path = NewFileCreator::GetDefaultWorkingStorageDictionaryFilePath(m_application.GetApplicationFilePath());
    return ( m_application.GetDictionaryDescription(working_storage_dictionary_file_path, SO::Empty_string, true) != nullptr );
}


void ManageFilesDlg::CheckDictionaryIsUnused(const std::string& dictionary_file_path) const
{
    if( ContainsStringInVectorNoCase(m_application.GetExternalDictionaryFilePaths(), dictionary_file_path) )
    {
        throw CSProException("The dictionary '%s' is already used as an external dictionary'",
                             PortableFunctions::PathGetFilename(dictionary_file_path).c_str());
    }

    // make sure the dictionary isn't part of a form file or table spec
    const DictionaryDescription* const existing_dictionary_description = m_application.GetDictionaryDescription(dictionary_file_path, SO::Empty_string, true);

    if( existing_dictionary_description != nullptr )
    {
        throw CSProException("The dictionary '%s' is already in use with parent '%s'",
                             PortableFunctions::PathGetFilename(dictionary_file_path).c_str(),
                             PortableFunctions::PathGetFilename(existing_dictionary_description->GetParentFilePath()).c_str());
    }
}


CDialog* ManageFilesDlg::AddExternalDictionary(const std::string& dictionary_file_path, const DictionaryType dictionary_type)
{
    std::shared_ptr<const CDataDict> dictionary = NewFileCreator::GetDictionary(dictionary_file_path);

    // only add dictionaries with unique names
    ValidateName(dictionary->GetName(), SO::Empty_string);

    DictionaryDescription dictionary_description(dictionary_file_path, dictionary_type);

    m_application.AddExternalDictionary(dictionary_file_path);
    m_application.AddDictionaryDescription(dictionary_description);

    // keep track of what is added
    m_filesAdded.emplace_back(dictionary_file_path, AppFileType::Dictionary);

    return BuildTreeDictionary(AppFileType::Dictionary,
                               std::move(dictionary_description),
                               std::move(dictionary),
                               true);
}


void ManageFilesDlg::OnAddDictionaryExternal()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select One or More Dictionaries", false, true,
                                                                         FileExtensions::Dictionary, L"Data Dictionary Files (*.dcf)|*.dcf|");

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        // only load dictionaries that aren't already added
        if( ContainsStringInVectorNoCase(m_application.GetExternalDictionaryFilePaths(), file_path) )
            continue;

        try
        {
            CheckDictionaryIsUnused(file_path);

            last_added_dialog = AddExternalDictionary(file_path, DictionaryType::External);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("Error adding dictionary '%s':\n\n%s",
                                             file_path.c_str(), exception.what()));
        }
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


void ManageFilesDlg::OnAddDictionaryWorking()
{
    try
    {
        const std::string working_storage_dictionary_file_path = NewFileCreator::CreateWorkingStorageDictionary(m_application);

        // only add dictionaries that aren't already added
        if( m_application.GetDictionaryDescription(working_storage_dictionary_file_path) != nullptr )
            return;

        CDialog* const added_dialog = AddExternalDictionary(working_storage_dictionary_file_path, DictionaryType::Working);
        SetPage(added_dialog);

        // disable the option to add another working storage dictionary
        EnableMenuItem(m_addFileButton.m_hMenu, ID_ADD_DICTIONARY_WORKING, MF_BYCOMMAND | MF_GRAYED);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(SO::Concatenate("Error adding working storage dictionary:\n\n", exception.what()));
    }
}


void ManageFilesDlg::OnRemoveDictionary(const ManagePage& manage_page)
{
    m_application.DropExternalDictionary(manage_page.path);
    m_application.DropDictionaryDescription(manage_page.path, false);

    // reenable the option to add another working storage dictionary if that is what is removed
    if( !UsesDefaultWorkingStoringDictionary() )
        EnableMenuItem(m_addFileButton.m_hMenu, ID_ADD_DICTIONARY_WORKING, MF_BYCOMMAND | MF_ENABLED);
}


void ManageFilesDlg::OnAddCode()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select Code File(s)", true, true,
                                                                         FileExtensions::Logic, L"Code Files (*.apc;*.app;*.js;*.mjs)|*.apc;*.app;*.js;*.mjs|");
    open_file_dlg->DisableExtensionCheck();

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        // only load code files that aren't already added
        if( IsFilePathInUse(m_application.GetCodeFiles(), file_path) )
            continue;

        try
        {
            const CodeType code_type = CodeFile::GetSuggestedCodeTypeForExternalCode(file_path);

            m_application.AddCodeFile(NewFileCreator::CreateOrOpenCodeFile(file_path, code_type, m_application));

            last_added_dialog = BuildTreeCode(AppFileType::Code, m_application.GetCodeFiles().back());

            // keep track of what is added
            m_filesAdded.emplace_back(file_path, AppFileType::Code);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("Error adding code file '%s':\n\n%s",
                                             file_path.c_str(), exception.what()));
        }
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


void ManageFilesDlg::OnAddMessages()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select Message File(s)", true, true,
                                                                         FileExtensions::Message, L"Message Files (*.mgf)|*.mgf|");

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        // only load message files that aren't already added
        if( IsFilePathInUse(m_application.GetMessageFiles(), file_path) )
            continue;

        try
        {
            constexpr AppMessageFile::Type type = AppMessageFile::Type::User;
            m_application.AddMessageFile(NewFileCreator::CreateOrOpenMessageFile(file_path, type, m_application));

            last_added_dialog = BuildTreeMessages(AppFileType::Message, m_application.GetMessageFiles().back(), false);

            // keep track of what is added
            m_filesAdded.emplace_back(file_path, AppFileType::Message);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("Error adding message file '%s':\n\n%s",
                                             file_path.c_str(), exception.what()));
        }
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


void ManageFilesDlg::OnAddReport()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select Report File(s)", true, true,
                                                                         FileExtensions::HTML, L"Report Files (*.html;*.md)|*.html;*.md|");
    open_file_dlg->DisableExtensionCheck();

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        try
        {
            const ReportFile::Encoding encoding = ReportFile::GetDefaultEncodingFromFilename(file_path, true);

            // if the file does not exist, create a default one
            if( !PortableFunctions::FileIsRegular(file_path) )
                CreateDefaultReport(file_path, encoding);

            // create a unique name based on the filename
            m_application.AddReport(ReportFile(CreateUniqueName(Path::GetFilenameWithoutExtension(file_path)),
                                               encoding,
                                               TextSourceEditable::FindOpenOrCreate(file_path)));

            last_added_dialog = BuildTreeReport(AppFileType::Report, m_application.GetReportFiles().back());

            // keep track of what is added
            m_filesAdded.emplace_back(file_path, AppFileType::Report);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("Error adding report file '%s':\n\n%s",
                                             file_path.c_str(), exception.what()));
        }
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


void ManageFilesDlg::CreateDefaultReport(const std::string& report_file_path, const ReportFile::Encoding encoding)
{
    ASSERT(!PortableFunctions::FileIsRegular(report_file_path));

    const std::string templates_directory = Html::GetDirectory(Html::Subdirectory::Templates);

    if( encoding == ReportFile::Encoding::Html )
    {
        if( CreateDefaultHtmlReport(templates_directory, report_file_path) )
            return;
    }

    else if( encoding == ReportFile::Encoding::Markdown )
    {
        const std::string markdown_report_template = Path::Combine(templates_directory, "report-basic.md");
        PortableFunctions::FileCopyWithExceptions(markdown_report_template, report_file_path, FileOverwriteFlag::Fail);
        return;
    }

    // default to creating a blank file for other report types, or if the user didn't want to use a template
    FileIO::WriteText(report_file_path, std::string_view(), false);
}


bool ManageFilesDlg::CreateDefaultHtmlReport(const std::string& templates_directory, const std::string& report_file_path)
{
    const std::string basic_report_template = Path::Combine(templates_directory, "report-basic.html");
    const std::string sections_report_template = Path::Combine(templates_directory, "report-sections.html");

    if( PortableFunctions::FileIsRegular(basic_report_template) && PortableFunctions::FileIsRegular(sections_report_template) )
    {
        ChoiceDlg choice_dlg(0);

        choice_dlg.SetTitle("Would you like to use one of the HTML report templates?");

        choice_dlg.AddChoice("Basic report");
        choice_dlg.AddChoice("Report with sections and a header/footer");

        if( choice_dlg.DoModal() == IDOK )
        {
            const std::string& desired_template = ( choice_dlg.GetSelectedChoiceIndex() == 0 ) ? basic_report_template :
                                                                                                 sections_report_template;
            PortableFunctions::FileCopy(desired_template, report_file_path, true);
            return true;
        }
    }

    return false;
}


void ManageFilesDlg::OnAddResourceDirectory()
{
    std::string directory = SelectFolderDialog(L"Select Resource Folder");

    if( directory.empty() )
        return;

    ASSERT(directory == PortableFunctions::PathRemoveTrailingSlash(directory));

    // only add resource directories that aren't already added
    if( m_application.GetResource(directory) != nullptr )
        return;

    const AppResource& resource = m_application.AddResource(AppResource(std::move(directory)));

    CDialog* const added_dialog = BuildTreeResource(AppFileType::Resource, resource);

    // keep track of what is added
    m_filesAdded.emplace_back(resource.GetPath(), AppFileType::Resource);

    SetPage(added_dialog);
}


void ManageFilesDlg::OnAddResourceFile()
{
    const std::unique_ptr<OpenFileDlg> open_file_dlg = CreateOpenFileDlg(L"Select Resource File(s)", false, true,
                                                                         nullptr, std::wstring());

    if( open_file_dlg->DoModal() != IDOK )
        return;

    CDialog* last_added_dialog = nullptr;

    for( const std::string& file_path : open_file_dlg->GetFilePaths() )
    {
        // only add resource files that aren't already added
        if( m_application.GetResource(file_path) != nullptr )
            continue;

        const AppResource& resource = m_application.AddResource(AppResource(file_path));

        last_added_dialog = BuildTreeResource(AppFileType::Resource, resource);

        // keep track of what is added
        m_filesAdded.emplace_back(resource.GetPath(), AppFileType::Resource);
    }

    if( last_added_dialog != nullptr )
        SetPage(last_added_dialog);
}


void ManageFilesDlg::UpdateNameInTree(const CDialog& dialog, std::wstring new_name)
{
    ManagePage* const manage_page = GetManagePage(&dialog);

    if( manage_page == nullptr )
    {
        ASSERT(false);
        return;
    }

    manage_page->tree_name = std::move(new_name);

    // if the names are showing, invalidate the item's bounding rectangle so that the name is redrawn
    if( m_showNamesInTree )
    {
        CRect rect;

        if( m_treeCtrl.GetItemRect(m_treeCtrl.GetSelectedItem(), &rect, FALSE) )
            m_treeCtrl.InvalidateRect(&rect, TRUE);
    }
}


void ManageFilesDlg::ValidateName(const std::string& name, const std::string& path) const
{
    // check that the name is valid
    if( SO::IsWhitespace(name) )
        throw CSProException("Symbol names cannot be blank.");

    if( !CIMSAString::IsName(name) )
        ExceptionThrowingSystemMessageIssuer().Issue(MessageType::Error, 101, name.c_str());

    // check if this is a reserved name
    if( Logic::ReservedWords::IsReservedWord(name) )
        ExceptionThrowingSystemMessageIssuer().Issue(MessageType::Error, 162, name.c_str());

    // check the application's objects to ensure that the name is unique
    for( const ManagePage& manage_page : m_managePages )
    {
        if( SO::EqualsNoCase(path, manage_page.path) )
            return;

        switch( manage_page.app_file_type )
        {
            case AppFileType::ApplicationBatch:
            case AppFileType::ApplicationEntry:
                CheckNameIsUnused(name, *assert_cast<const ApplicationPropertiesPageDlg*>(manage_page.dialog.get()));
                break;

            case AppFileType::ApplicationTabulation:
                // disabled this check for tabulation applications because the table spec name and the application share the same name,
                // and the application name (more generally) doesn't need to be unique since it isn't used in logic
                // CheckNameIsUnused(name, *assert_cast<const ApplicationPropertiesPageDlg*>(manage_page.dialog.get()));
                break;

            case AppFileType::Form:
            case AppFileType::Order:
                CheckNameIsUnused(name, *assert_cast<const FormFilePropertiesPageDlg*>(manage_page.dialog.get()));
                break;

            case AppFileType::TableSpec:
                CheckNameIsUnused(name, *assert_cast<const TableSpecPropertiesPageDlg*>(manage_page.dialog.get()));
                break;

            case AppFileType::Dictionary:
                CheckNameIsUnused(name, assert_cast<const DictionaryPropertiesPageDlg*>(manage_page.dialog.get())->GetDictionary());
                break;

            case AppFileType::Report:
                CheckNameIsUnused(name, assert_cast<const ReportPropertiesPageDlg*>(manage_page.dialog.get())->GetReportFile());
                break;

            default:
                ASSERT(manage_page.app_file_type == AppFileType::Code ||
                       manage_page.app_file_type == AppFileType::Message ||
                       manage_page.app_file_type == AppFileType::QuestionText ||
                       manage_page.app_file_type == AppFileType::Resource);
                break;
        }
    }
}


void ManageFilesDlg::CheckNameIsUnused(const std::string& name, const ApplicationPropertiesPageDlg& application_properties_page_dlg)
{
    if( SO::EqualsNoCase(name, application_properties_page_dlg.GetName()) )
        throw CSProException("The name '%s' is already in use as the application name.", name.c_str());
}


void ManageFilesDlg::CheckNameIsUnused(const std::string& name, const FormFilePropertiesPageDlg& form_file_properties_page_dlg)
{
    if( SO::EqualsNoCase(name, form_file_properties_page_dlg.GetName()) )
        throw CSProException("The name '%s' is already in use by the form file: %s", name.c_str(), form_file_properties_page_dlg.GetFilePath().c_str());
}


void ManageFilesDlg::CheckNameIsUnused(const std::string& name, const TableSpecPropertiesPageDlg& table_spec_properties_page_dlg)
{
    if( SO::EqualsNoCase(name, table_spec_properties_page_dlg.GetName()) )
        throw CSProException("The name '%s' is already in use by the table spec: %s", name.c_str(), table_spec_properties_page_dlg.GetFilePath().c_str());
}

void ManageFilesDlg::CheckNameIsUnused(const std::string& name, const CDataDict& dictionary)
{
    if( SO::EqualsNoCase(name, dictionary.GetName()) )
        throw CSProException("The name '%s' is already in use by the dictionary: %s", name.c_str(), dictionary.GetFilePath().c_str());
}


void ManageFilesDlg::CheckNameIsUnused(const std::string& name, const ReportFile& report_file)
{
    if( SO::EqualsNoCase(name, report_file.GetName()) )
        throw CSProException("The name '%s' is already in use as the name of a report file: %s", name.c_str(), report_file.GetFilePath().c_str());
}


std::string ManageFilesDlg::CreateUniqueName(const std::string& initial_name_candidate) const
{
    return CIMSAString::CreateUnreservedName(initial_name_candidate,
        [&](const std::string& name_candidate)
        {
            try
            {
                ValidateName(name_candidate, SO::Empty_string);
                return true;
            }
            catch(...) { return false; }
        });
}


void ManageFilesDlg::OnOK()
{
    if( !ValidatePages() )
        return;

    // rebuild the application's objects
    m_application.ClearApplicationFiles();

    for( const ManagePage& manage_page : m_managePages )
    {
        switch( manage_page.app_file_type )
        {
            case AppFileType::ApplicationBatch:
            case AppFileType::ApplicationEntry:
            case AppFileType::ApplicationTabulation:
            {
                const ApplicationPropertiesPageDlg& application_properties_page_dlg = *assert_cast<const ApplicationPropertiesPageDlg*>(manage_page.dialog.get());
                m_application.SetName(application_properties_page_dlg.GetName());
                m_application.SetLabel(application_properties_page_dlg.GetLabel());
                break;
            }

            case AppFileType::Form:
            case AppFileType::Order:
            {
                FormFilePropertiesPageDlg& form_file_properties_page_dlg = *assert_cast<FormFilePropertiesPageDlg*>(manage_page.dialog.get());
                m_application.AddForm(form_file_properties_page_dlg.GetFilePath());

                if( form_file_properties_page_dlg.ApplyChanges() )
                    m_nonApplicationObjectsModified.insert(form_file_properties_page_dlg.GetFilePath());

                break;
            }

            case AppFileType::TableSpec:
            {
                TableSpecPropertiesPageDlg& table_spec_properties_page_dlg = *assert_cast<TableSpecPropertiesPageDlg*>(manage_page.dialog.get());
                m_application.AddTableSpec(table_spec_properties_page_dlg.GetFilePath());

                if( table_spec_properties_page_dlg.ApplyChanges() )
                    m_nonApplicationObjectsModified.insert(table_spec_properties_page_dlg.GetFilePath());

                break;
            }

            case AppFileType::Dictionary:
            {
                DictionaryDescription dictionary_description = assert_cast<const DictionaryPropertiesPageDlg*>(manage_page.dialog.get())->GetDictionaryDescription();

                if( dictionary_description.GetParentFilePath().empty() )
                    m_application.AddExternalDictionary(dictionary_description.GetDictionaryFilePath());

                m_application.AddDictionaryDescription(std::move(dictionary_description));

                break;
            }

            case AppFileType::Code:
            {
                m_application.AddCodeFile(assert_cast<const CodeFilePropertiesPageDlg*>(manage_page.dialog.get())->GetCodeFile());
                break;
            }

            case AppFileType::Message:
            {
                m_application.AddMessageFile(assert_cast<const AppMessageFilePropertiesPageDlg*>(manage_page.dialog.get())->GetAppMessageFile());
                break;
            }

            case AppFileType::Report:
            {
                m_application.AddReport(assert_cast<const ReportPropertiesPageDlg*>(manage_page.dialog.get())->GetReportFile());
                break;
            }

            case AppFileType::Resource:
            {
                m_application.AddResource(assert_cast<const ResourcePropertiesPageDlg*>(manage_page.dialog.get())->GetResource());
                break;
            }

            case AppFileType::QuestionText:
            {
                m_application.SetQuestionTextFilePath(manage_page.path);
                break;
            }

            default:
            {
                ASSERT(false);
                break;
            }
        }
    }

    // with the application fully built, we can do additional cross-application checks
    try
    {
        if( m_application.GetEngineAppType() == EngineAppType::Tabulation &&
            m_application.GetFirstDictionaryFilePathOfType(DictionaryType::Working).empty() )
        {
            throw CSProException("A tabulation application must have at least one working storage dictionary.");
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    CDialog::OnOK();
}
