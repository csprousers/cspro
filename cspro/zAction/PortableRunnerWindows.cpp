#include "stdafx.h"
#include "PortableRunner.h"
#include <zToolsO/WinClipboard.h>
#include <zUtilO/Filedlg.h>


namespace
{
    CWnd* GetSafeMainWnd()
    {
        return ( AfxGetApp() != nullptr ) ? AfxGetApp()->GetMainWnd() :
                                            nullptr;
    }
}


std::optional<std::wstring> ActionInvoker::PortableRunner::ClipboardGetText()
{
    if( WinClipboard::HasText() )
        return WinClipboard::GetText(GetSafeMainWnd());

    return std::nullopt;
}


void ActionInvoker::PortableRunner::ClipboardPutText(const std::wstring& text)
{
    WinClipboard::PutText(GetSafeMainWnd(), text);
}


ActionInvoker::Result ActionInvoker::PortableRunner::PortableRunner::PathShowNativeFileDialog(const std::wstring& start_directory, bool open_file_dialog, bool confirm_overwrite,
                                                                                              const TCHAR* name, const TCHAR* filter, const JsonNode<wchar_t>& json_node)
{
    const DWORD flags = OFN_HIDEREADONLY | ( open_file_dialog  ? OFN_FILEMUSTEXIST :
                                             confirm_overwrite ? OFN_OVERWRITEPROMPT :
                                                                 0 );

    const std::wstring title = json_node.Contains(JK::title) ? json_node.Get<std::wstring>(JK::title) :
                               open_file_dialog              ? _T("Select a File to Open") :
                                                               _T("Enter a Filename or Select a File to Save");

    if( filter == nullptr )
        filter = _T("*.*");

    const std::wstring filter_text = FormatTextCS2WS(_T("Files (%s)|%s||"), filter, filter);

    CFileDialog file_dlg(open_file_dialog, nullptr, name, flags, filter_text.c_str(), GetSafeMainWnd());
    file_dlg.m_ofn.lpstrTitle = title.c_str();
    file_dlg.m_ofn.lpstrInitialDir = start_directory.c_str();

    if( file_dlg.DoModal() != IDOK )
        return Result::Undefined();

    return Result::String(file_dlg.GetPathName().GetString());
}


std::vector<std::tuple<std::wstring, std::wstring>> ActionInvoker::PortableRunner::SystemShowSelectDocumentDialog(const std::vector<std::wstring>& mime_types, const bool multiple)
{
    ASSERT(!mime_types.empty());

    // translate the MIME types to their supported extensions
    std::vector<const TCHAR*> extensions;
    bool use_all_extensions = false;

    for( const std::wstring& mime_type : mime_types )
    {
        if( mime_type == _T("*/*") )
        {
            use_all_extensions = true;
            break;
        }

        const std::vector<const TCHAR*> extensions_for_type = MimeType::GetFileExtensionsFromTypeWithWildcardSupport(mime_type);

        // if not known, default to showing all extensions
        if( extensions_for_type.empty() )
        {
            use_all_extensions = true;
            break;
        }

        extensions.insert(extensions.end(), extensions_for_type.cbegin(), extensions_for_type.cend());
    }

    std::wstring filter;

    if( use_all_extensions )
    {
        filter = _T("*.*");
    }

    else
    {
        ASSERT(!extensions.empty());
        RemoveDuplicateStringsInVectorNoCase(extensions);

        filter = SO::CreateSingleStringUsingCallback(extensions,
                                                     [](std::wstring extension) { return _T("*.") + extension; },
                                                     _T(";"));
    }

    const std::wstring filter_text = FormatTextCS2WS(_T("Documents (%s)|%s||"), filter.c_str(), filter.c_str());

    CIMSAFileDialog file_dlg(TRUE,
                             nullptr, nullptr,
                             OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | ( multiple ? OFN_ALLOWMULTISELECT : 0 ),
                             filter_text.c_str(),
                             GetSafeMainWnd());

    file_dlg.m_ofn.lpstrTitle = multiple ? _T("Select One or More Documents") :
                                           _T("Select a Document");

    const std::wstring documents_directory = GetWindowsSpecialFolder(WindowsSpecialFolder::Documents);
    file_dlg.m_ofn.lpstrInitialDir = documents_directory.c_str();

    if( multiple )
        file_dlg.SetMultiSelectBuffer();

    std::vector<std::tuple<std::wstring, std::wstring>> paths_and_names;

    if( file_dlg.DoModal() == IDOK )
    {
        for( int i = 0; i < file_dlg.m_aFileName.GetSize(); ++i )
        {
            paths_and_names.emplace_back(CS2WS(file_dlg.m_aFileName[i]),
                                         PortableFunctions::PathGetFilename(file_dlg.m_aFileName[i]));
        }
    }

    return paths_and_names;
}
