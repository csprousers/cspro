#pragma once
//***************************************************************************
//  File name: GradLbl.h
//
//  Description:
//  Custom draw CStatic that draws a gradient background under the text.
//
//***************************************************************************

/////////////////////////////////////////////////////////////////////////////
// CGradientLabel window

class CGradientLabel : public CStatic
{
// Construction
public:
    CGradientLabel();

// Attributes
public:

// Operations
public:

    virtual void PreSubclassWindow();

// Implementation
public:
    virtual ~CGradientLabel();

    // Generated message map functions
protected:
    afx_msg void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    DECLARE_MESSAGE_MAP()

    void DrawGradient(CDC& dc, const CRect& r, COLORREF clrLeft, COLORREF clrRight);
};

/////////////////////////////////////////////////////////////////////////////
