#pragma once
// ==========================================================================
//                          Class Specification : COXDialogBar
// ==========================================================================

// Header file : OXDlgBar.h

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved

// //////////////////////////////////////////////////////////////////////////

// Properties:
//  NO  Abstract class (does not have any objects)
//  YES Derived from CControlBar

//  YES Is a Cwnd.
//  YES Two stage creation (constructor & Create())
//  YES Has a message map
//  NO  Needs a resource (template)

//  NO  Persistent objects (saveable on disk)
//  YES Uses exceptions

// //////////////////////////////////////////////////////////////////////////

// Desciption :
//  This class encapsulates a controlbar with a dialog created from a memory template
//  on top of it.

/////////////////////////////////////////////////////////////////////////////

#include <zUToolO/OXDllExt.h>
class OX_CLASS_DECL COXDialogBar : public CControlBar
{
// Data Members

public:
    //{{AFX_DATA(COXDockPropertySheet)
    //}}AFX_DATA

protected:
    CSize m_sizeDefault;

private:

// Member Functions
public:
    COXDialogBar();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Contructor of object

    BOOL Create(CWnd* pParentWnd, UINT nStyle, UINT nID);
    // --- In  : pParentWnd : parent window
    //           nDlgStyle : styles for the 'memory' dialog
    //           nID : ID of the dialog
    // --- Out :
    // --- Returns : successful or not
    // --- Effect : Creates Control bar with a dialog from a mem template on top of it

    virtual ~COXDialogBar();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Destructor of object

protected:
    virtual BOOL CreateMemDialog(CWnd* pParentWnd);
    virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COXDialogBar)
    public:
    protected:
    void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
    //}}AFX_VIRTUAL


    // Generated message map functions
    //{{AFX_MSG(COXDialogBar)
    //}}AFX_MSG
protected:
#ifndef _AFX_NO_OCC_SUPPORT
    // data and functions necessary for OLE control containment
    _AFX_OCC_DIALOG_INFO* m_pOccDialogInfo;
    LPCTSTR m_lpszTemplateName;
    virtual BOOL SetOccDialogInfo(_AFX_OCC_DIALOG_INFO* pOccDialogInfo);
    afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()
#endif
};
