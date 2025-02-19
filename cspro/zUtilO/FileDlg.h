#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// FileDlg / OpenFileDlg / SaveFileDlg
//
// A wrapper around CFileDialog that supports working with UTF-8 paths.
//
// There is some additional functionality compared CFileDialog:
//
//     - The constructor's parameters are in a different order.
//
//     - If no flags are specified (passing 0), the following flags are
//       applied by default:
//
//           - Open: OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
//           - Save: OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
//
//     - If the filename argument used in the constructor is a directory
//       that exists, a trailing slash will be added so that a string
//       like "C:\\Directory" isn't treated as a file named "Directory"
//       to be placed in C:\.
//
//     - If default extension(s) are given, an error is shown if the filename
//       contains an extension that does not match one of the extensions. To
//       disable this behavior, call DisableExtensionCheck.
//
// If migrating from CIMSAFileDialog, there are some differences:
//
//     - For open dialogs, CIMSAFileDialog always checks for the existence
//       of file(s). To get this same behavior, use the flag
//       OFN_FILEMUSTEXIST. This is part of the default open flags.
//
//     - For save dialogs, CIMSAFileDialog tried to create a file at the
//       selected file path, displaying an error if this was not possible.
//       FileDlg does not do this.
//
//     - CIMSAFileDialog allowed * to be used as a default extension. This is
//       no longer allowed and DisableExtensionCheck should be called
//       instead if all extensions are valid.
//
//     - Calling SetMultiSelectBuffer will automatically add the
//       OFN_ALLOWMULTISELECT flag.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO FileDlg : public CFileDialog
{
public:
    using StringType = std::variant<std::monostate, const wchar_t*, std::string_view>;

    static constexpr DWORD DefaultOpenFlags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    static constexpr DWORD DefaultSaveFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    FileDlg(bool open_dialog,
            DWORD flags = 0,
            StringType default_extension = std::monostate(), // this can contain one or more extensions
            StringType filename = std::monostate(),          // this can be a file or directory path, or just a filename
            StringType filter = std::monostate(),
            CWnd* pParentWnd = nullptr,
            std::vector<std::unique_ptr<std::wstring>> wide_strings = std::vector<std::unique_ptr<std::wstring>>()); // this is used internally for string conversions

    // Sets the dialog title.
    FileDlg& SetTitle(StringType title);

    // Disables the checking of file extensions when default_extension was set in the constructor.
    FileDlg& DisableExtensionCheck();

    // Sets the initial dictionary.
    FileDlg& SetInitialDirectory(std::wstring directory);
    FileDlg& SetInitialDirectory(std::string_view directory_sv) { return SetInitialDirectory(TC::ToWide(directory_sv)); }

    // If pMDIFrameWnd is not null and a document is open, its directory will be used as the initial directory.
    FileDlg& UseInitialDirectoryOfActiveDocument(CMDIFrameWnd* pMDIFrameWnd);

    // Establishes the size of the multiple selection buffer. OFN_ALLOWMULTISELECT will be added to the flags.
    FileDlg& SetMultiSelectBuffer(size_t max_files = 250);

    // Returns a single selected path.
    const std::string& GetFilePath() const;

    // Returns the selected paths (when using multiple selection).
    const std::vector<std::string>& GetFilePaths() const { return m_filePaths; }

protected:
    BOOL OnFileNameOK() override;

private:
    static const wchar_t* ConvertString(std::vector<std::unique_ptr<std::wstring>>& wide_strings, const StringType& optional_string);
    static const wchar_t* ConvertString(std::vector<std::unique_ptr<std::wstring>>& wide_strings, std::wstring text);

    const wchar_t* StoreString(std::wstring text)        { return ConvertString(m_wideStrings, std::move(text)); }
    const wchar_t* StoreString(std::string_view text_sv) { return StoreString(TC::ToWide(text_sv)); }

    static const wchar_t* ConvertConstructorFilename(std::vector<std::unique_ptr<std::wstring>>& wide_strings, const StringType& filename);

    void CheckExtension(const std::string& file_path);

    // this is not implemented but is added here to cause a compiler error since GetFilePath should be used instead
    CString GetPathName() const;

private:
    std::vector<std::unique_ptr<std::wstring>> m_wideStrings;

    bool m_checkExtensions;
    std::optional<std::vector<std::string>> m_validExtensions;

    std::unique_ptr<wchar_t[]> m_multiSelectBuffer;

    std::vector<std::string> m_filePaths;
};



// --------------------------------------------------------------------------
// OpenFileDlg
// --------------------------------------------------------------------------

class OpenFileDlg : public FileDlg
{
public:
    template<typename... Args>
    OpenFileDlg(Args&&... args) : FileDlg(true, std::forward<Args>(args)...) { }
};



// --------------------------------------------------------------------------
// SaveFileDlg
// --------------------------------------------------------------------------

class SaveFileDlg : public FileDlg
{
public:
    template<typename... Args>
    SaveFileDlg(Args&&... args) : FileDlg(false, std::forward<Args>(args)...) { }
};
