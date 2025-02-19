#pragma once

#include <zUtilF/zUtilF.h>


// A CFileDialog subclass for loading image files that adds a checkbox to indicate whether
// or not to include the image as an application resource.

class CLASS_DECL_ZUTILF ImageFileDialog : public CFileDialog
{
public:
    ImageFileDialog(LPCTSTR lpszFileName = nullptr, CWnd* pParentWnd = nullptr);

    INT_PTR DoModal() override;

private:
    bool m_canIncludeAsResource;
};
