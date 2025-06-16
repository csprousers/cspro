#include "StdAfx.h"
#include "DataFileDlg.h"
#include <zToolsO/WinRegistry.h>


namespace
{
    constexpr std::string_view LastDataRegistryValueName_sv = "<Last Data>";

    constexpr const wchar_t* OpenExistingSingleFileString    = L"Select an Existing Data Source";
    constexpr const wchar_t* OpenExistingMultipleFilesString = L"Select Existing Data Source(s)";
}


DataFileDlg* DataFileDlg::m_currentDataFileDlg = nullptr;


DataFileDlg::DataFileDlg(const Type type, const bool add_only_readable_types, ConnectionString connection_string/* = ConnectionString()*/,
                         CWnd* const pParentWnd/* = nullptr*/)
    :   CFileDialog(( type == Type::OpenExisting ),
                    nullptr,
                    connection_string.IsDefined() ? TC::ToWide(connection_string.ToStringWithoutDirectory()).c_str() : nullptr,
                    OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
                    GetDataFileFilterManager(add_only_readable_types).GetFilterText().c_str(),
                    pParentWnd),
        m_type(type),
        m_dataFileFilterManager(GetDataFileFilterManager(add_only_readable_types)),
        m_initialConnectionString(std::move(connection_string)),
        m_warnIfDifferentDataRepositoryType(false),
        m_createNewDefaultDataRepositoryType(DataRepositoryType::SQLite),
        m_mustSubclassDialog(true),
        m_fileDialogEventsHandlerCode(0)
{
    ASSERT(add_only_readable_types || type == Type::CreateNew || type == Type::OpenOrCreate);

    // set up the main properties
    SetTitle(( m_type == Type::OpenExisting ) ? OpenExistingSingleFileString :
             ( m_type == Type::OpenOrCreate ) ? L"Select an Existing or Create a New Data Source" :
                                                L"Create a New Data Source");

    ASSERT(m_currentDataFileDlg == nullptr);
    m_currentDataFileDlg = this;
    GetIFileDialog()->Advise(this, &m_fileDialogEventsHandlerCode);

    UpdateInitialDirectory();
    UpdateFilters();
}


DataFileDlg::DataFileDlg(const Type type, const bool add_only_readable_types, const std::vector<ConnectionString>& connection_strings,
                         CWnd* const pParentWnd/* = nullptr*/)
    :   DataFileDlg(type, add_only_readable_types, connection_strings.empty() ? ConnectionString() : connection_strings.front(), pParentWnd)
{
    // if a repository that does not use files is the only connection string, don't set the multiple selection filename
    if( connection_strings.size() == 1 && !DataRepositoryHelpers::TypeUsesFileResource(connection_strings.front().GetType()) )
        return;

    m_initialMultipleSelectionFilename = UTF8_TODO::GetCString(PathHelpers::CreateSingleStringFromConnectionStrings(connection_strings, true));
}


DataFileDlg::~DataFileDlg()
{
    GetIFileDialog()->Unadvise(m_fileDialogEventsHandlerCode);
}


const DataFileFilterManager& DataFileDlg::GetDataFileFilterManager(const bool add_only_readable_types)
{
    return DataFileFilterManager::Get(DataFileFilterManager::UseType::FileChooserDlg, add_only_readable_types);
}


IFileDialog* DataFileDlg::GetIFileDialog()
{
    return ( m_type == Type::OpenExisting ) ? static_cast<IFileDialog*>(GetIFileOpenDialog()) :
                                              static_cast<IFileDialog*>(GetIFileSaveDialog());
}


DataFileDlg& DataFileDlg::SetTitle(std::wstring title)
{
    m_title = std::move(title);
    m_ofn.lpstrTitle = m_title.c_str();
    return *this;
}


DataFileDlg& DataFileDlg::SetTitle(const std::string_view title_sv)
{
    return SetTitle(TC::ToWide(title_sv));
}


DataFileDlg& DataFileDlg::SetDictionaryFilePath(std::string dictionary_file_path)
{
    m_dictionaryFilePath = std::move(dictionary_file_path);
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
    ASSERT(DataRepositoryHelpers::TypeUsesFileResource(type));
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
        m_multipleSelectionBuffer = std::make_unique_for_overwrite<wchar_t[]>(MultipleSelectionBufferSize);
        m_multipleSelectionBuffer[0] = '\0';

        m_ofn.Flags |= OFN_ALLOWMULTISELECT;
        m_ofn.lpstrFile = m_multipleSelectionBuffer.get();
        m_ofn.nMaxFile = MultipleSelectionBufferSize;

        // update the default title
        if( m_title == OpenExistingSingleFileString )
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
    std::string initial_directory;

    auto get_directory_from_connection_string = [&](const ConnectionString& connection_string)
    {
        if( connection_string.HasFilePath() )
        {
            std::string directory = PortableFunctions::PathGetDirectory(connection_string.GetFilePath());

            if( PortableFunctions::FileIsDirectory(directory) )
            {
                initial_directory = std::move(directory);
                return true;
            }
        }

        return false;
    };

    auto read_directory_from_registry = [&](const std::string_view value_name_sv)
    {
        if( GetWinRegistry()->ReadString(value_name_sv, initial_directory) )
        {
            // make sure that the last used directory still exists
            if( !PortableFunctions::FileIsDirectory(initial_directory) )
                initial_directory.clear();
        }
    };


    if( !get_directory_from_connection_string(m_initialConnectionString) &&
        !get_directory_from_connection_string(m_suggestedMatchingDataRepositoryTypeConnectionString) &&
        !m_dictionaryFilePath.empty() )
    {
        read_directory_from_registry(GetDictionaryRegistryKeyName());

        if( initial_directory.empty() && PortableFunctions::FileIsRegular(m_dictionaryFilePath) )
            initial_directory = PortableFunctions::PathGetDirectory(m_dictionaryFilePath);
    }

    if( initial_directory.empty() )
        read_directory_from_registry(LastDataRegistryValueName_sv);

    m_initialDirectory = TC::ToWide(initial_directory);
    m_ofn.lpstrInitialDir = !m_initialDirectory.empty() ? m_initialDirectory.c_str() : nullptr;
}


void DataFileDlg::UpdateFilters()
{
    // figure out what filter to show
    std::optional<size_t> filter_index;

    if( m_initialConnectionString.HasFilePath() )
    {
        filter_index = m_dataFileFilterManager.GetFilterIndex(m_initialConnectionString);
    }

    else if( m_suggestedMatchingDataRepositoryTypeConnectionString.HasFilePath() )
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
                    wchar_t this_class_name[200];
                    GetClassName(hSearchWnd, this_class_name, _countof(this_class_name));

                    if( wcscmp(this_class_name, L"ComboBox") == 0 )
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


bool DataFileDlg::IsValidDataFilename(const std::string& filename, const bool allow_wildcards)
{
    // mostly based on https://stackoverflow.com/questions/1976007/what-characters-are-forbidden-in-windows-and-linux-directory-names
    if( filename.empty() )
        return false;

    auto filename_itr = filename.crbegin();

    // filenames cannot end in a space or dot
    if( *filename_itr == ' ' || *filename_itr == '.' )
        return false;

    // filenames cannot have invalid characters
    for( ; filename_itr != filename.crend(); ++filename_itr )
    {
        if( *filename_itr < 32 )
            return false;
    }

    if( ( filename.find_first_of(Path::InvalidCharacters) != std::string::npos ) ||
        ( !allow_wildcards && Path::HasWildcardCharacters(filename) ) )
    {
        return false;
    }

    return true;
}


LRESULT CALLBACK DataFileDlg::DataFileDlgSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uidSubclass, DWORD_PTR /*dwData*/)
{
    ASSERT(m_currentDataFileDlg != nullptr);

    auto get_current_filename = []()
    {
        wchar_t* buffer = nullptr;
        m_currentDataFileDlg->GetIFileDialog()->GetFileName(&buffer);
        std::string current_filename = TC::ToUtf8(buffer);
        CoTaskMemFree(buffer);
        return current_filename;
    };

    // override the OK button click to allow invalid characters in the path;
    // while in the file name edit control, this works with the Enter key on open dialogs but not on save dialogs
    if( msg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK )
    {
        try
        {
            ASSERT(m_currentDataFileDlg->m_selectedConnectionStrings.empty());

            const std::string current_filename = get_current_filename();

            std::vector<ConnectionString> connection_strings;

            // when allowing multiple selection, the text may be separated into several entries with quotes
            if( m_currentDataFileDlg->AllowingMultipleSelection() )
            {
                connection_strings = PathHelpers::SplitSingleStringIntoConnectionStrings(current_filename);
            }

            else if( !SO::IsWhitespace(current_filename) )
            {
                connection_strings.emplace_back(current_filename);
            }

            // at least one connection string must be specified
            if( connection_strings.empty() )
                return TRUE;

            // process each connection string
            for( ConnectionString& connection_string : connection_strings )
            {
                // if a file path is present, make sure that it is valid
                if( connection_string.HasFilePath() )
                {
                    connection_string.AdjustRelativePath(TC::ToUtf8(m_currentDataFileDlg->GetFolderPath()));

                    // if the user specifies a directory, let the default processing continue
                    // so that the user can enter text like ".." to navigate directories
                    if( PortableFunctions::FileIsDirectory(connection_string.GetFilePath()) &&
                        connection_strings.size() == 1 )
                    {
                        return DefSubclassProc(hWnd, msg, wParam, lParam);
                    }

                    // issue an error if the directory does not exist
                    if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) )
                        throw CSProException("%s\nPath does not exist.\nCheck the path and try again.", connection_string.GetFilePath().c_str());

                    if( !IsValidDataFilename(PortableFunctions::PathGetFilename(connection_string.GetFilePath()), m_currentDataFileDlg->AllowingMultipleSelection()) )
                        throw CSProException("%s\nThe file name is not valid.", connection_string.GetFilePath().c_str());
                }

                if( !m_currentDataFileDlg->ValidateConnectionStringText(connection_string) )
                    return TRUE;
            }

            // if everything is valid, close the dialog
            m_currentDataFileDlg->m_selectedConnectionStrings = std::move(connection_strings);
            ::SendMessage(hWnd, WM_CLOSE, 0, 0);
        }

        catch( const CSProException& exception)
        {
            ErrorMessage::Display(exception);
        }

        return TRUE;
    }

    // if the filter changes, potentially change the extension of the filename
    else if( msg == WM_COMMAND && HIWORD(wParam) == CBN_SELCHANGE )
    {
        if( m_currentDataFileDlg->m_type != Type::OpenExisting )
        {
            ConnectionString connection_string(get_current_filename());

            if( connection_string.HasFilePath() )
            {
                connection_string.AdjustRelativePath(TC::ToUtf8(m_currentDataFileDlg->GetFolderPath()));

                if( !PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
                {
                    const int combo_box_filter_selected_index = ::SendMessage(hWnd, CB_GETCURSEL, 0, 0);

                    if( m_currentDataFileDlg->m_dataFileFilterManager.AdjustConnectionStringFromFilterIndex(connection_string, combo_box_filter_selected_index) )
                        m_currentDataFileDlg->GetIFileDialog()->SetFileName(TC::ToWide(connection_string.ToStringWithoutDirectory()).c_str());
                }
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

    auto add_file = [&](const CString& file_path)
    {
        ConnectionString& connection_string = m_selectedConnectionStrings.emplace_back(TC::ToUtf8(file_path));

        if( !ValidateConnectionStringText(connection_string) )
            m_selectedConnectionStrings.clear();

        return !m_selectedConnectionStrings.empty();
    };

    if( !AllowingMultipleSelection() )
    {
        add_file(GetPathName());
    }

    else
    {
        for( POSITION pos = GetStartPosition(); pos != nullptr && add_file(GetNextPathName(pos)); )
        {
        }
    }

    return !m_selectedConnectionStrings.empty() ? FALSE : TRUE; // FALSE means that the filename is okay
}


bool DataFileDlg::ValidateConnectionStringText(ConnectionString& connection_string)
{
    if( connection_string.HasFilePath() )
    {
        const std::string extension = PortableFunctions::PathGetFileExtension(connection_string.GetFilePath());

        if( !PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
        {
            // when opening existing files, issue an error if the file doesn't exist (when not using wildcards)
            if( m_type == Type::OpenExisting )
            {
                if( !AllowingMultipleSelection() || !Path::HasWildcardCharacters(connection_string.GetFilePath()) )
                {
                    AfxMessageBox(FormatText("%s\nFile not found.\nCheck the file name and try again.", connection_string.GetFilePath().c_str()));
                    return false;
                }
            }

            // for new files, if no extension was provided, potentially add an extension based on the selected filter
            else
            {
                if( extension.empty() )
                {
                    // nFilterIndex is one-based
                    m_dataFileFilterManager.AdjustConnectionStringFromFilterIndex(connection_string, m_ofn.nFilterIndex - 1);
                }

                // otherwise make sure that the data file doesn't use a reserved extension
                else if( FileExtensions::IsExtensionForbiddenForDataFiles(extension) )
                {
                    AfxMessageBox(FormatText("The file extension '.%s' is reserved by CSPro and cannot be used for data files.", extension.c_str()));
                    return false;
                }
            }
        }

        // issue an overwrite warning for new files
        if( m_type == Type::CreateNew && PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
        {
            const std::string message = FormatText("%s already exists\nDo you want to replace it?",
                                                   PortableFunctions::PathGetFilename(connection_string.GetFilePath()).c_str());

            if( AfxMessageBox(message, MB_YESNO) == IDNO )
                return false;
        }
    }

    // issue a warning if the repository type is different than the one to be matched against
    if( m_warnIfDifferentDataRepositoryType &&
        m_suggestedMatchingDataRepositoryTypeConnectionString.GetType() != connection_string.GetType() &&
        m_suggestedMatchingDataRepositoryTypeConnectionString.HasFilePath() && connection_string.HasFilePath() &&
        !PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
    {
        const std::string message = FormatText("You have already selected data files with the format %s. Are you sure you want to use the format %s?",
                                               ToString(m_suggestedMatchingDataRepositoryTypeConnectionString.GetType()),
                                               ToString(connection_string.GetType()));

        if( AfxMessageBox(message, MB_YESNO) == IDNO )
            return false;
    }

    // save the directory of the selected data file in the registry
    if( connection_string.HasFilePath() )
    {
        const std::string directory = PortableFunctions::PathGetDirectory(connection_string.GetFilePath());
        GetWinRegistry()->WriteString(LastDataRegistryValueName_sv, directory);

        if( !m_dictionaryFilePath.empty() )
            GetWinRegistry()->WriteString(GetDictionaryRegistryKeyName(), directory);
    }

    return true;
}


WinRegistry* DataFileDlg::GetWinRegistry()
{
    if( m_winRegistry == nullptr )
    {
        m_winRegistry = std::make_unique<WinRegistry>();
        m_winRegistry->Open(HKEY_CURRENT_USER, L"Software\\U.S. Census Bureau\\Data Paths", true);
    }

    return m_winRegistry.get();
}


std::string DataFileDlg::GetDictionaryRegistryKeyName() const
{
    ASSERT(!m_dictionaryFilePath.empty());
    return SO::ToUpper(Path::GetFilenameWithoutExtension(m_dictionaryFilePath));
}


std::optional<ConnectionString> DataFileDlg::ShowDialogFromWinForms(CWnd* const pParentWnd, const Type type, const bool add_only_readable_types,
                                                                    ConnectionString connection_string)
{
    AfxSetResourceHandle(zBridgeODLL.hModule);

    DataFileDlg data_file_dlg(type, add_only_readable_types, std::move(connection_string), pParentWnd);

    const INT_PTR result = data_file_dlg.DoModal();

    return ( result == IDOK ) ? std::make_optional(data_file_dlg.GetConnectionString()) :
                                std::nullopt;
}
