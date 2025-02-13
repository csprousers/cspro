// UGHint.cpp : implementation file
//


#include "StdAfx.h"

#include "Ugctrl.h"
//#include "UGHint.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/********************************************
*********************************************/
CUGHint::CUGHint()
{
    //alloc memory
}

/********************************************
*********************************************/
CUGHint::~CUGHint()
{
    //perform clean-up
}


/********************************************
*********************************************/
BEGIN_MESSAGE_MAP(CUGHint, CWnd)
    //{{AFX_MSG_MAP(CUGHint)
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/********************************************
*********************************************/
void CUGHint::OnPaint()
{

    CDC* dc =GetDC();

    if(m_hFont != NULL)
        dc->SelectObject(m_hFont);

    RECT rect;
    GetClientRect(&rect);

    dc->SetTextColor(m_textColor);
    dc->SetBkColor(m_backColor);


    dc->SetBkMode(OPAQUE);
    dc->DrawText(m_text,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

    ReleaseDC(dc);
    ValidateRect(NULL);
}

BOOL CUGHint::Create(CWnd* pParentWnd, HBRUSH hbrBackground)
{
    ASSERT_VALID(pParentWnd);
    ASSERT(::IsWindow(pParentWnd->GetSafeHwnd()));

/// m_pParentWnd=pParentWnd;

    // creation of window
    //
    if(hbrBackground==NULL)
    {
        hbrBackground=(HBRUSH) (COLOR_INFOBK+1);
    }

    WNDCLASS wndClass;
    wndClass.style=CS_SAVEBITS|CS_DBLCLKS;
    wndClass.lpfnWndProc=AfxWndProc;
    wndClass.cbClsExtra=0;
    wndClass.cbWndExtra=0;
    wndClass.hInstance=AfxGetInstanceHandle();
    wndClass.hIcon=::LoadCursor(NULL,IDC_ARROW);
    wndClass.hCursor=0;
    wndClass.hbrBackground=hbrBackground;
    wndClass.lpszMenuName=NULL;
    wndClass.lpszClassName=_T("HintWnd");

    if(!AfxRegisterClass(&wndClass))
    {
        return FALSE;
    }

    CRect rect(0,0,0,0);
    if(!CWnd::CreateEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST, wndClass.lpszClassName, _T(""),
        WS_BORDER|WS_POPUP, rect, NULL, 0, NULL))
    {
        return FALSE;
    }

    //init the variables
    m_text=_T("");                      //display text
    m_textColor = RGB(0,0,0);       //text color
    m_backColor = RGB(255,255,224); //background color
    m_windowAlign   = UG_ALIGNLEFT; //UG_ALIGNLEFT,UG_ALIGNRIGHT,UG_ALIGNCENTER
                                    //UG_ALIGNTOP,UG_ALIGNBOTTOM,UG_ALIGNVCENTER
    m_textAlign     = UG_ALIGNLEFT; //UG_ALIGNLEFT,UG_ALIGNRIGHT,UG_ALIGNCENTER
    m_hFont = NULL;                 //font handle

    //get the font height
    CDC * dc =GetDC();
    CSize s = dc->GetTextExtent(_T("X"),1);
    m_fontHeight = s.cy;
    ReleaseDC(dc);

    return TRUE;
}


/********************************************
*********************************************/
int CUGHint::SetFont(CFont * font){

    m_hFont = font;

    //get the font height
    CDC * dc =GetDC();
    if(m_hFont != NULL)
        dc->SelectObject(m_hFont);
    CSize s = dc->GetTextExtent(_T("Xy"),2);
    m_fontHeight = s.cy + 3;
    ReleaseDC(dc);
    return UG_SUCCESS;
}
/********************************************
align = UG_ALIGNLEFT or UG_ALIGNRIGHT or UG_ALIGNCENTER
        + UG_ALIGNTOP or UG_ALIGNBOTTOM or UG_ALIGNVCENTER
*********************************************/
int CUGHint::SetWindowAlign(int align){
    m_windowAlign = align;
    return UG_SUCCESS;
}
/********************************************
align = UG_ALIGNLEFT or UG_ALIGNRIGHT or UG_ALIGNCENTER
*********************************************/
int CUGHint::SetTextAlign(int align){
    m_textAlign = align;
    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::SetTextColor(COLORREF color){
    m_textColor = color;
    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::SetBackColor(COLORREF color){
    m_backColor = color;
    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::SetText(LPCTSTR string,int update){
    m_text = string;
    if(update)
        Invalidate();
    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::MoveHintWindow(int x,int y,int width){

    RECT rect;

    //get the width of the string and reset the
    //specified width if needed
    CDC * dc =GetDC();
    // TD first select font...
    if(m_hFont != NULL)
        dc->SelectObject(m_hFont);
    CSize s = dc->GetTextExtent(m_text,m_text.GetLength());
    if((s.cx+4) > width)
        width = s.cx+4;
    ReleaseDC(dc);

    //set up the horizontal pos
    if(m_windowAlign&UG_ALIGNCENTER){       //center
        rect.left = x-(width/2);
        rect.right = x+width;
    }
    else if(m_windowAlign&UG_ALIGNRIGHT){   //right
        rect.left = x-width;
        rect.right = x;
    }
    else{                                   //left
        rect.left = x;
        rect.right = x+width;
    }

    //set up the vertical pos
    if(m_windowAlign&UG_ALIGNVCENTER){      //center
        rect.top    = y-(m_fontHeight/2);
        rect.bottom = rect.top+m_fontHeight;
    }
    else if(m_windowAlign&UG_ALIGNBOTTOM){  //bottom
        rect.top    = y-m_fontHeight;
        rect.bottom = y;
    }
    else{                                   //top
        rect.top = y;
        rect.bottom = y+m_fontHeight;
    }

    //make sure the position is within the parent
    RECT parentRect;
    int dif;
    m_ctrl->GetClientRect(&parentRect);


    if(rect.left < 0){
        dif = 0 - rect.left;
        rect.left+=dif;
        rect.right +=dif;
    }
    if(rect.top <0){
        dif = 0 - rect.top;
        rect.top +=dif;
        rect.bottom +=dif;
    }
    if(rect.right > parentRect.right){
        dif = rect.right - parentRect.right;
        rect.right -=dif;
        rect.left -=dif;
    }
    if(rect.bottom > parentRect.bottom){
        dif = rect.bottom - parentRect.bottom;
        rect.top -= dif;
        rect.bottom -= dif;
    }


    m_ctrl->ClientToScreen(&rect);

    Hide();
    MoveWindow(&rect,TRUE);
    Show();

    SendMessage(WM_PAINT,0,0);

    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::Hide(){
    ShowWindow(SW_HIDE);
    return UG_SUCCESS;
}
/********************************************
*********************************************/
int CUGHint::Show(){
    if(IsWindowVisible() == FALSE)
        ShowWindow(SW_SHOWNA);
    return UG_SUCCESS;
}

