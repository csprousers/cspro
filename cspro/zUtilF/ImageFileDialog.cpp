#include "StdAfx.h"
#include "ImageFileDialog.h"
#include <zToolsO/WinSettings.h>


ImageFileDialog::ImageFileDialog(const LPCTSTR lpszFileName/* = nullptr*/, CWnd* const pParentWnd/* = nullptr*/)
    :   CFileDialog(TRUE, nullptr, lpszFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    L"Images|*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff|All Files (*.*)|*.*||",
                    pParentWnd),
    m_canIncludeAsResource(( WindowsDesktopMessage::Send(UWM::UtilF::CanAddResources) != 0 ))
{
    m_ofn.lpstrTitle = L"Select Image File";

    if( m_canIncludeAsResource )
    {
        const BOOL include_as_resource = WinSettings::Read<DWORD>(WinSettings::Type::AddImageToResourceFolder, TRUE);
        AddCheckButton(IDC_CHECK_INCLUDE_AS_RESOURCE, "Copy to 'Resources' directory as an application resource?", include_as_resource);
    }
}


INT_PTR ImageFileDialog::DoModal()
{
    const INT_PTR result = CFileDialog::DoModal();

    if( result == IDOK )
    {
        BOOL include_as_resource = FALSE;

        if( m_canIncludeAsResource )
        {
            GetCheckButtonState(IDC_CHECK_INCLUDE_AS_RESOURCE, include_as_resource);
            WinSettings::Write<DWORD>(WinSettings::Type::AddImageToResourceFolder, include_as_resource);
        }

        if( include_as_resource )
        {
            const std::string file_path = TC::ToUtf8(CFileDialog::GetPathName());
            std::string file_path_in_resources_directory;

            if( WindowsDesktopMessage::Send(UWM::UtilF::CopyToResourceDirectory, &file_path, &file_path_in_resources_directory) == 1 )
                wcsncpy_s(m_szFileName, _countof(m_szFileName), TC::ToWide(file_path_in_resources_directory).c_str(), _TRUNCATE);
        }
    }

    return result;
}
