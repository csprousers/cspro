#pragma once

//***************************************************************************
//  File name: FileUtil.h
//
//  Description:
//       Misc utility functions for working with files.
//
//***************************************************************************

#include <zUtilO/zUtilO.h>

// generate a unique filename based on a template
// Allows you to mimick the windows exporer foo, copy of foo, copy (2) of foo
// scheme.  Place () in your template where you want it to place the number.
// The parens will be removed if they aren't needed.
// Returns full path to new file (including file name).
CLASS_DECL_ZUTILO CString GenUniqueFileNameFromTemplate(LPCTSTR sPath, // path to save in, no filename
                                                        LPCTSTR sTemplate); // e.g. "Copy () of Foo"


// generate a unique filename with the "copy of", "copy (2) of" scheme
// based on an existing filename.  Pass in full path (including file name).
// Returns full path including name with appropriate version of
// copy of prepended to name e.g pass "C:\docs\stuff\foo.txt" and get back
// "C:\docs\stuff\Copy of foo.txt" or pass in "C:\docs\stuff\Copy of foo.txt"
// and get back "C:\docs\stuff\Copy (2) of foo.txt".
CLASS_DECL_ZUTILO CString GenUniqueFileName(NullTerminatedString sFullPath);


// a wrapper around the SHBrowseForFolder method
CLASS_DECL_ZUTILO std::optional<std::string> RunSHBrowseForFolder(HWND hWnd, UINT flags, std::string_view title_sv, const std::string& initial_path);

// display a "select folder" dialog
inline std::optional<std::string> SelectFolderDialog(HWND hWnd, std::string_view title_sv, const std::string& initial_path = std::string())
{
    return RunSHBrowseForFolder(hWnd, BIF_USENEWUI | BIF_SHAREABLE, title_sv, initial_path);
}

// display a "select file or folder" dialog
inline std::optional<std::string> SelectFileOrFolderDialog(HWND hWnd, std::string_view title_sv, const std::string& initial_path = std::string())
{
    return RunSHBrowseForFolder(hWnd, BIF_USENEWUI | BIF_SHAREABLE | BIF_NONEWFOLDERBUTTON | BIF_BROWSEINCLUDEFILES, title_sv, initial_path);
}
