// CSButton.cpp : implementation file
//

#include "StdAfx.h"

#include "CSButton.h"
#include "MsgDial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCSButton

CCSButton::CCSButton()
{
}

CCSButton::~CCSButton()
{
}


BEGIN_MESSAGE_MAP(CCSButton, CButton)
    //{{AFX_MSG_MAP(CCSButton)
    ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCSButton message handlers

void CCSButton::OnClicked()
{
((CMsgDialog*)m_pParent)->PressButton(m_iIndex);
}


CCSButton::CCSButton( CCSButton & x)
{
    Copy( x );
}

void CCSButton::operator= (CCSButton & x) {
    Copy( x );
}

void CCSButton::Copy( CCSButton & x)
{
    m_iIndex  = x.GetIndex ();
    m_pParent = x.GetParent();
    m_CSLabel = x.GetLabel ();
}


//
void CCSButton::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}
int CCSButton::GetIndex()
{
    return m_iIndex;
}

//
void CCSButton::SetLabel(CString CSLabel)
{
    m_CSLabel = CSLabel;
}
CString CCSButton::GetLabel()
{
    return m_CSLabel;
}

//
void CCSButton::SetParent(CWnd* pParent)
{
    m_pParent = pParent;
}
CWnd* CCSButton::GetParent()
{
    return m_pParent;
}



/*
void CCSButton::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if( nChar == VK_TAB ) {
        //When a button catch this message ==> there is an active button :)

        int iActiveButtonIndex = ((CMsgDialog*)m_pParent)->GetActiveButtonIndex();

        SHORT bShiftPressed = GetKeyState( VK_SHIFT ) & 0x8000;
        if( bShiftPressed ){

            ((CMsgDialog*)m_pParent)->SetActiveButtonIndex( iActiveButtonIndex - 1 );

        } else {

            ((CMsgDialog*)m_pParent)->SetActiveButtonIndex( iActiveButtonIndex + 1 );

        }

    }


    CButton::OnChar(nChar, nRepCnt, nFlags);
}
*/
