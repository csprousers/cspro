#pragma once
// ==========================================================================
//                  Class Specification : COXGridHeader
// ==========================================================================

// Header file : OXGridHdr.h

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved

// //////////////////////////////////////////////////////////////////////////

// Properties:
//  NO  Abstract class (does not have any objects)
//  YES Derived from CHeaderCtrl

//  YES Is a Cwnd.
//  YES Two stage creation (constructor & Create())
//  YES Has a message map
//  NO  Needs a resource (template)

//  NO  Persistent objects (saveable on disk)
//  NO  Uses exceptions

// //////////////////////////////////////////////////////////////////////////

// Desciption :
//  This class is used to subclass the Header control of a list control (during column resizing)

// Remark:
//  To prevent the resizing cursor to appear when grid in no resizing mode


// Prerequisites (necessary conditions):

/////////////////////////////////////////////////////////////////////////////

#include <zUToolO/OXDllExt.h>


class OX_CLASS_DECL COXGridHeader : public CHeaderCtrl
{
friend class COXGridList;

DECLARE_DYNAMIC(COXGridHeader);

// Data members -------------------------------------------------------------
public:

protected:
    BOOL m_bResizing;

    int m_nSortCol;
    int m_nSortOrder;

private:

// Member functions ---------------------------------------------------------
public:
    COXGridHeader();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Constructs the object

    BOOL SortColumn(int nCol, int nSortOrder);
    // --- In  :    nCol        -   sorted column
    //              nSortOrder  -   sorting order:
    //                  1   -   ascending
    //                  -1  -   descending
    //                  0   -   no sorting
    // --- Out :
    // --- Returns: TRUE if header columns were successfully overdrawn to show
    //              visual hints on sorted column and sort direction
    // --- Effect : Redraw header control in order to show visual hints on sorted
    //              column and sort direction

#ifdef _DEBUG
    virtual void AssertValid() const;
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : AssertValid performs a validity check on this object
    //              by checking its internal state.
    //              In the Debug version of the library, AssertValid may assert and
    //              thus terminate the program.

    virtual void Dump(CDumpContext& dc) const;
    // --- In  : dc : The diagnostic dump context for dumping, usually afxDump.
    // --- Out :
    // --- Returns :
    // --- Effect : Dumps the contents of the object to a CDumpContext object.
    //              It provides diagnostic services for yourself and
    //               other users of your class.
    //              Note  The Dump function does not print a newline character
    //               at the end of its output.
#endif

    virtual ~COXGridHeader();
    // --- In  :
    // --- Out :
    // --- Returns :
    // --- Effect : Destructor of the object

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COXGridHeader)
    //}}AFX_VIRTUAL

protected:

    virtual void  DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

    //{{AFX_MSG(COXGridHeader)
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    //}}AFX_MSG
    afx_msg BOOL OnHdrCtrlNotify(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};

// ==========================================================================
