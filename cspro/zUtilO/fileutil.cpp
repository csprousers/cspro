//***************************************************************************
//  File name: FileUtil.cpp
//
//  Description:
//       Misc utility functions for working with files.
//
//***************************************************************************
#include "StdAfx.h"
#include "FileUtil.h"

// generate a unique filename based on a template
// Allows you to mimick the windows exporer foo, copy of foo, copy (2) of foo
// scheme.  Place () in your template where you want it to place the number.
// The parens will be removed if they aren't needed.
// Returns full path to new file (including file name).
CString GenUniqueFileNameFromTemplate(LPCTSTR sPath, // path to save in, no filename
                                      LPCTSTR sTemplate) // e.g. "Copy () of Foo"
{
    TCHAR sUniqueFilePathW[MAX_PATH];
    CT2W sFileNameTemplateW(sTemplate);
    CT2W sPathTemplateW(sPath + CString(PATH_STRING) + sTemplate);

    PathYetAnotherMakeUniqueName(sUniqueFilePathW,
                                 sPathTemplateW,
                                 nullptr,
                                 sFileNameTemplateW);

    CW2T sUniqueFilePathA(sUniqueFilePathW);

    return CString(sUniqueFilePathA);
}

// generate a unique filename with the "copy of", "copy (2) of" scheme
// based on an existing filename.  Pass in full path (including file name).
// Returns full path including name with appropriate version of
// copy of prepended to name e.g pass "C:\docs\stuff\foo.txt" and get back
// "C:\docs\stuff\Copy of foo.txt" or pass in "C:\docs\stuff\Copy of foo.txt"
// and get back "C:\docs\stuff\Copy (2) of foo.txt".
CString GenUniqueFileName(NullTerminatedString sFullPath)
{
    CString sNewFileNameTemplate = CString(_T("Copy () of ")) + PathFindFileName(sFullPath.c_str());
    CString sPath = sFullPath;
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    return GenUniqueFileNameFromTemplate(sPath, sNewFileNameTemplate);
}



namespace
{
    int CALLBACK RunSHBrowseForFolder_Callback(HWND hWnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
    {
        if( uMsg == BFFM_INITIALIZED )
            SendMessage(hWnd, BFFM_SETEXPANDED, TRUE, lpData);

        return 0;
    }
}


std::optional<std::wstring> RunSHBrowseForFolder(HWND hWnd, UINT flags, const TCHAR* title/* = nullptr*/, const TCHAR* initial_path/* = nullptr*/)
{
    if( FAILED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) )
        return std::nullopt;

    IMalloc* pMalloc = nullptr;

    if( FAILED(SHGetMalloc(&pMalloc)) )
        return std::nullopt;

    std::wstring path(MAX_PATH, '\0');

    BROWSEINFO browse_info;
    memset(&browse_info, 0, sizeof(browse_info));

    browse_info.hwndOwner = hWnd;
    browse_info.pszDisplayName = path.data();
    browse_info.ulFlags = flags;
    browse_info.lpszTitle = title;

    if( initial_path != nullptr && PortableFunctions::FileIsDirectory(initial_path) )
    {
        browse_info.lpfn = RunSHBrowseForFolder_Callback;
        browse_info.lParam = reinterpret_cast<LPARAM>(initial_path);
    }

    ITEMIDLIST* pItemList = ::SHBrowseForFolder(&browse_info);

    if( pItemList != nullptr )
    {
        ::SHGetPathFromIDList(pItemList, path.data());
        pMalloc->Free(pItemList);
    }

    pMalloc->Release();
    ::CoUninitialize();

    path.resize(_tcslen(path.data()));

    if( !path.empty() )
        return path;

    return std::nullopt;
}
