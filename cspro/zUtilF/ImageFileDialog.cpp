#include "StdAfx.h"
#include "ImageFileDialog.h"
#include <zToolsO/WinSettings.h>


ImageFileDialog::ImageFileDialog(LPCTSTR lpszFileName)
    :   CFileDialog(TRUE, nullptr, lpszFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("Images|*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff|All Files (*.*)|*.*||"))
{
    m_ofn.lpstrTitle = _T("Select Image File");

    m_canIncludeAsResource = ( AfxGetMainWnd()->SendMessage(UWM::UtilF::CanAddResourceFolder) != 0 );

    if( m_canIncludeAsResource )
    {
        BOOL include_as_resource = WinSettings::Read<DWORD>(WinSettings::Type::AddImageToResourceFolder, TRUE);
        AddCheckButton(IDC_CHECK_INCLUDE_AS_RESOURCE, "Include in pen file as image resource?", include_as_resource);
    }
}


INT_PTR ImageFileDialog::DoModal()
{
    INT_PTR result = CFileDialog::DoModal();

    if( result == IDOK )
    {
        BOOL include_as_resource = FALSE;

        if( m_canIncludeAsResource )
        {
            GetCheckButtonState(IDC_CHECK_INCLUDE_AS_RESOURCE, include_as_resource);
            WinSettings::Write<DWORD>(WinSettings::Type::AddImageToResourceFolder, include_as_resource);
        }

        m_pathName = CFileDialog::GetPathName();

        if( include_as_resource )
            CopyToResourceFolder();
    }

    return result;
}


void ImageFileDialog::CopyToResourceFolder()
{
    CString resource_folder_name = _T("Resources");
    CString resource_folder_directory;

    LRESULT result = AfxGetMainWnd()->SendMessage(UWM::UtilF::CreateResourceFolder,
        (WPARAM)&resource_folder_name,
        (LPARAM)&resource_folder_directory);

    if( result == 0 )
    {
        m_pathName.Empty();
        return;
    }

    CString full_path = PortableFunctions::PathAppendToPath(resource_folder_directory, _T("Images"));
    full_path = PortableFunctions::PathAppendToPath(full_path, PortableFunctions::PathGetFilename(m_pathName));
    PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(full_path));
    PortableFunctions::FileCopy(m_pathName, full_path, false);
    m_pathName = full_path;
}
