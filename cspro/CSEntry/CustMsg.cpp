// CustMsg.cpp : implementation file
//
#include "StdAfx.h"
#include "CustMsg.h"
#include "MainFrm.h"
#include "RunView.h"
#include "Rundoc.h"
#include <zUtilO/CustomFont.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustMsg

CCustMsg::CCustMsg(const MessageOverrides& message_overrides/* = MessageOverrides()*/, const CMsgOptions* pMsgOptions)
    :   m_messageOverrides(message_overrides)
{
    m_eMsgType = None_Msg;
    m_cColorFg = RGB(0,0,0);
    m_uRetVal = 0;
    m_bCanForceOutOfRange = false;
    m_pEdit = NULL;
    m_pMsgOptions = pMsgOptions;
}


CCustMsg::~CCustMsg()
{
}


BEGIN_MESSAGE_MAP(CCustMsg, CWnd)
    //{{AFX_MSG_MAP(CCustMsg)
    ON_WM_CHAR()
    ON_WM_KILLFOCUS()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_ACTIVATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_SYSCOMMAND()
    ON_WM_KEYUP()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustMsg message handlers

void CCustMsg::OnKillFocus(CWnd* pNewWnd)
{
    CWnd::OnKillFocus(pNewWnd);
    //DestroyWindow();
    // TODO: Add your message handler code here

}

/*void CCustMsg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default
//  PostMessage(WM_DESTROY);
 //   return;
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}*/

void CCustMsg::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default

    CWnd::OnChar(nChar, nRepCnt, nFlags);
}


/////////////////////////////////////////////////////////////////////////////////
//
//                            CCustMsg::ShowMessage
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::ShowMessage()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

    ASSERT(m_eMsgType != None_Msg);
    if(this->GetSafeHwnd() != NULL){
        DestroyWindow();
    }
    if(m_sMessage.IsEmpty()){
        //   return;
    }

    // GHM 20100621 to allow for the dynamic setting of fonts for the error message dialog
    UserDefinedFonts* pUserFonts = nullptr;

    // GHM 20100708 on macro's request user-defined fonts will only be used for user error messages
    if( m_sMessage.Left(2).CompareNoCase(_T("U ")) == 0)
        AfxGetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr && pUserFonts->IsFontDefined(UserDefinedFonts::FontType::ErrMsg) ) // user has defined a particular font
    {
        LOGFONT lf;
        pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg)->GetLogFont(&lf);

        m_ErrorFont2.CreateFontIndirect(&lf);

        // the header font will be bolded and 4 points larger
        if( lf.lfHeight < 0 )
            lf.lfHeight -= 4;
        else
            lf.lfHeight += 4;
        lf.lfWeight = FW_BOLD;
        m_ErrorFont1.CreateFontIndirect(&lf);
    }

    m_ErrorFont1.CreateFont (18, 0, 0, 0, 700, FALSE, FALSE, 0, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, FF_DONTCARE,
                            _T("Arial"));
    m_ErrorFont2.CreateFont (14, 0, 0, 0, 400, FALSE, FALSE, 0, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, FF_DONTCARE,
                            _T("Arial"));

//    m_ErrorFont1.CreateFont(18,0,0,0,700,0,0,0,0,0,0,0,0,_T("MS Sans Serif"));
//    m_ErrorFont2.CreateFont(14,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Sans Serif"));
    m_iBorder = 4;
    m_iLine = 1;

   // CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CWnd::CreateEx(WS_EX_TOPMOST|WS_EX_TRANSPARENT,NULL,_T(""),WS_VISIBLE|WS_CHILD|WS_DLGFRAME,CRect(10,10,200,200),(CWnd*)pFrame->GetRunView(),1023);
    pFrame->m_bClose = FALSE;
    if(m_eMsgType == OutOfRange) {
        CalcRMsgDims();
    }
    else if(m_eMsgType == ISSA) {
        CalcGMsgDims();
    }
    else if(m_eMsgType == OutOfSeq) {
        CalcSMsgDims();
    }
    else if(m_eMsgType == VerifyMsg) {
        CalcVMsgDims();
    }


    // RHF INIC Dec 30, 2003
    // The custmsg windows is deleted when (for example) an SKIP is executed from On_Key function.
    // Remove WM_CHAR before to show the window. Other option is that On_Key returns 0.
    MSG msg1;
    while ( ::PeekMessage( &msg1, NULL, WM_CHAR, WM_CHAR, PM_REMOVE ) )
            ;
    // RHF END Dec 30, 2003


    BOOL bDoingBackgroundProcessing = TRUE;
    while ( bDoingBackgroundProcessing ) {
        MSG msg;
        while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ){
            if(msg.message == WM_SIZE)
                continue;
            else if(msg.message == WM_CHAR) {
                if(m_eMsgType == OutOfRange){
                    m_uRetVal = 1;
                    break;
                }
                else if(m_eMsgType == OutOfSeq) {
                    m_uRetVal = 0;
                    m_pEdit->SetRemoveTxtFlag(TRUE);
                    break;
                }
                else if(m_eMsgType == VerifyMsg){
                    if(m_pEdit){
                        if(!m_pEdit->IsValidChar(msg.wParam)){
                            ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
                        }
                    }
                    break;
                }
            }
            else if(msg.message == WM_LBUTTONDOWN || msg.message == WM_DESTROY) {
                if(m_eMsgType == OutOfRange){
                    m_uRetVal = 1;
                }
                else {
                    m_uRetVal = 0;
                }
                ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                break;
            }
            else if(msg.message == WM_KEYDOWN ){
                if(m_eMsgType == OutOfRange){
                    if(msg.wParam == VK_F2 && m_bCanForceOutOfRange) {
                        m_uRetVal = 0;
                    }
                    else if(msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE) {
                        m_uRetVal = 1;
                        ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);

                    }
                    else {
                        m_uRetVal = 1;
                    }
                    break;
                }
                else if(m_eMsgType == ISSA){
                    WPARAM keystroke = m_messageOverrides.clear_key_code.value_or(VK_F8);
                    if( msg.wParam == keystroke){
                        ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                        break;
                    }
                    else {
                      ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                      continue;
                    }
                }
                else if(m_eMsgType == OutOfSeq){
                    if(msg.wParam == VK_F2){
                        m_uRetVal = 1;
                        break;
                    }
                    else {
                        m_uRetVal = 0;
                        m_pEdit->SetRemoveTxtFlag(TRUE);
                        break;
                    }

                }
                else if(m_eMsgType == VerifyMsg){
                    if(m_pEdit){
                        if(!m_pEdit->IsValidChar(msg.wParam)){
                            ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
                        }
                    }
                    break;
                }
            }
            else if (msg.message == WM_MOUSEACTIVATE || msg.message == UWM::CSEntry::EndCustomMessage /* || msg.message == WM_SYSKEYDOWN */) {
                if(m_eMsgType == OutOfRange){
                    m_uRetVal = 1;
                }
                else {
                    m_uRetVal = 0;
                }
                ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                break;
            }
            else if(msg.message == WM_SYSCOMMAND || msg.message == WM_COMMAND || msg.message == WM_MENUSELECT ) {
                if(m_eMsgType == OutOfRange){
                    m_uRetVal = 1;
                }
                else {
                    m_uRetVal = 0;
                }
                ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                break;
            }
            else if(GetSafeHwnd() == NULL ) {
                return;
            }
            if (!AfxGetThread()->PumpMessage())
            {
                bDoingBackgroundProcessing = FALSE;
                ::PostQuitMessage(0);
                break;
            }
        }
        // let MFC do its idle processing
        LONG lIdle = 0;
        while ( AfxGetApp()->OnIdle(lIdle++ ) )
            ;
        if(msg.message == WM_CHAR) {
            if(m_eMsgType == OutOfRange){
                m_uRetVal = 1;
                break;
            }
            else if(m_eMsgType == OutOfSeq) {
                m_uRetVal = 0;
                m_pEdit->SetRemoveTxtFlag(TRUE);
                break;
            }
            else if(m_eMsgType == VerifyMsg){
                if(m_pEdit){
                    if(!m_pEdit->IsValidChar(msg.wParam)){
                        ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
                    }
                }
                break;
            }
        }
        if(msg.message == WM_LBUTTONDOWN || msg.message == WM_DESTROY) {
            if(m_eMsgType == OutOfRange){
                m_uRetVal = 1;
            }
            else {
                m_uRetVal = 0;
            }
            ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
            break;
        }
        if(msg.message == WM_KEYDOWN ){
            if(m_eMsgType == OutOfRange){
                if(msg.wParam == VK_F2 && m_bCanForceOutOfRange) {
                    m_uRetVal = 0;
                }
                else if(msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE) {
                     m_uRetVal = 1;
                    ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                }
                else {
                    m_uRetVal = 1;
                }
                break;
            }
            else if(m_eMsgType == OutOfSeq){
                if(msg.wParam == VK_F2){
                    m_uRetVal = 1;
                    break;
                }
                else {
                    m_uRetVal = 0;
                    m_pEdit->SetRemoveTxtFlag(TRUE);
                    break;
                }
            }
            else if(m_eMsgType == ISSA){
                WPARAM keystroke = m_messageOverrides.clear_key_code.value_or(VK_F8);
                if(msg.wParam == keystroke){
                    ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                    break;
                }
                else {
                    ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
                    continue;
                }
            }
            else if(m_eMsgType == VerifyMsg){
                if(m_pEdit){
                    if(!m_pEdit->IsValidChar(msg.wParam)){
                        ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
                    }
                }
                break;
            }
        }
        else if (msg.message == WM_MOUSEACTIVATE || msg.message == UWM::CSEntry::EndCustomMessage /*|| msg.message == WM_SYSKEYDOWN*/) {
            if(m_eMsgType == OutOfRange){
                m_uRetVal = 1;
            }
            else {
                   m_uRetVal = 0;
            }
            ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
            break;
        }
        else if(msg.message == WM_SYSCOMMAND || msg.message == WM_COMMAND || msg.message== WM_MENUSELECT) {
            if(m_eMsgType == OutOfRange){
                m_uRetVal = 1;
            }
            else {
                m_uRetVal = 0;
            }
            ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE);
            break;
        }
        if(GetSafeHwnd() == NULL ) {
            return;
        }
        // Perform some background processing here
        // using another call to OnIdle
    }

    pFrame->m_bClose = TRUE;
    DestroyWindow();
    m_ErrorFont1.DeleteObject();
    m_ErrorFont2.DeleteObject();

    //BOOL bRet = this->Create(_T(""), NULL, WS_CHILD|WS_VISIBLE, , pFrame->GetActiveView(), 1023);
}


/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::CalcRMsgDims
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::CalcRMsgDims()
{
    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    // Get size of error message
    CFont* pOldFont = dc.SelectObject(&m_ErrorFont1);
    m_rectErr = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(MGF::OutOfRangeOperatorControlledTitle).c_str(), &m_rectErr, DT_CALCRECT);

    // Get size of instructions
    dc.SelectObject(&m_ErrorFont2);
    m_rectInst = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(m_bCanForceOutOfRange ? MGF::EnterValidValueConfirm : MGF::EnterValidValue).c_str(), &m_rectInst, DT_CALCRECT);
    dc.SelectObject(pOldFont);

    // Calc size of window
    m_rectWnd = CRect(0, 0, 2 * m_iBorder + std::max(m_rectErr.Width(), m_rectInst.Width()),
                            4 * m_iBorder + m_iLine + m_rectErr.Height() + m_rectInst.Height());

    // Calc rect of each element
    int xErr  = (m_rectWnd.Width() - m_rectErr.Width()) / 2;
    int xInst = (m_rectWnd.Width() - m_rectInst.Width()) / 2;

    m_rectLine.left = xErr;
    m_rectLine.right = xErr + m_rectErr.Width();
    m_rectLine.top = 2 * m_iBorder + m_rectErr.Height();
    m_rectLine.bottom = m_rectLine.top + m_iLine;

    m_rectErr += CPoint(xErr, m_iBorder);
    m_rectInst += CPoint(xInst, m_rectLine.bottom + m_iBorder);

    CPoint point = CalcPos(m_rectWnd);
    MoveWindow(point.x, point.y, m_rectWnd.Width() + 6, m_rectWnd.Height() + 6);
    this->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::CalcGMsgDims
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::CalcGMsgDims()
{
    m_sMsgText = m_sMessage;
    m_sMsgText.TrimLeft();
    m_sMsgText.TrimRight();
    m_sMsgText.Replace(_T("&"), _T("&&"));

    if( m_pMsgOptions != nullptr && m_pMsgOptions->GetMessageNumber().has_value() )
    {
        m_cColorFg =
            ( *m_pMsgOptions->GetMessageType() == MessageType::User )    ? RGB(0, 0, 0) :
            ( *m_pMsgOptions->GetMessageType() == MessageType::Warning ) ? RGB(0, 0, 255) :
                                                                           RGB(255, 0, 0);
        if( *m_pMsgOptions->GetMessageNumber() >= 0 )
            m_sMsgNum.Format(_T("Message %d"), *m_pMsgOptions->GetMessageNumber());
    }

    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    // Get size of error message
    CFont* pOldFont = dc.SelectObject(&m_ErrorFont1);
    m_rectErr = CRect(0,0,MaxErrWidth(),100);
    dc.DrawText(m_sMsgText, &m_rectErr, DT_CALCRECT | DT_WORDBREAK);

    // Get size of instructions
    dc.SelectObject(&m_ErrorFont2);
    m_rectInst = CRect(0,0,100,100);

    const std::wstring& clear_text = m_messageOverrides.clear_text.has_value() ? *m_messageOverrides.clear_text :
                                                                                 MGF::GetMessageText(MGF::PressF8ToClear);
    dc.DrawText(clear_text.c_str(), &m_rectInst, DT_CALCRECT);

    // Get size of message
    m_rectMsg = CRect(0,0,100,100);
    dc.DrawText(m_sMsgNum, &m_rectMsg, DT_CALCRECT);
    dc.SelectObject(pOldFont);

    // Calc size of window
    m_rectWnd = CRect(0, 0, 2 * m_iBorder + std::max(m_rectErr.Width(), m_rectInst.Width() + m_rectMsg.Width() + 2 * m_iBorder),
                            4 * m_iBorder + m_iLine + m_rectErr.Height() + m_rectInst.Height());

    // Calc rect of each element
    int xErr  = (m_rectWnd.Width() - m_rectErr.Width()) / 2;

    m_rectLine.left = m_iBorder;
    m_rectLine.right = m_rectWnd.right - m_iBorder;
    m_rectLine.top = 2 * m_iBorder + m_rectErr.Height();
    m_rectLine.bottom = m_rectLine.top + m_iLine;

    m_rectErr += CPoint(xErr, m_iBorder);
    m_rectInst += CPoint(m_iBorder, m_rectLine.bottom + m_iBorder);
    m_rectMsg += CPoint(m_rectWnd.Width() - m_rectMsg.Width() - m_iBorder, m_rectLine.bottom + m_iBorder);

    CPoint point = CalcPos(m_rectWnd);
    MoveWindow(point.x, point.y, m_rectWnd.Width() + 6, m_rectWnd.Height() + 6);
    this->SetFocus();
}


/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::CalcSMsgDims
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::CalcSMsgDims()
{
    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    // Get size of error message
    CFont* pOldFont = dc.SelectObject(&m_ErrorFont1);
    m_rectErr = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(MGF::OutOfSequenceTitle).c_str(), &m_rectErr, DT_CALCRECT);

    // Get size of instructions
    dc.SelectObject(&m_ErrorFont2);
    m_rectInst = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(m_bCanForceOutOfRange ? MGF::EnterValidValueConfirm : MGF::EnterCorrectValue).c_str(), &m_rectInst, DT_CALCRECT);
    dc.SelectObject(pOldFont);

    // Calc size of window
    m_rectWnd = CRect(0, 0, 2 * m_iBorder + std::max(m_rectErr.Width(), m_rectInst.Width()),
                            4 * m_iBorder + m_iLine + m_rectErr.Height() + m_rectInst.Height());

    // Calc rect of each element
    int xErr  = (m_rectWnd.Width() - m_rectErr.Width()) / 2;
    int xInst = (m_rectWnd.Width() - m_rectInst.Width()) / 2;

    m_rectLine.left = xErr;
    m_rectLine.right = xErr + m_rectErr.Width();
    m_rectLine.top = 2 * m_iBorder + m_rectErr.Height();
    m_rectLine.bottom = m_rectLine.top + m_iLine;

    m_rectErr += CPoint(xErr, m_iBorder);
    m_rectInst += CPoint(xInst, m_rectLine.bottom + m_iBorder);

    CPoint point = CalcPos(m_rectWnd);
    MoveWindow(point.x, point.y, m_rectWnd.Width() + 6, m_rectWnd.Height() + 6);
    this->SetFocus();
}


/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::CalcVMsgDims
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::CalcVMsgDims()
{
    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    // Get size of error message
    CFont* pOldFont = dc.SelectObject(&m_ErrorFont1);
    m_rectErr = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(MGF::VerifyFieldNotMatch).c_str(), &m_rectErr, DT_CALCRECT);

    // Get size of instructions
    dc.SelectObject(&m_ErrorFont2);
    m_rectInst = CRect(0,0,100,100);
    dc.DrawText(MGF::GetMessageText(MGF::VerifyReenter).c_str(), &m_rectInst, DT_CALCRECT);
    dc.SelectObject(pOldFont);

    // Calc size of window
    m_rectWnd = CRect(0, 0, 2 * m_iBorder + std::max(m_rectErr.Width(), m_rectInst.Width()),
                            4 * m_iBorder + m_iLine + m_rectErr.Height() + m_rectInst.Height());

    // Calc rect of each element
    int xErr  = (m_rectWnd.Width() - m_rectErr.Width()) / 2;
    int xInst = (m_rectWnd.Width() - m_rectInst.Width()) / 2;

    m_rectLine.left = xErr;
    m_rectLine.right = xErr + m_rectErr.Width();
    m_rectLine.top = 2 * m_iBorder + m_rectErr.Height();
    m_rectLine.bottom = m_rectLine.top + m_iLine;

    m_rectErr += CPoint(xErr, m_iBorder);
    m_rectInst += CPoint(xInst, m_rectLine.bottom + m_iBorder);

    CPoint point = CalcPos(m_rectWnd);
    MoveWindow(point.x, point.y, m_rectWnd.Width() + 6, m_rectWnd.Height() + 6);
    this->SetFocus();
}


/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::OnPaint
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    CRect rcDraw, rcClient;

    if(m_eMsgType == OutOfRange) {
        DrawRMsg(&dc);
    }
    else if(m_eMsgType == ISSA){
        DrawGMsg(&dc);
    }
    else if(m_eMsgType == OutOfSeq){
        DrawSMsg(&dc);
    }
    else if(m_eMsgType == VerifyMsg){
        DrawVMsg(&dc);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//                            CCustMsg::DrawRMsg
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::DrawRMsg(CDC* pDC)
{
    pDC->SetMapMode(MM_TEXT);

    // Display error message
    CFont* pOldFont = pDC->SelectObject(&m_ErrorFont1);
    pDC->SetTextColor(RGB(0,0,255));
    pDC->DrawText(MGF::GetMessageText(MGF::OutOfRangeOperatorControlledTitle).c_str(), m_rectErr, DT_CENTER);

    // Display line
    CPen penCell(PS_SOLID, 1, RGB(0,0,0));
    pDC->SelectObject(&penCell);
    pDC->DrawEdge(m_rectLine, EDGE_ETCHED, BF_BOTTOM);

    // Display instructions
    pDC->SelectObject(&m_ErrorFont2);
    pDC->SetTextColor(RGB(0,0,0));
    pDC->DrawText(MGF::GetMessageText(m_bCanForceOutOfRange ? MGF::EnterValidValueConfirm : MGF::EnterValidValue).c_str(), m_rectInst, DT_CENTER);
    pDC->SelectObject(pOldFont);
}

/////////////////////////////////////////////////////////////////////////////////
//
//                            CCustMsg::DrawGMsg
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::DrawGMsg(CDC* pDC)
{
    pDC->SetMapMode(MM_TEXT);

    // Display error message
    CFont* pOldFont = pDC->SelectObject(&m_ErrorFont1);
    pDC->SetTextColor(m_cColorFg);
    pDC->DrawText(m_sMsgText, m_rectErr, DT_WORDBREAK);

    // Display line
    CPen penCell(PS_SOLID, 1, RGB(0,0,0));
    pDC->SelectObject(&penCell);
    pDC->DrawEdge(m_rectLine, EDGE_ETCHED, BF_BOTTOM);

    // Display instructions
    pDC->SelectObject(&m_ErrorFont2);
    pDC->SetTextColor(RGB(0,0,0));

    const std::wstring& clear_text = m_messageOverrides.clear_text.has_value() ? *m_messageOverrides.clear_text :
                                                                                 MGF::GetMessageText(MGF::PressF8ToClear);
    pDC->DrawText(clear_text.c_str(), m_rectInst, DT_CENTER);

    if (m_sMsgNum  != _T("")) {
        pDC->DrawText(m_sMsgNum, m_rectMsg, DT_CENTER);
    }
    pDC->SelectObject(pOldFont);
}

/////////////////////////////////////////////////////////////////////////////////
//
//                        CCustMsg::DrawSMsg(CDC *pDC)
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::DrawSMsg(CDC *pDC)
{
    pDC->SetMapMode(MM_TEXT);

    // Display error message
    CFont* pOldFont = pDC->SelectObject(&m_ErrorFont1);
    pDC->SetTextColor(RGB(0,0,255));
    pDC->DrawText(MGF::GetMessageText(MGF::OutOfSequenceTitle).c_str(), m_rectErr, DT_CENTER);

    // Display line
    CPen penCell(PS_SOLID, 1, RGB(0,0,0));
    pDC->SelectObject(&penCell);
    pDC->DrawEdge(m_rectLine, EDGE_ETCHED, BF_BOTTOM);

    // Display instructions
    pDC->SelectObject(&m_ErrorFont2);
    pDC->SetTextColor(RGB(0,0,0));
    pDC->DrawText(MGF::GetMessageText(m_bCanForceOutOfRange ? MGF::EnterValidValueConfirm : MGF::EnterCorrectValue).c_str(), m_rectInst, DT_CENTER);
    pDC->SelectObject(pOldFont);
}

/////////////////////////////////////////////////////////////////////////////////
//
//                          CCustMsg::DrawVMsg
//
/////////////////////////////////////////////////////////////////////////////////

void CCustMsg::DrawVMsg(CDC *pDC)
{
    pDC->SetMapMode(MM_TEXT);

    // Display error message
    CFont* pOldFont = pDC->SelectObject(&m_ErrorFont1);
    pDC->SetTextColor(RGB(0,128,0));
    pDC->DrawText(MGF::GetMessageText(MGF::VerifyFieldNotMatch).c_str(), m_rectErr, DT_CENTER);

    // Display line
    CPen penCell(PS_SOLID, 1, RGB(0,0,0));
    pDC->SelectObject(&penCell);
    pDC->DrawEdge(m_rectLine, EDGE_ETCHED, BF_BOTTOM);

    // Display instructions
    pDC->SelectObject(&m_ErrorFont2);
    pDC->SetTextColor(RGB(0,0,0));
    pDC->DrawText(MGF::GetMessageText(MGF::VerifyReenter).c_str(), m_rectInst, DT_CENTER);
    pDC->SelectObject(pOldFont);
}

/////////////////////////////////////////////////////////////////////////////////
//
//                          CCustMsg::OnEraseBkgnd
//
/////////////////////////////////////////////////////////////////////////////////

BOOL CCustMsg::OnEraseBkgnd(CDC* pDC)
{
    CRect rect;
    GetClientRect(rect);
    pDC->FillSolidRect(rect, RGB(255,255,224));
//  pDC->FillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));
    return TRUE;
}

void CCustMsg::OnDestroy()
{
    m_ErrorFont1.DeleteObject();
    m_ErrorFont2.DeleteObject();
    SendMessage(UWM::CSEntry::EndCustomMessage);
}


void CCustMsg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CWnd::OnActivate(nState, pWndOther, bMinimized);

    // TODO: Add your message handler code here

}

BOOL CCustMsg::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->message == WM_CHAR) {
    }
    return CWnd::PreTranslateMessage(pMsg);
}

/*void CCustMsg::OnLButtonDown(UINT nFlags, CPoint point)
{
    PostMessage(WM_DESTROY);
    return;
//  CWnd::OnLButtonDown(nFlags, point);
}*/

void CCustMsg::OnSize(UINT nType, int cx, int cy)
{
    return;
    // CWnd::OnSize(nType, cx, cy);
}


void CCustMsg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if(nID == SC_SIZE || nID ==  SC_MAXIMIZE || nID == SC_MINIMIZE)
        return;
    CWnd::OnSysCommand(nID, lParam);
}

/////////////////////////////////////////////////////////////////////////////////
//
//                          CCustMsg::MaxErrWidth
//
/////////////////////////////////////////////////////////////////////////////////

int CCustMsg::MaxErrWidth(void)
{
    CMainFrame* pFrame = (CMainFrame*) AfxGetMainWnd();
    CEntryrunView* pView = pFrame->GetRunView();
    CRect rect;
    pView->GetWindowRect(&rect);
    pView->ScreenToClient(&rect);
    return (rect.Width() / 2) - 10;
}

/////////////////////////////////////////////////////////////////////////////////
//
//                          CCustMsg::CalcPos
//
/////////////////////////////////////////////////////////////////////////////////

CPoint CCustMsg::CalcPos(CRect rectWnd)
{
    CPoint point = CPoint(0,0);
    CMainFrame* pFrame = (CMainFrame*) AfxGetMainWnd();

    CEntryrunView* pView = pFrame->GetRunView();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();

    CDEBaseEdit* pCurEdit = pView->SearchEdit((CDEField*)pDoc->GetCurField());
    CRect rect;
    pView->GetWindowRect(&rect);
    pView->ScreenToClient(&rect);

    // Field present
    if(pCurEdit) {
        CRect rectEdit;
        pCurEdit->GetWindowRect(&rectEdit);
        pView->ScreenToClient(&rectEdit);

        int X2 =  rectEdit.BottomRight().x;
        int Y2 =  rectEdit.BottomRight().y;

        int X1 = rect.left + (rect.Width() / 2);
        int Y1 = rect.top + (rect.Height() / 2);

        ASSERT(X1 > rectWnd.Width());
        ASSERT(Y1 > rectWnd.Height());

        int iDX = X1 - X2;
        int iDY = Y1 - Y2;

        // Field in upper left
        if(iDX >= 0 && iDY >=0 ) {
            point.x = X1 + (X1 - rectWnd.Width()) / 2;
            point.y = Y1 + (Y1 - rectWnd.Height()) / 2;
        }
        // Field in upper right
        else if(iDX <= 0 && iDY >=0 ) {
            point.x = (X1 - rectWnd.Width()) / 2;
            point.y = Y1 + (Y1 - rectWnd.Height()) / 2;
        }
        // Field in lower left
        else if(iDX <= 0 && iDY <=0 ) {
            point.x = (X1 - rectWnd.Width()) / 2;
            point.y = (Y1 - rectWnd.Height()) / 2;
        }
        // Field in lower right
        else if(iDX >= 0 && iDY <=0 ) {
            point.x = X1 + (X1 - rectWnd.Width()) / 2;
            point.y = (Y1 - rectWnd.Height()) / 2;
        }
    }
    // No field present
    else {
        point.x = (rect.Width() - rectWnd.Width()) / 2;
        point.y = (rect.Height() - rectWnd.Height()) / 2;
    }
    return point;
}

void CCustMsg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CEntryrunView* pView = (CEntryrunView*) GetParent();
    CEntryrunDoc* pDoc = (CEntryrunDoc*) pView->GetDocument();
    CDEField* pField = (CDEField*) pDoc->GetCurField();
    CDEBaseEdit* pEdit = (CDEBaseEdit*) pView->SearchEdit(pField);
    if (nChar == 16) {
        pEdit->m_bShift = false;
    }
    else if (nChar == 17) {
        pEdit->m_bControl = false;
    }
    else if (nChar == 18) {
        pEdit->m_bAlt = false;
    }
    CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}
