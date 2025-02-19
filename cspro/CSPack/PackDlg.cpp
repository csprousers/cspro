#include "StdAfx.h"
#include "PackDlg.h"
#include <zUtilO/ArrUtil.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/TemporaryFile.h>
#include <zUtilO/WindowsUtf8.h>
#include <zUtilO/WindowsWS.h>


BEGIN_MESSAGE_MAP(PackDlg, CDialog)
    ON_MESSAGE(UWM::Pack::UpdateDialogUI, OnUpdateDialogUI)
    ON_COMMAND(IDOK, OnOK)
    ON_COMMAND(IDCANCEL, OnFileExit)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_APP_EXIT, OnFileExit)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_PACK, OnPack)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_INPUTS, OnInputsItemChanged)
    ON_BN_CLICKED(IDC_ADD_FILE, OnInputsAddFile)
    ON_BN_CLICKED(IDC_ADD_FOLDER, OnInputsAddFolder)
    ON_BN_CLICKED(IDC_REMOVE, OnInputsRemove)
    ON_BN_CLICKED(IDC_CLEAR, OnInputsClear)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_INCLUDE_VALUE_SET_IMAGES, IDC_INCLUDE_RECURSIVE_DIRECTORIES, OnOptionChange)
    ON_BN_CLICKED(IDC_ZIP_BROWSE, OnZipBrowse)
    ON_EN_CHANGE(IDC_ZIP, OnZipEditChange)
    ON_BN_CLICKED(IDC_INPUT_DETAILS, OnInputDetails)
    ON_BN_CLICKED(IDC_PACK, OnPack)
END_MESSAGE_MAP()


namespace
{
    constexpr const wchar_t* PackSpecFilter = L"Pack Specification Files (*.cspack)|*.cspack|"
                                              L"All Files (*.*)|*.*||";

    constexpr const wchar_t* AddFileFilter  = L"Application Files (*.ent;*.bch;*.xtb)|*.ent;*.bch;*.xtb|"
                                              L"Data Entry Application Files (*.ent)|*.ent|"
                                              L"Batch Edit Application Files (*.bch)|*.bch|"
                                              L"Tabulation Application Files (*.xtb)|*.xtb|"
                                              L"All Files (*.*)|*.*||";

    namespace UpdateDialogAction
    {
        constexpr WPARAM UpdateAll           = 1;
        constexpr WPARAM UpdateInputs        = 2;
        constexpr WPARAM UpdateOptions       = 3;
        constexpr WPARAM UpdateTitleAndMenus = 4;
    }
}


PackDlg::PackDlg(std::unique_ptr<PackSpec> pack_spec, std::string file_path, CWnd* const pParent/* = nullptr*/)
    :   CDialog(PackDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME)),
        m_packSpec(std::move(pack_spec)),
        m_modified(false)
{
    if( !file_path.empty() )
    {
        ASSERT(m_packSpec != nullptr && SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), FileExtensions::PackSpec));
        m_packSpecFilePath = std::move(file_path);
    }

    if( m_packSpec == nullptr )
        m_packSpec = std::make_unique<PackSpec>();
}


void PackDlg::DoDataExchange(CDataExchange* const pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_INPUTS, m_inputsListCtrl);
}


BOOL PackDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // store the base module name
    m_moduleName = WindowsUtf8::GetText(this);

    // add the menu
    m_menu.LoadMenu(IDR_PACK);
    SetMenu(&m_menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // set up the list contrl and the icon image list
    m_inputsListCtrl.InsertColumn(0, L"");
    m_inputsListCtrl.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

    m_systemIconImageList.Create(16, 16, ILC_COLOR32);
    m_inputsListCtrl.SetImageList(&m_systemIconImageList, LVSIL_SMALL);

    // set up the callback to allow the dragging of files onto the inputs list
    m_inputsListCtrl.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::AddToPaths,
        [&](const std::vector<std::string>& paths)
        {
            AddInputs(paths);
        });

    PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateAll);

    if( !CanRunPack() )
    {
        GetDlgItem(IDC_ADD_FILE)->SetFocus();
        return FALSE;
    }

    return TRUE;
}


void PackDlg::OnOK()
{
    // force the closing of the dialog as if it were a full application (not just a dialog)
}


bool PackDlg::CanRunPack() const
{
    return ( m_packSpec->GetNumEntries() > 0 && !SO::IsWhitespace(m_packSpec->GetZipFilePath()) );
}


LRESULT PackDlg::OnUpdateDialogUI(const WPARAM wParam, const LPARAM lParam)
{
    // add the inputs
    if( wParam == UpdateDialogAction::UpdateAll || wParam == UpdateDialogAction::UpdateInputs )
    {
        m_selectedInputIndex.reset();
        m_inputsListCtrl.DeleteAllItems();

        for( size_t i = 0; i < m_packSpec->GetNumEntries(); ++i )
        {
            const PackEntry& pack_entry = m_packSpec->GetEntry(i);
            const int icon_index = m_systemIconImageList.GetIconIndexFromPath(pack_entry.GetPath().c_str());
            m_inputsListCtrl.InsertItem(i, TC::ToWide(pack_entry.GetPath()).c_str(), icon_index);
        }

        // select an entry (which will call this method with the UpdateOptions action)
        const int entry_to_select = std::min(static_cast<int>(lParam), m_inputsListCtrl.GetItemCount() - 1);

        if( entry_to_select >= 0 )
            ListView_SetItemState(m_inputsListCtrl.m_hWnd, entry_to_select, LVIS_SELECTED, LVIS_SELECTED);
    }


    // set the zip filename
    if( wParam == UpdateDialogAction::UpdateAll )
    {
        WindowsUtf8::SetText(this, IDC_ZIP, m_packSpec->GetZipFilePath());
    }


    // conditionally enable options based on the selected entry
    if( wParam == UpdateDialogAction::UpdateAll || wParam == UpdateDialogAction::UpdateInputs || wParam == UpdateDialogAction::UpdateOptions )
    {
        ASSERT(!m_selectedInputIndex.has_value() || *m_selectedInputIndex < m_packSpec->GetNumEntries());

        auto update = [&](const cs::string_sz group_box_text,
                          const DirectoryPackEntryExtras* const directory_extras,
                          const DictionaryPackEntryExtras* const dictionary_extras,
                          const PffPackEntryExtras* const pff_extras,
                          const ApplicationPackEntryExtras* const application_extras)
        {
            WindowsUtf8::SetText(this, IDC_OPTIONS_GROUP, group_box_text.c_str());

            auto update_check = [&](const int control_id, const auto* const extras, auto extra_field)
            {
                const bool enabled = ( extras != nullptr );
                const bool checked = ( enabled && extras->*extra_field );

                CButton* button = static_cast<CButton*>(GetDlgItem(control_id));
                button->EnableWindow(enabled);
                button->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
            };

            update_check(IDC_INCLUDE_VALUE_SET_IMAGES, dictionary_extras, &DictionaryPackEntryExtras::value_set_images);

            update_check(IDC_INCLUDE_PFF, application_extras, &ApplicationPackEntryExtras::pff);
            update_check(IDC_INCLUDE_RESOURCES, application_extras, &ApplicationPackEntryExtras::resources);

            update_check(IDC_INCLUDE_INPUT_DATA_FILE, pff_extras, &PffPackEntryExtras::input_data);
            update_check(IDC_INCLUDE_EXTERNAL_DATA_FILES, pff_extras, &PffPackEntryExtras::external_dictionary_data);
            update_check(IDC_INCLUDE_USER_FILES, pff_extras, &PffPackEntryExtras::user_files);

            update_check(IDC_INCLUDE_RECURSIVE_DIRECTORIES, directory_extras, &DirectoryPackEntryExtras::recursive);
        };

        if( m_selectedInputIndex.has_value() )
        {
            const PackEntry& pack_entry = m_packSpec->GetEntry(*m_selectedInputIndex);

            update("Extra Inclusions: " + PortableFunctions::PathGetFilename(pack_entry.GetPath()),
                   pack_entry.GetDirectoryExtras(), pack_entry.GetDictionaryExtras(), pack_entry.GetPffExtras(), pack_entry.GetApplicationExtras());
        }

        else
        {
            update("Extra Inclusions", nullptr, nullptr, nullptr, nullptr);
        }
    }


    // update the window title if it has changed
    if( !m_lastWindowTitleInputs.has_value() ||
        std::get<0>(*m_lastWindowTitleInputs) != m_packSpecFilePath ||
        std::get<1>(*m_lastWindowTitleInputs) != m_modified )
    {
        std::string title = m_moduleName;

        if( m_modified || m_packSpecFilePath.has_value() )
        {
            title.append(FormatText(" [%s%s]",
                                    m_packSpecFilePath.has_value() ? PortableFunctions::PathGetFilename(*m_packSpecFilePath).c_str() : "Untitled",
                                    m_modified ? " *" : ""));
        }

        WindowsUtf8::SetText(this, title);

        m_lastWindowTitleInputs.emplace(m_packSpecFilePath, m_modified);
    }


    // update the menu
    const bool inputs_exist = ( m_packSpec->GetNumEntries() > 0 );
    GetDlgItem(IDC_INPUT_DETAILS)->EnableWindow(inputs_exist);

    const bool can_pack = CanRunPack();
    GetDlgItem(IDC_PACK)->EnableWindow(can_pack);
    m_menu.EnableMenuItem(ID_FILE_PACK, can_pack ? MF_ENABLED : MF_DISABLED);

    return 0;
}


void PackDlg::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void PackDlg::OnFileNew()
{
    if( !ContinueWithClosePackSpecOperation() )
        return;

    m_packSpec = std::make_unique<PackSpec>();
    m_packSpecFilePath.reset();
    m_modified = false;

    PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateAll);
}


void PackDlg::OnFileOpen()
{
    if( !ContinueWithClosePackSpecOperation() )
        return;

    OpenFileDlg open_file_dlg(0, FileExtensions::PackSpec, nullptr, PackSpecFilter, this);
    open_file_dlg.SetTitle(L"Open Pack Specification File");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        auto pack_spec = std::make_unique<PackSpec>();
        pack_spec->Load(open_file_dlg.GetFilePath(), false, false);

        m_packSpec = std::move(pack_spec);
        m_packSpecFilePath = open_file_dlg.GetFilePath();
        m_modified = false;

        PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateAll);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void PackDlg::OnFileSave()
{
    if( m_packSpecFilePath.has_value() )
    {
        if( m_modified )
            SavePackSpec(*m_packSpecFilePath, false);
    }

    else
    {
        OnFileSaveAs();
    }
}


void PackDlg::OnFileSaveAs()
{
    const std::string starting_directory = ( m_packSpecFilePath.has_value() )  ? PortableFunctions::PathGetDirectory(*m_packSpecFilePath) :
                                           ( m_packSpec->GetNumEntries() > 0 ) ? PortableFunctions::PathGetDirectory(m_packSpec->GetEntry(0).GetPath()) :
                                                                                 std::string();

    SaveFileDlg save_file_dlg(0, FileExtensions::PackSpec, starting_directory, PackSpecFilter, this);
    save_file_dlg.SetTitle(L"Save Pack Specification File");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    SavePackSpec(save_file_dlg.GetFilePath(), true);
}


void PackDlg::SavePackSpec(std::string file_path, const bool create_pff)
{
    try
    {
        m_packSpec->Save(file_path);
        m_packSpecFilePath = std::move(file_path);
        m_modified = false;
        PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateTitleAndMenus);

        // when saving a spec file for the first time, also create a PFF
        if( create_pff )
        {
            const std::string pff_file_path = PortableFunctions::PathReplaceFileExtension(*m_packSpecFilePath, FileExtensions::Pff);

            if( !PortableFunctions::FileIsRegular(pff_file_path) )
            {
                PFF pff(UTF8_TODO::GetCString(pff_file_path));

                pff.SetAppType(APPTYPE::PACK_TYPE);
                pff.SetAppFName(UTF8_TODO::GetCString(*m_packSpecFilePath));
                pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(pff_file_path, FileExtensions::Listing)));
                pff.SetViewListing(VIEWLISTING::ALWAYS);

                pff.Save();
            }
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void PackDlg::OnFileExit()
{
    if( ContinueWithClosePackSpecOperation() )
        EndDialog(IDCANCEL);
}


bool PackDlg::ContinueWithClosePackSpecOperation()
{
    if( m_modified && m_packSpecFilePath.has_value() )
    {
        const int result = AfxMessageBox(FormatText("Save changes to '%s'?", PortableFunctions::PathGetFilename(*m_packSpecFilePath).c_str()), MB_YESNOCANCEL);

        if( result == IDCANCEL )
        {
            return false;
        }

        else if( result == IDYES )
        {
            OnFileSave();
        }
    }

    return true;
}


void PackDlg::AddInputs(const std::vector<std::string>& paths)
{
    bool input_added = false;

    for( const std::string& path : paths )
    {
        // do not add entries that already exist
        const SharedPointerVectorWrapper<PackEntry>& pack_entries = m_packSpec->GetEntries();
        const auto& entry_lookup = std::find_if(pack_entries.cbegin(), pack_entries.cend(),
                                                [&](const PackEntry& pack_entry) { return SO::EqualsNoCase(pack_entry.GetPath(), path); });

        if( entry_lookup == pack_entries.cend() )
        {
            try
            {
                m_packSpec->AddEntry(PackEntry::Create(path));
                input_added = true;
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
            }
        }
    }

    if( input_added )
    {
        m_modified = true;

        WPARAM update_action = UpdateDialogAction::UpdateInputs;

        // after adding the first input, set the zip filename if it has not been set yet
        ASSERT(!paths.empty());

        if( m_packSpec->GetNumEntries() == 1 &&
            m_packSpec->GetZipFilePath().empty() &&
            PortableFunctions::FileIsRegular(paths.front()) )
        {
            m_packSpec->SetZipFilePath(PortableFunctions::PathReplaceFileExtension(paths.front(), FileExtensions::Zip));
            update_action = UpdateDialogAction::UpdateAll;
        }

        PostMessage(UWM::Pack::UpdateDialogUI, update_action, static_cast<LPARAM>(m_packSpec->GetNumEntries() - 1));
    }
}


void PackDlg::OnInputsAddFile()
{
    OpenFileDlg open_file_dlg(0, nullptr, nullptr, AddFileFilter, this);
    open_file_dlg.SetTitle(L"Select Application(s) or File(s)")
                 .SetMultiSelectBuffer();

    if( open_file_dlg.DoModal() != IDOK )
        return;

    AddInputs(open_file_dlg.GetFilePaths());
}


void PackDlg::OnInputsAddFolder()
{
    std::optional<std::string> folder = SelectFolderDialog(m_hWnd, "Select Folder");

    if( folder.has_value() )
        AddInputs({ std::move(*folder) });
}


void PackDlg::OnInputsRemove()
{
    if( !m_selectedInputIndex.has_value() )
        return;

    m_packSpec->RemoveEntry(*m_selectedInputIndex);
    m_modified = true;
    PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateInputs, static_cast<LPARAM>(*m_selectedInputIndex));
}


void PackDlg::OnInputsClear()
{
    if( m_packSpec->GetNumEntries() == 0 )
        return;

    const std::string prompt = FormatText("Are you sure that you want to clear %d input%s?",
                                          static_cast<int>(m_packSpec->GetNumEntries()), PluralizeWord(m_packSpec->GetNumEntries()));

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return;

    m_packSpec->RemoveAllEntries();
    m_packSpec->SetZipFilePath(std::string());
    m_modified = true;
    PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateAll);
}


void PackDlg::OnInputsItemChanged(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) == LVIF_STATE &&
        ( pNMLV->uNewState & LVIS_SELECTED ) == LVIS_SELECTED && ( pNMLV->uOldState & LVIS_SELECTED ) == 0 )
    {
        m_selectedInputIndex = pNMLV->iItem;
    }

    else
    {
        m_selectedInputIndex.reset();
    }

    PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateOptions);

    *pResult = 0;
}


void PackDlg::OnOptionChange(const UINT nID)
{
    if( !m_selectedInputIndex.has_value() || *m_selectedInputIndex >= m_packSpec->GetNumEntries() )
    {
        ASSERT(false);
        return;
    }

    PackEntry& pack_entry = m_packSpec->GetEntry(*m_selectedInputIndex);

    CButton* const button = static_cast<CButton*>(GetDlgItem(nID));
    const bool checked = ( button->GetCheck() == BST_CHECKED );

    auto update_option = [checked](auto* const extras, auto extra_field)
    {
        ASSERT(extras != nullptr);
        extras->*extra_field = checked;
    };

    if     ( nID == IDC_INCLUDE_VALUE_SET_IMAGES )      update_option(pack_entry.GetDictionaryExtras(),  &DictionaryPackEntryExtras::value_set_images);
    else if( nID == IDC_INCLUDE_PFF )                   update_option(pack_entry.GetApplicationExtras(), &ApplicationPackEntryExtras::pff);
    else if( nID == IDC_INCLUDE_RESOURCES )             update_option(pack_entry.GetApplicationExtras(), &ApplicationPackEntryExtras::resources);
    else if( nID == IDC_INCLUDE_INPUT_DATA_FILE )       update_option(pack_entry.GetPffExtras(),         &PffPackEntryExtras::input_data);
    else if( nID == IDC_INCLUDE_EXTERNAL_DATA_FILES )   update_option(pack_entry.GetPffExtras(),         &PffPackEntryExtras::external_dictionary_data);
    else if( nID == IDC_INCLUDE_USER_FILES )            update_option(pack_entry.GetPffExtras(),         &PffPackEntryExtras::user_files);
    else if( nID == IDC_INCLUDE_RECURSIVE_DIRECTORIES ) update_option(pack_entry.GetDirectoryExtras(),   &DirectoryPackEntryExtras::recursive);
    else                                                ASSERT(false);

    // toggling the PFF option will enable/disable the PFF extras, so redraw the options in that case
    const WPARAM update_action = ( nID == IDC_INCLUDE_PFF ) ? UpdateDialogAction::UpdateOptions :
                                                              UpdateDialogAction::UpdateTitleAndMenus;

    m_modified = true;
    PostMessage(UWM::Pack::UpdateDialogUI, update_action);
}


void PackDlg::OnZipBrowse()
{
    const std::string zip_file_path = WindowsUtf8::GetText(this, IDC_ZIP);

    SaveFileDlg save_file_dlg(0, FileExtensions::Zip, zip_file_path, "ZIP Files (*.zip)|*.zip||", this);
    save_file_dlg.SetTitle(L"Select ZIP File");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    WindowsUtf8::SetText(this, IDC_ZIP, save_file_dlg.GetFilePath());
}


void PackDlg::OnZipEditChange()
{
    static std::string working_directory = GetWorkingDirectory();
    std::string zip_file_path = MakeFullPath(working_directory, WindowsUtf8::GetText(this, IDC_ZIP));

    if( zip_file_path != m_packSpec->GetZipFilePath() )
    {
        m_packSpec->SetZipFilePath(std::move(zip_file_path));
        m_modified = true;
        PostMessage(UWM::Pack::UpdateDialogUI, UpdateDialogAction::UpdateTitleAndMenus);
    }
}


void PackDlg::OnInputDetails()
{
    try
    {
        DisplayInputDetailsReport();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void PackDlg::OnPack()
{
    try
    {
        // write the results to a temporary listing file
        const std::string listing_file_path = GetUniqueTempFilePath("CSPack.lst", true);
        TemporaryFile::RegisterFileForDeletion(listing_file_path);

        PFF pff;
        pff.SetAppType(APPTYPE::PACK_TYPE);
        pff.SetListingFName(UTF8_TODO::GetCString(listing_file_path));
        pff.SetViewListing(VIEWLISTING::ALWAYS);

        if( m_packSpecFilePath.has_value() )
            pff.SetAppFName(UTF8_TODO::GetCString(*m_packSpecFilePath));

        Packer().Run(&pff, *m_packSpec);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
