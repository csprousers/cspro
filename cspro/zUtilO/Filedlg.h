#pragma once

//***************************************************************************
//  File name: FILEDLG.H
//
//  Description:
//       Header for CIMSAFileDialog
//
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   bmd     Created from IMPS 4.1.
//              04 Mar 98   bmd     Remove Windows version stuff.
//              05 Mar 98   bmd     Add to CIMSAFileDialog to handle opening new files.
//              12 Mar 98   bmd     Add to CIMSAFileDialog to change button text.
//
//***************************************************************************
//
//  class CIMSAFileDialog : public CFileDialog
//
//  Description:
//      Add functionality to CFileDialog for IMSA.
//
//  Construction
//      CIMSAFileDialog         Construction of the dialog
//
//***************************************************************************
//
//  void CIMSAFileDialog::CIMSAFileDialog(BOOL bOpenFileDialog,
//                                        LPCTSTR pszDefExt = NULL,
//                                        LPCTSTR pszFileName = NULL,
//                                        DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//                                        LPCTSTR pszFilter = NULL,
//                                        CWnd* pParentWnd = NULL,
//                                        UNIT uSavePathType = CFD_PROJ_DIR,
//                                        BOOL bNewFile = FALSE);
//
//      Parameters
//          bOpenFileDialog     TRUE to construct a File Open dialog box or
//                              FALSE to construct a File Save As dialog box.
//          pszDefExt           Required file name extensions.  This is different
//                              from CFileDialog. (see remarks below)
//          pszFileName         The initial filename that appears in the filename edit box.
//                              If NULL, no filename initially appears.
//          dwFlags             A combination of one or more flags that allow you to
//                              customize the dialog box.  For a description of these flags,
//                              see the OPENFILENAME structure in the Win32 SDK documentation.
//                              If you modify the m_ofn.Flags structure member, use a bitwise-OR
//                              operator in your changes to keep the default behavior intact.
//          pszFilter           A series of string pairs that specify filters you can apply to the file.
//          pParentWnd          A pointer to the file dialog-box object's parent or owner window.
//          uSavePathType       CFD_NO_DIR      Don't set or save the path.
//                              CFD_PROJ_DIR    Set and save the path as the project directory.
//                              CFD_DATA_DIR    Set and save the path as the data directory.
//          bNewFile            TRUE to create a new file to open.
//                              FALSE to open an already existing file.
//
//      Remarks
//          If pszDefExt has one extension and the user does not include an extension
//          in the Filename edit box, the extension specified by pszDefExt is automatically appended
//          to the filename.  If an extension is supplied in the File name it must be the specified
//          extension.  If pszDefExt contains more than on extension separated by commas, the user
//          must supply one of the these extensions in the filename.  If pszDefExt is "*", then the
//          append extensions from filter list but allow for no extension if filter list entry is
//          all (*.*).
//
//          If pszDefExt is NULL, no file extension is appended or required.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************


#include <zUtilO/zUtilO.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CIMSAFileDialog
//
/////////////////////////////////////////////////////////////////////////////

constexpr UINT CFD_NO_DIR   = 0;
constexpr UINT CFD_PROJ_DIR = 1;
constexpr UINT CFD_DATA_DIR = 2;


class CLASS_DECL_ZUTILO CIMSAFileDialog : public CFileDialog
{
public:
    CIMSAFileDialog(BOOL bOpenFileDialog,
                    LPCTSTR lpszDefExt = NULL,
                    LPCTSTR lpszFileName = NULL,
                    DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    LPCTSTR lpszFilter = NULL,
                    CWnd* pParentWnd = NULL,
                    UINT uSavePathType = CFD_PROJ_DIR,
                    BOOL bNewFile = FALSE);

    // if pMDIFrameWnd is not null and a document is open, its directory will be used as the initial directory
    CIMSAFileDialog& UseInitialDirectoryOfActiveDocument(CMDIFrameWnd* pMDIFrameWnd);

    CIMSAFileDialog& SetMultiSelectBuffer(size_t MaxFiles = 250);

protected:
    BOOL OnFileNameOK() override;

private:
    void CheckNewFile(const CString& csFullFileName, BOOL& bReturn);

public:
    CStringArray m_aFileName;

private:
    BOOL m_bOpenFileDialog;
    BOOL m_bNewFile;
    UINT m_uDirType;
    TCHAR m_pszDefExt[64];
    CStringArray m_aDefExt;
    TCHAR m_pszFileTitle[_MAX_PATH];
    std::unique_ptr<std::wstring> m_initialDirectory;
    std::unique_ptr<TCHAR[]> m_multiSelectBuffer;
};
