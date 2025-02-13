#pragma once

#include <zUtilF/zUtilF.h>


/// <summary>
/// A CFileDialog class for loading image files that adds a checkbox to indicate whether
/// or not to save the image as in the resource folder. If the user checks that box, a resource folder
/// is created and the image file is copied into it.
/// </summary>
class CLASS_DECL_ZUTILF ImageFileDialog : public CFileDialog
{
public:
    ImageFileDialog(LPCTSTR lpszFileName = NULL);

    INT_PTR DoModal() override;

    const CString& GetPathName() const { return m_pathName; }

private:
    void CopyToResourceFolder();

private:
    bool m_canIncludeAsResource;
    CString m_pathName;
};
