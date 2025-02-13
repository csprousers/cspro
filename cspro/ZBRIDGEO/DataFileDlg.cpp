#include "StdAfx.h"
#include "DataFileDlg.h"
#include <zToolsO/WinRegistry.h>


namespace
{
    constexpr const TCHAR* LastDataRegistryValueName = _T("<Last Data>");


    bool IsValidDataFilename(const TCHAR* filename, bool allow_wildcards)
    {
        // mostly based on https://stackoverflow.com/questions/1976007/what-characters-are-forbidden-in-windows-and-linux-directory-names
        const TCHAR* filename_itr = filename + _tcslen(filename) - 1;
        ASSERT(filename_itr >= filename);

        // filenames cannot end in a space or dot
        if( *filename_itr == _T(' ') || *filename_itr == _T('.') )
            return false;

        // filenames cannot have invalid characters
        for( ; filename_itr >= filename; filename_itr-- )
        {
            if( *filename_itr < 32 )
                return false;
        }

        static TCHAR InvalidCharacters[] = { _T('<'), _T('>'), _T(':'), _T('"'), _T('/'), _T('\\'), _T('|'), 0 };


        if( ( _tcspbrk(filename, InvalidCharacters) != nullptr ) ||
            ( !allow_wildcards && PathHasWildcardCharacters(filename) ) )
        {
            return false;
        }

        return true;
    }

    constexpr const TCHAR* OpenExistingSingleFileString    = _T("Select an Existing Data File");
    constexpr const TCHAR* OpenExistingMultipleFilesString = _T("Select Existing Data File(s)");


    const DataFileFilterManager& GetDataFileFilterManager(bool add_only_readable_types)
    {
        return DataFileFilterManager::Get(DataFileFilterManager::UseType::FileChooserDlg, add_only_readable_types);
    }
}


DataFileDlg* DataFileDlg::m_currentDataFileDlg = nullptr;


DataFileDlg::DataFileDlg(const Type type, const bool add_only_readable_types, ConnectionString connection_string/* = ConnectionString()*/)
    :   CFileDialog(( type == Type::OpenExisting ),
                    nullptr,
                    connection_string.IsDefined() ? connection_string.ToStringWithoutDirectory().c_str() : nullptr,
                    OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
                    GetDataFileFilterManager(add_only_readable_types).GetFilterText().c_str()),
        m_type(type),
        m_dataFileFilterManager(GetDataFileFilterManager(add_only_readable_types)),
        m_initialConnectionString(std::move(connection_string)),
        m_warnIfDifferentDataRepositoryType(false),
        m_createNewDefaultDataRepositoryType(DataRepositoryType::SQLite),
        m_mustSubclassDialog(true),
        m_fileDialogEventsHandlerCode(0)
{
    ASSERT(add_only_readable_types || type == Type::CreateNew || type == Type::OpenOrCreate);

    // setup the main properties
    SetTitle(( m_type == Type::OpenExisting ) ? OpenExistingSingleFileString :
             ( m_type == Type::OpenOrCreate ) ? _T("Select an Existing or Create a New Data File") :
                                                _T("Create a New Data File"));
    m_ofn.lpstrTitle = m_title;

    ASSERT(m_currentDataFileDlg == nullptr);
    m_currentDataFileDlg = this;
    GetIFileDialog()->Advise(this, &m_fileDialogEventsHandlerCode);

    UpdateInitialDirectory();
    UpdateFilters();
}


DataFileDlg::DataFileDlg(Type type, bool add_only_readable_types, const std::vector<ConnectionString>& connection_strings)
    :   DataFileDlg(type, add_only_readable_types, connection_strings.empty() ? ConnectionString() : connection_strings.front())
{
    // if a repository that does not use filenames is the only connection string, don't set the multiple selection filename
    if( connection_strings.size() == 1 && DataRepositoryHelpers::TypeDoesNotUseFilename(connection_strings.front().GetType()) )
        return;

    m_initialMultipleSelectionFilename = PathHelpers::CreateSingleStringFromConnectionStrings(connection_strings, true);
}


DataFileDlg::~DataFileDlg()
{
    GetIFileDialog()->Unadvise(m_fileDialogEventsHandlerCode);
}


IFileDialog* DataFileDlg::GetIFileDialog()
{
    return ( m_type == Type::OpenExisting ) ? (IFileDialog*)GetIFileOpenDialog() :
                                              (IFileDialog*)GetIFileSaveDialog();
}


DataFileDlg& DataFileDlg::SetTitle(const CString& title)
{
    m_title = title;
    m_ofn.lpstrTitle = m_title;
    return *this;
}

DataFileDlg& DataFileDlg::SetDictionaryFilename(const CString& dictionary_filename)
{
    m_dictionaryFilename = dictionary_filename;
    UpdateInitialDirectory();
    return *this;
}

DataFileDlg& DataFileDlg::SuggestMatchingDataRepositoryType(const ConnectionString& connection_string)
{
    m_suggestedMatchingDataRepositoryTypeConnectionString = connection_string;
    UpdateInitialDirectory();
    UpdateFilters();
    return *this;
}

DataFileDlg& DataFileDlg::SuggestMatchingDataRepositoryType(const std::vector<ConnectionString>& connection_strings)
{
    // only suggest a matching data repository type if all of the connection strings have the same type
    bool suggest_match = !connection_strings.empty();

    for( size_t i = 1; suggest_match && i < connection_strings.size(); i++ )
        suggest_match = ( connection_strings[i].GetType() == connection_strings[i - 1].GetType() );

    return suggest_match ? SuggestMatchingDataRepositoryType(connection_strings.front()) : *this;
}

DataFileDlg& DataFileDlg::WarnIfDifferentDataRepositoryType()
{
    ASSERT(m_type != Type::OpenExisting);
    m_warnIfDifferentDataRepositoryType = true;
    return *this;
}

DataFileDlg& DataFileDlg::SetCreateNewDefaultDataRepositoryType(DataRepositoryType type)
{
    ASSERT(!DataRepositoryHelpers::TypeDoesNotUseFilename(type));
    m_createNewDefaultDataRepositoryType = type;
    UpdateFilters();
    return *this;
}

DataFileDlg& DataFileDlg::AllowMultipleSelections()
{
    ASSERT(m_type == Type::OpenExisting);

    if( !AllowingMultipleSelection() )
    {
        constexpr size_t MultipleSelectionBufferSize = 500 * ( _MAX_PATH + 1 ) + 1;
        m_multipleSelectionBuffer = std::make_unique<TCHAR[]>(MultipleSelectionBufferSize);

        m_ofn.Flags |= OFN_ALLOWMULTISELECT;
        m_ofn.lpstrFile = m_multipleSelectionBuffer.get();
        m_ofn.nMaxFile = MultipleSelectionBufferSize;

        // update the default title
        if( m_title.Compare(OpenExistingSingleFileString) == 0 )
            SetTitle(OpenExistingMultipleFilesString);
    }

    return *this;
}


void DataFileDlg::UpdateInitialDirectory()
{
    // the initial data directory will be calculated as following:
    //      1) if a filename was specified and it exists, use its directory
    //      2) if a filename was specified for a suggested matching data repository type and it exists, use its directory
    //      3) if a dictionary filename was specified and it exists:
    //          a) if a data file was already selected for that data dictionary, use its directory
    //          b) if not, use the dictionary's directory
    //      4) if none of the above, use the directory of th last dta file selected
    m_initialDirectory.Empty();

    auto get_directory_from_connection_string = [this](const ConnectionString& connection_string) -> bool
    {
        if( connection_string.IsFilenamePresent() )
        {
            CString directory = PortableFunctions::PathGetDirectory<CString>(connection_string.GetFilename());

            if( PortableFunctions::FileIsDirectory(directory) )
            {
                m_initialDirectory = directory;
                return true;
            }
        }

        return false;
    };

    auto read_directory_from_registry = [this](const TCHAR* value_name)
    {
        if( GetWinRegistry()->ReadString(value_name, &m_initialDirectory) )
        {
            // make sure that the last used directory still exists
            if( !PortableFunctions::FileIsDirectory(m_initialDirectory) )
                m_initialDirectory.Empty();
        }
    };


    if( !get_directory_from_connection_string(m_initialConnectionString) &&
        !get_directory_from_connection_string(m_suggestedMatchingDataRepositoryTypeConnectionString) &&
        !m_dictionaryFilename.IsEmpty() )
    {
        read_directory_from_registry(GetDictionaryRegistryKeyName());

        if( m_initialDirectory.IsEmpty() && PortableFunctions::FileExists(m_dictionaryFilename) )
            m_initialDirectory = PortableFunctions::PathGetDirectory<CString>(m_dictionaryFilename);
    }

    if( m_initialDirectory.IsEmpty() )
        read_directory_from_registry(LastDataRegistryValueName);

    m_ofn.lpstrInitialDir = !m_initialDirectory.IsEmpty() ? (LPCTSTR)m_initialDirectory : nullptr;
}


void DataFileDlg::UpdateFilters()
{
    // figure out what filter to show
    std::optional<size_t> filter_index;

    if( m_initialConnectionString.IsFilenamePresent() )
    {
        filter_index = m_dataFileFilterManager.GetFilterIndex(m_initialConnectionString);
    }

    else if( m_suggestedMatchingDataRepositoryTypeConnectionString.IsFilenamePresent() )
    {
        filter_index = m_dataFileFilterManager.GetFilterIndex(m_suggestedMatchingDataRepositoryTypeConnectionString);
    }

    // if not matched against any extension, assign the index as following:
    //      1) when opening existing files, show CSPro Data Files
    //      2) otherwise use the create new default data repository type
    if( !filter_index.has_value() )
    {
        if( m_type == Type::OpenExisting )
        {
            filter_index = m_dataFileFilterManager.GetFilterIndex(DataFileFilterManager::CombinedType::CSProData);
        }

        else
        {
            filter_index = m_dataFileFilterManager.GetFilterIndex(m_createNewDefaultDataRepositoryType);
        }

        ASSERT(filter_index.has_value());
    }    

    m_ofn.nFilterIndex = *filter_index + 1; // nFilterIndex is one-based
}


INT_PTR DataFileDlg::DoModal()
{
    CFileDialog::DoModal();
    m_currentDataFileDlg = nullptr;
    return !m_selectedConnectionStrings.empty() ? IDOK : IDCANCEL;
}


IFACEMETHODIMP DataFileDlg::OnFolderChange(IFileDialog* pfd)
{
    if( m_mustSubclassDialog )
    {
        ASSERT(m_currentDataFileDlg == this);

        HWND hWnd = nullptr;
        IUnknown_GetWindow(pfd, &hWnd);

        if( hWnd != nullptr )
        {
            if( !m_initialMultipleSelectionFilename.IsEmpty() )
                GetIFileDialog()->SetFileName(m_initialMultipleSelectionFilename);

            SetWindowSubclass(hWnd, DataFileDlgSubclass, 0, 0);

            // find the combo box with the filters
            std::function<bool(HWND)> subclass_filter_combo_box = [&subclass_filter_combo_box](HWND hSearchWnd) -> bool
            {
                for( hSearchWnd = ::GetWindow(hSearchWnd, GW_CHILD); hSearchWnd != nullptr; hSearchWnd = ::GetWindow(hSearchWnd, GW_HWNDNEXT) )
                {
                    TCHAR this_class_name[200];
                    GetClassName(hSearchWnd, this_class_name, _countof(this_class_name));

                    if( _tcscmp(this_class_name, _T("ComboBox")) == 0 )
                    {
                        // there are two combo boxes (one for the file name) but the filter one has no children
                        if( ::GetWindow(hSearchWnd, GW_CHILD) == nullptr )
                        {
                            SetWindowSubclass(hSearchWnd, DataFileDlgSubclass, 0, 0);
                            return true;
                        }
                    }

                    if( subclass_filter_combo_box(hSearchWnd) )
                        return true;
                }

                return false;
            };

            subclass_filter_combo_box(hWnd);
        }

        m_mustSubclassDialog = false;
    }

    return S_OK;
}


LRESULT CALLBACK DataFileDlg::DataFileDlgSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uidSubclass, DWORD_PTR /*dwData*/)
{
    ASSERT(m_currentDataFileDlg != nullptr);

    auto get_current_filename = []() -> CString
    {
        LPWSTR filename_buffer = nullptr;
        m_currentDataFileDlg->GetIFileDialog()->GetFileName(&filename_buffer);
        CString connection_string_text = filename_buffer;
        CoTaskMemFree(filename_buffer);
        return connection_string_text;
    };

    // override the OK button click to allow invalid characters in the path;
    // while in the file name edit control, this works with the Enter key on open dialogs but not on save dialogs
    if( msg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK )
    {
        ASSERT(m_currentDataFileDlg->m_selectedConnectionStrings.empty());

        CString current_filename = get_current_filename();
        std::vector<ConnectionString> connection_strings;

        // do special processing on a single selection if there is a | character
        auto does_filename_need_special_processing = ( current_filename.Find(_T('|')) >= 0 );

        // when allowing multiple selection, the text may be separated into several entries with quotes
        if( m_currentDataFileDlg->AllowingMultipleSelection() )
        {
            connection_strings = PathHelpers::SplitSingleStringIntoConnectionStrings(current_filename);

            // if there was only one connection string and it doesn't require special processing, don't process it as such
            if( connection_strings.size() == 1 && !does_filename_need_special_processing )
                connection_strings.clear();
        }

        else if( does_filename_need_special_processing )
        {
            connection_strings.emplace_back(current_filename);
        }

        // do the special processing
        if( !connection_strings.empty() )
        {
            for( ConnectionString& connection_string : connection_strings )
            {
                connection_string.AdjustRelativePath(CS2WS(m_currentDataFileDlg->GetFolderPath()));

                // if a filename is present, make sure that it's valid
                if( connection_string.IsFilenamePresent() )
                {
                    CString message;

                    // issue an error if the directory does not exist
                    if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(connection_string.GetFilename())) )
                    {
                        message.Format(_T("%s\nPath does not exist.\nCheck the path and try again."), connection_string.GetFilename().c_str());
                    }

                    else if( !IsValidDataFilename(PortableFunctions::PathGetFilename(connection_string.GetFilename()), m_currentDataFileDlg->AllowingMultipleSelection()) )
                    {
                        message.Format(_T("%s\nThe file name is not valid."), connection_string.GetFilename().c_str());
                    }

                    if( !message.IsEmpty() )
                    {
                        AfxMessageBox(message);
                        return TRUE;
                    }
                }

                if( !m_currentDataFileDlg->ValidateConnectionStringText(connection_string) )
                    return TRUE;
            }

            // if everything is valid, so close the dialog
            m_currentDataFileDlg->m_selectedConnectionStrings = connection_strings;
            ::SendMessage(hWnd, WM_CLOSE, 0, 0);

            return TRUE;
        }
    }

    // if the filter changes, potentially change the extension of the filename
    else if( msg == WM_COMMAND && HIWORD(wParam) == CBN_SELCHANGE )
    {
        if( m_currentDataFileDlg->m_type != Type::OpenExisting )
        {
            ConnectionString connection_string(get_current_filename());
            connection_string.AdjustRelativePath(CS2WS(m_currentDataFileDlg->GetFolderPath()));

            if( !PortableFunctions::FileIsRegular(connection_string.GetFilename()) )
            {
                int combo_box_filter_selected_index = ::SendMessage(hWnd, CB_GETCURSEL, 0, 0);

                if( m_currentDataFileDlg->m_dataFileFilterManager.AdjustConnectionStringFromFilterIndex(connection_string, combo_box_filter_selected_index) )
                    m_currentDataFileDlg->GetIFileDialog()->SetFileName(connection_string.ToStringWithoutDirectory().c_str());
            }
        }
    }

    else if( msg == WM_DESTROY )
    {
        RemoveWindowSubclass(hWnd, DataFileDlgSubclass, uidSubclass);
    }

    return DefSubclassProc(hWnd, msg, wParam, lParam);
}


BOOL DataFileDlg::OnFileNameOK()
{
    ASSERT(m_selectedConnectionStrings.empty());

    std::vector<CString> selected_filenames;

    if( !AllowingMultipleSelection() )
    {
        selected_filenames.emplace_back(GetPathName());
    }

    else
    {
        for( POSITION pos = GetStartPosition(); pos != nullptr; )
            selected_filenames.emplace_back(GetNextPathName(pos));
    }

    for( const CString& filename : selected_filenames )
    {
        ConnectionString connection_string(filename);

        if( !ValidateConnectionStringText(connection_string) )
        {
            m_selectedConnectionStrings.clear();
            return TRUE;
        }

        m_selectedConnectionStrings.emplace_back(std::move(connection_string));
    }

    return FALSE; // FALSE means that the filename is okay
}


bool DataFileDlg::ValidateConnectionStringText(ConnectionString& connection_string)
{
    if( connection_string.IsFilenamePresent() )
    {
        CString extension = PortableFunctions::PathGetFileExtension<CString>(connection_string.GetFilename());

        if( !PortableFunctions::FileExists(connection_string.GetFilename()) )
        {
            // when opening existing files, issue an error if the file doesn't exist (when not using wildcards)
            if( m_type == Type::OpenExisting )
            {
                if( !AllowingMultipleSelection() || !PathHasWildcardCharacters(connection_string.GetFilename()) )
                {
                    CString message;
                    message.Format(_T("%s\nFile not found.\nCheck the file name and try again."), connection_string.GetFilename().c_str());
                    AfxMessageBox(message);
                    return false;
                }
            }

            // for new files, if no extension was provided, potentially add an extension based on the selected filter
            else
            {
                if( extension.IsEmpty() )
                {
                    // nFilterIndex is one-based
                    m_dataFileFilterManager.AdjustConnectionStringFromFilterIndex(connection_string, m_ofn.nFilterIndex - 1);
                }

                // otherwise make sure that the data file doesn't use a reserved extension
                else if( FileExtensions::IsExtensionForbiddenForDataFiles(extension) )
                {
                    AfxMessageBox(FormatText(_T("The file extension .%s is reserved by CSPro and cannot be used for data files."), (LPCTSTR)extension));
                    return false;
                }
            }
        }

        // issue an overwrite warning for new files
        if( m_type == Type::CreateNew && PortableFunctions::FileExists(connection_string.GetFilename()) )
        {
            CString message;
            message.Format(_T("%s already exists\nDo you want to replace it?"), PortableFunctions::PathGetFilename(connection_string.GetFilename()));

            if( AfxMessageBox(message, MB_YESNO) == IDNO )
                return false;
        }
    }

    // issue a warning if the repository type is different than the one to be matched against
    if( m_warnIfDifferentDataRepositoryType && m_suggestedMatchingDataRepositoryTypeConnectionString.IsFilenamePresent() &&
        m_suggestedMatchingDataRepositoryTypeConnectionString.GetType() != connection_string.GetType() &&
        !PortableFunctions::FileExists(connection_string.GetFilename()) )
    {
        CString message;
        message.Format(_T("You have already selected data files with the format %s. Are you sure you want to use the format %s?"),
            ToString(m_suggestedMatchingDataRepositoryTypeConnectionString.GetType()),
            ToString(connection_string.GetType()));

        if( AfxMessageBox(message, MB_YESNO) == IDNO )
            return false;
    }

    // save the directory of the selected data file in the registry
    if( connection_string.IsFilenamePresent() )
    {
        CString directory_name = PortableFunctions::PathGetDirectory<CString>(connection_string.GetFilename());
        GetWinRegistry()->WriteString(LastDataRegistryValueName, directory_name);

        if( !m_dictionaryFilename.IsEmpty() )
            GetWinRegistry()->WriteString(GetDictionaryRegistryKeyName(), directory_name);
    }

    return true;
}


WinRegistry* DataFileDlg::GetWinRegistry()
{
    if( m_winRegistry == nullptr )
    {
        m_winRegistry = std::make_unique<WinRegistry>();
        m_winRegistry->Open(HKEY_CURRENT_USER, _T("Software\\U.S. Census Bureau\\Data Paths"), true);
    }

    return m_winRegistry.get();
}


CString DataFileDlg::GetDictionaryRegistryKeyName() const
{
    ASSERT(!m_dictionaryFilename.IsEmpty());
    return PortableFunctions::PathGetFilenameWithoutExtension<CString>(m_dictionaryFilename).MakeUpper();
}
