#pragma once
/*----------------------------------------------------------------------
Copyright (C)2001 MJSoft. All Rights Reserved.
          This source may be used freely as long as it is not sold for
                    profit and this copyright information is not altered or removed.
                    Visit the web-site at www.mjsoft.co.uk
                    e-mail comments to info@mjsoft.co.uk
File:     SortHeaderCtrl.h
Purpose:  Provides the header control, with drawing of the arrows, for
          the list control.
----------------------------------------------------------------------*/

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF CSortHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
    CSortHeaderCtrl();

// Overrides
// ClassWizard generated virtual function overrides
//{{AFX_VIRTUAL(CSortHeaderCtrl)
public:
    virtual void Serialize(CArchive& ar);
//}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CSortHeaderCtrl();

    void SetSortArrow( const int iColumn, const BOOL bAscending );

    // Generated message map functions
protected:
    void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

    int m_iSortColumn;
    BOOL m_bSortAscending;

    //{{AFX_MSG(CSortHeaderCtrl)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
