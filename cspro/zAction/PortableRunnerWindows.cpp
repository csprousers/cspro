#include "stdafx.h"
#include "PortableRunner.h"
#include <zToolsO/WinClipboard.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/Interapp.h>
#include <zMultimediaO/Icon.h>
#include <zMultimediaO/Image.h>


namespace
{
    CWnd* GetSafeMainWnd()
    {
        return ( AfxGetApp() != nullptr ) ? AfxGetApp()->GetMainWnd() :
                                            nullptr;
    }
}


SharableString ActionInvoker::PortableRunner::Clipboard_GetText()
{
    return WinClipboard::HasText() ? WinClipboard::GetText<std::string>(GetSafeMainWnd()) :
                                     SharableString();
}


void ActionInvoker::PortableRunner::Clipboard_PutText(const std::string_view text_sv)
{
    WinClipboard::PutText(GetSafeMainWnd(), text_sv);
}


ActionInvoker::Result ActionInvoker::PortableRunner::PortableRunner::Path_ShowNativeFileDialog(const std::string& start_directory, const bool open_file_dialog, const bool confirm_overwrite,
                                                                                               const std::optional<std::string>& name, const std::optional<std::string>& filter,
                                                                                               const JsonNode& json_node)
{
    const DWORD flags = OFN_HIDEREADONLY | ( open_file_dialog  ? OFN_FILEMUSTEXIST :
                                             confirm_overwrite ? OFN_OVERWRITEPROMPT :
                                                                 0 );

    const char* const evaluated_filter = filter.has_value() ? filter->c_str() :
                                                              "*.*";
    const std::string filter_text = FormatText("Files (%s)|%s||", evaluated_filter, evaluated_filter);

    CFileDialog file_dlg(open_file_dialog,
                         nullptr,
                         name.has_value() ? TC::ToWide(*name).c_str() : nullptr,
                         flags,
                         TC::ToWide(filter_text).c_str(),
                         GetSafeMainWnd());

    std::unique_ptr<const std::wstring> wide_title;
    file_dlg.m_ofn.lpstrTitle = json_node.Contains(JK::title) ? ( wide_title = std::make_unique<const std::wstring>(TC::ToWide(json_node.Get<std::string_view>(JK::title))) ).get()->c_str() :
                                open_file_dialog              ? L"Select a File to Open" :
                                                                L"Enter a Filename or Select a File to Save";

    const std::wstring wide_start_directory = TC::ToWide(start_directory);
    file_dlg.m_ofn.lpstrInitialDir = wide_start_directory.c_str();

    if( file_dlg.DoModal() != IDOK )
        return Result::Undefined();

    return Result::String(TC::ToUtf8(file_dlg.GetPathName()));
}


void ActionInvoker::PortableRunner::System_CreateShortcut(const std::string& shortcut_id, const std::string& target_file_path,
                                                          const std::optional<std::string>& icon_file_path,
                                                          const std::string& label, const std::optional<std::string>& long_label)
{
    // code based on:
    //     - https://learn.microsoft.com/en-us/windows/win32/shell/links
    //     - https://www.codeproject.com/Articles/11467/How-to-create-short-cuts-link-files

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize has already been called.
    IShellLink* pShellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&pShellLink));

    if( SUCCEEDED(hr) )
    {
        // set the target
        pShellLink->SetPath(TC::ToWide(target_file_path).c_str());

        // when specified, set the comment
        if( long_label.has_value() )
            pShellLink->SetDescription(TC::ToWide(*long_label).c_str());

        // when specified, add the icon
        if( icon_file_path.has_value() )
        {
            // in case the image is temporary, or is not an icon (and will be converted to an icon),
            // store the image as an icon in the AppData...shortcuts directory, with the icon's
            // filename based on the shortcut ID
            std::string shortcuts_directory = Path::Combine(GetAppDataPath(), "shortcuts");
            PortableFunctions::PathMakeDirectories(shortcuts_directory);

            const std::string actual_icon_file_path = PortableFunctions::CreateFilePath(std::move(shortcuts_directory), shortcut_id, "ico");

            // create a copy of the icon...
            if( Multimedia::Icon::IsExtensionIcon(*icon_file_path) )
            {
                PortableFunctions::FileCopyWithExceptions(*icon_file_path, actual_icon_file_path, FileOverwriteFlag::Always);
            }

            // ...or convert an image to an icon
            else
            {
                const std::unique_ptr<Multimedia::Image> image = Multimedia::Image::FromFile(*icon_file_path);
                image->ToFile(actual_icon_file_path);
            }

            hr = pShellLink->SetIconLocation(TC::ToWide(actual_icon_file_path).c_str(), 0);
        }

        if( SUCCEEDED(hr) )
        {
            // Query IShellLink for the IPersistFile interface, used for saving the shortcut in persistent storage.
            IPersistFile* pPersistFile;
            hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&pPersistFile));

            if( SUCCEEDED(hr) )
            {
                // save the link to the desktop using the label as the filename
                const std::string link_file_path = PortableFunctions::CreateFilePath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop),
                                                                                     Path::CreateValidFilename(label),
                                                                                     "lnk");

                hr = pPersistFile->Save(TC::ToWide(link_file_path).c_str(), TRUE);
                pPersistFile->Release();
            }
        }

        pShellLink->Release();
    }

    if( !SUCCEEDED(hr) )
        throw CSProException("The shortcut to '%s' could not be created.", target_file_path.c_str());
}


std::vector<std::tuple<std::string, std::string>> ActionInvoker::PortableRunner::System_ShowSelectDocumentDialog(const std::vector<std::string>& mime_types,
                                                                                                                 const bool multiple)
{
    ASSERT(!mime_types.empty());

    // translate the MIME types to their supported extensions
    std::vector<const char*> extensions;
    bool use_all_extensions = false;

    for( const std::string& mime_type : mime_types )
    {
        if( mime_type == "*/*" )
        {
            use_all_extensions = true;
            break;
        }

        const std::vector<const char*> extensions_for_type = MimeType::GetFileExtensionsFromTypeWithWildcardSupport(mime_type);

        // if not known, default to showing all extensions
        if( extensions_for_type.empty() )
        {
            use_all_extensions = true;
            break;
        }

        extensions.insert(extensions.end(), extensions_for_type.cbegin(), extensions_for_type.cend());
    }

    std::string filter;

    if( use_all_extensions )
    {
        filter = "*.*";
    }

    else
    {
        ASSERT(!extensions.empty());
        RemoveDuplicateStringsInVectorNoCase(extensions);

        filter = SO::CreateSingleStringUsingCallback(extensions,
                                                     [](const char* const extension) { return FileExtensions::CreateWildcard(extension); },
                                                     ";");
    }

    const std::string filter_text = FormatText("Documents (%s)|%s||", filter.c_str(), filter.c_str());
    const std::string documents_directory = GetWindowsSpecialFolder(WindowsSpecialFolder::Documents);

    OpenFileDlg open_file_dlg(0, nullptr, documents_directory, filter_text, GetSafeMainWnd());

    if( multiple )
    {
        open_file_dlg.SetTitle(L"Select One or More Documents")
                     .SetMultiSelectBuffer();
    }

    else
    {
        open_file_dlg.SetTitle(L"Select a Document");
    }

    std::vector<std::tuple<std::string, std::string>> paths_and_names;

    if( open_file_dlg.DoModal() == IDOK )
    {
        for( const std::string& file_path : open_file_dlg.GetFilePaths() )
            paths_and_names.emplace_back(file_path, PortableFunctions::PathGetFilename(file_path));
    }

    return paths_and_names;
}
