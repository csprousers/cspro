#include "StdAfx.h"
#include "FileDlg.h"


FileDlg::FileDlg(const bool open_dialog,
                 const DWORD flags/* = 0*/,
                 const StringType default_extension/* = std::monostate()*/,
                 const StringType filename/* = std::monostate()*/,
                 const StringType filter/* = std::monostate()*/,
                 CWnd* const pParentWnd/* = nullptr*/,
                 std::vector<std::unique_ptr<std::wstring>> wide_strings/* = std::vector<std::unique_ptr<std::wstring>>()*/)
    :   CFileDialog(open_dialog,
                    ConvertString(wide_strings, default_extension),
                    ConvertConstructorFilename(wide_strings, filename),
                    ( flags != 0 ) ? flags : open_dialog ? DefaultOpenFlags : DefaultSaveFlags,
                    ConvertString(wide_strings, filter),
                    pParentWnd),
        m_wideStrings(std::move(wide_strings)),
        m_checkExtensions(m_ofn.lpstrDefExt != nullptr)
{
    // the CFileDialog constructor makes copies of the filename and filter, so the only
    // string that we needed to keep a copy of is technically the default extension
}


const wchar_t* FileDlg::ConvertString(std::vector<std::unique_ptr<std::wstring>>& wide_strings, const StringType& optional_string)
{
    if( std::holds_alternative<std::monostate>(optional_string) )
    {
        return nullptr;
    }

    else if( std::holds_alternative<const wchar_t*>(optional_string) )
    {
        return std::get<const wchar_t*>(optional_string);
    }

    else
    {
        ASSERT(std::holds_alternative<std::string_view>(optional_string));
        return ConvertString(wide_strings, TC::ToWide(std::get<std::string_view>(optional_string)));
    }
}


const wchar_t* FileDlg::ConvertString(std::vector<std::unique_ptr<std::wstring>>& wide_strings, std::wstring text)
{
    return wide_strings.emplace_back(std::make_unique<std::wstring>(std::move(text)))->c_str();
}


const wchar_t* FileDlg::ConvertConstructorFilename(std::vector<std::unique_ptr<std::wstring>>& wide_strings, const StringType& filename)
{
    const wchar_t* const converted_filename = ConvertString(wide_strings, filename);

    if( converted_filename != nullptr && *converted_filename != '\0' )
    {
        const std::wstring_view directory_check_sv = converted_filename;

        if( directory_check_sv.back() != Path::NativeSlashChar &&
            PortableFunctions::FileIsDirectory(converted_filename) )
        {
            constexpr wchar_t SlashString[] = { Path::NativeSlashChar, '\0' };
            return ConvertString(wide_strings, std::wstring(directory_check_sv) + SlashString);
        }
    }

    return converted_filename;
}


FileDlg& FileDlg::SetTitle(const StringType title)
{
    m_ofn.lpstrTitle = ConvertString(m_wideStrings, title);
    return *this;
}


FileDlg& FileDlg::DisableExtensionCheck()
{
    m_checkExtensions = false;
    return *this;
}


FileDlg& FileDlg::SetInitialDirectory(std::wstring directory)
{
    ASSERT(m_ofn.lpstrInitialDir == nullptr);

    if( !directory.empty() )
        m_ofn.lpstrInitialDir = StoreString(std::move(directory));

    return *this;
}


FileDlg& FileDlg::UseInitialDirectoryOfActiveDocument(CMDIFrameWnd* const pMDIFrameWnd)
{
    ASSERT(m_ofn.lpstrInitialDir == nullptr);

    if( pMDIFrameWnd != nullptr )
    {
        CMDIChildWnd* const pActiveWnd = pMDIFrameWnd->MDIGetActive();

        if( pActiveWnd != nullptr )
        {
            const CDocument* const pDoc = pActiveWnd->GetActiveDocument();

            if( pDoc != nullptr )
                SetInitialDirectory(PortableFunctions::PathGetDirectory(pDoc->GetPathName()));
        }
    }

    return *this;
}


FileDlg& FileDlg::SetMultiSelectBuffer(const size_t max_files/* = 250*/)
{
    m_ofn.Flags |= OFN_ALLOWMULTISELECT;

    const size_t buffer_size = max_files * ( _MAX_PATH + 1 ) + 1;
    m_multiSelectBuffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_size);
    m_multiSelectBuffer[0] = '\0';

    m_ofn.lpstrFile = m_multiSelectBuffer.get();
    m_ofn.nMaxFile = buffer_size;

    return *this;
}


BOOL FileDlg::OnFileNameOK()
{
    constexpr BOOL Valid = FALSE;
    constexpr BOOL NotValid = TRUE;

    if( __super::OnFileNameOK() == NotValid )
        return NotValid;

    // store the selected paths and do any additional checks
    m_filePaths.clear();

    try
    {
        POSITION pos = GetStartPosition();

        while( pos != nullptr )
        {
            const std::string& file_path = m_filePaths.emplace_back(TC::ToUtf8(GetNextPathName(pos)));

            if( m_checkExtensions )
                CheckExtension(file_path);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return NotValid;
    }

    ASSERT(!m_filePaths.empty());
    ASSERT(m_filePaths.size() == 1 || ( m_ofn.Flags & OFN_ALLOWMULTISELECT ) != 0);

    return Valid;
}


void FileDlg::CheckExtension(const std::string& file_path)
{
    ASSERT(m_checkExtensions);

    // lazily calculate the valid extensions
    if( !m_validExtensions.has_value() )
    {
        m_validExtensions = SO::SplitString(TC::ToUtf8(m_ofn.lpstrDefExt), ", ", true, false);

        ASSERT(!m_validExtensions->empty());
        ASSERT(std::find_if(m_validExtensions->cbegin(), m_validExtensions->cend(),
                            [&](const std::string& valid_extension) { return ( valid_extension == "*" ); }) == m_validExtensions->cend());
    }

    const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

    const auto& lookup = std::find_if(m_validExtensions->cbegin(), m_validExtensions->cend(),
                                      [&](const std::string& valid_extension) { return SO::EqualsNoCase(extension, valid_extension); });

    if( lookup != m_validExtensions->cend() )
        return;

    const std::string message =
        ( m_validExtensions->size() == 1 ) ? ( "The file must have the extension: " + m_validExtensions->front() ) :
                                             ( "The file must have one of the extensions: " + SO::CreateSingleString(*m_validExtensions) );
    throw CSProException(message);
}


const std::string& FileDlg::GetFilePath() const
{
    return !m_filePaths.empty() ? m_filePaths.front() :
                                  ReturnProgrammingError(SO::Empty_string);
}
