// UGEdit.cpp : implementation file
//

#include "StdAfx.h"
#include "Ugmedit.h"
#include "Ugctrl.h"
#include <ctype.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
****************************************************/
CUGMaskedEdit::CUGMaskedEdit()
{

        m_ctrl = NULL;

        m_MaskKeyInProgress = FALSE;
        m_mask = _T("");
        m_useMask = FALSE;
        m_blankChar = _T('_');
        m_storeLiterals = 1;

        static csprochar Chars[] ={_T("09#L?Aa&C")};
        m_maskChars= Chars;

}

/***************************************************
****************************************************/
CUGMaskedEdit::~CUGMaskedEdit()
{
}


/***************************************************
****************************************************/
BEGIN_MESSAGE_MAP(CUGMaskedEdit, CEdit)
        //{{AFX_MSG_MAP(CUGMaskedEdit)
        ON_WM_KILLFOCUS()
        ON_WM_MOUSEACTIVATE()
        ON_WM_SETFOCUS()
        ON_WM_KEYDOWN()
        ON_WM_CHAR()
        ON_WM_GETDLGCODE()
        ON_WM_CREATE()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/***************************************************
****************************************************/
void CUGMaskedEdit::OnKillFocus(CWnd* pNewWnd)
{
        CEdit::OnKillFocus(pNewWnd);

        if(pNewWnd->GetSafeHwnd() != NULL){
                if(pNewWnd != m_ctrl && pNewWnd->GetParent() != m_ctrl)
                        m_cancel = TRUE;
        }
        else
                m_cancel = TRUE;

        CString string;
        GetWindowText(string);

        if(m_ctrl->EditCtrlFinished(string,m_cancel,m_continueFlag,
          m_continueCol,m_continueRow) == FALSE){
                m_ctrl->GotoCell(m_ctrl->m_editCol,m_ctrl->m_editRow);
                SetCapture();
                ReleaseCapture();
                SetFocus();
        }
}

/***************************************************
****************************************************/
int CUGMaskedEdit::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    return MA_ACTIVATE;
}

/***************************************************
****************************************************/
void CUGMaskedEdit::OnSetFocus(CWnd* pOldWnd)
{

        SetMask(m_ctrl->m_editCell.GetMask());

        m_cancel = FALSE;
        m_continueFlag = FALSE;

        SetSel(0,-1);

        CEdit::OnSetFocus(pOldWnd);
}

/***************************************************
****************************************************/
void CUGMaskedEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

        switch(nChar){
                case VK_TAB:{

                        if(GetKeyState(VK_SHIFT)<0){
                                m_continueCol = m_ctrl->m_editCol ;
                                m_continueRow = m_ctrl->m_editRow -1 ;

                                if(m_continueRow == -1) {
                                    m_continueFlag = FALSE;
                                    CWnd* pWnd = m_ctrl->GetParent()->GetDescendantWindow(ID_HELP);
                                    if(pWnd) {
                                        pWnd->SetFocus();
                                        return;
                                    }

                                }
                        }
                        else{
                                m_continueCol = m_ctrl->m_editCol ;
                                m_continueRow = m_ctrl->m_editRow +1;

                                if(m_continueRow == m_ctrl->GetNumberRows()) {
                                    m_continueFlag = FALSE;
                                    CWnd* pWnd = m_ctrl->GetParent()->GetDescendantWindow(IDOK);
                                    if(pWnd) {
                                        pWnd->SetFocus();
                                        return;
                                    }

                                }
                        }

                        m_continueFlag = TRUE;
                        m_ctrl->SetFocus();
                        return;
                }
                case VK_RETURN:{
                        m_continueCol = m_ctrl->m_editCol;
                        m_continueRow = m_ctrl->m_editRow +1;
                        m_continueFlag = TRUE;

                        if(m_continueRow == m_ctrl->GetNumberRows()) {
                            m_continueFlag = FALSE;
                            CWnd* pWnd = m_ctrl->GetParent()->GetDescendantWindow(IDOK);
                            if(pWnd) {
                                pWnd->SetFocus();
                                return;
                            }

                        }
                        m_ctrl->SetFocus();
                        return;
                }
                case VK_ESCAPE:{
                        m_cancel = TRUE;
                        m_continueFlag = FALSE;
                        m_ctrl->SetFocus();
                        return;
                }
                case VK_UP:{
                        m_continueCol = m_ctrl->m_editCol;
                        m_continueRow = m_ctrl->m_editRow-1;
                        m_continueFlag = TRUE;
                        m_ctrl->SetFocus();
                        return;
                }
                case VK_DOWN:{
                        m_continueCol = m_ctrl->m_editCol;
                        m_continueRow = m_ctrl->m_editRow+1;
                        m_continueFlag = TRUE;
                        m_ctrl->SetFocus();
                        return;
                }
        }

        CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


/***************************************************
****************************************************/
void CUGMaskedEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
        // TODO: Add your message handler code here and/or call default
        int col = m_ctrl->GetCurrentCol();
        long row = m_ctrl->GetCurrentRow();

        if(!m_MaskKeyInProgress){
                if(MaskKeyStrokes(&nChar)==FALSE)
                        return;
                if(m_ctrl->OnEditVerify(col,row,this,&nChar)==FALSE)
                        return;
        }

        if(nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE)
                return;

        CEdit::OnChar(nChar, nRepCnt, nFlags);
}

/***************************************************
****************************************************/
UINT CUGMaskedEdit::OnGetDlgCode()
{
        if(GetKeyState(VK_TAB)<0)
                return DLGC_WANTTAB;
        else if (GetKeyState(VK_RETURN)<0)
                return  DLGC_WANTALLKEYS;
        else if (GetKeyState(VK_ESCAPE)<0)
                return  DLGC_WANTALLKEYS;
        return CEdit::OnGetDlgCode();
}
/***************************************************
****************************************************/
int CUGMaskedEdit::MaskKeyStrokes(UINT *vKey){

        int key = *vKey;

        if(!m_useMask)
                return TRUE;

        //
        if(_istprint(key)==0)
                return TRUE;

        //clear all selections, if any
        Clear();

        //check the key against the mask
        int startPos,endPos;
        GetSel(startPos,endPos);

        //make sure the string is not longer than the mask
        if(endPos >= m_mask.GetLength())
                return FALSE;

        //check to see if a literal is in this position
        int l = m_literals.GetAt(endPos);
        if(l != 8){
                m_MaskKeyInProgress = TRUE;
                #ifdef WIN32
                        AfxCallWndProc(this,m_hWnd,WM_CHAR,l,1);
                #else
                        SendMessage(WM_CHAR,l,1);
                #endif
                m_MaskKeyInProgress = FALSE;
                GetSel(startPos,endPos);
        }

        //check the key against the mask
        int c = m_mask.GetAt(endPos);
        switch(c){
                case _T('0'):{
                        if(_istdigit(key))
                                return TRUE;
                        return FALSE;
                }
                case _T('9'):{
                        if(_istdigit(key))
                                return TRUE;
                        if(key == VK_SPACE)
                                return TRUE;
                        return FALSE;
                }
                case _T('#'):{
                        if(_istdigit(key))
                                return TRUE;
                        if(key == VK_SPACE || key == VK_ADD || key == VK_SUBTRACT)
                                return TRUE;
                        return FALSE;
                }
                case _T('L'):{
                        if(_istalpha(key))
                                return TRUE;
                        return FALSE;
                }
                case _T('?'):{
                        if(_istalpha(key))
                                return TRUE;
                        if(key == VK_SPACE)
                                return TRUE;
                        return FALSE;
                }
                case _T('A'):{
                        if(_istalnum(key))
                                return TRUE;
                        return FALSE;
                }
                case _T('a'):{
                        if(_istalnum(key))
                                return TRUE;
                        if(key == VK_SPACE)
                                return TRUE;
                        return FALSE;
                }
                case _T('&'):{
                        if(_istprint(key))
                                return TRUE;
                        return FALSE;
                }
                case _T('C'):{
                        if(_istprint(key))
                                return TRUE;
                        return FALSE;
                }
        }

        return TRUE;
}
/***************************************************
****************************************************/
void CUGMaskedEdit::ShowInvalidEntryMessage(){
        MessageBeep((UINT)-1);
}
/***************************************************
****************************************************/
int CUGMaskedEdit::SetMask(LPCTSTR string){

        if(string == NULL){
                m_useMask = FALSE;
                return UG_SUCCESS;
        }

        //set the mask
        m_origMask = string;
        if(m_origMask.GetLength() > 0)
                m_useMask = TRUE;
        else{
                m_useMask = FALSE;
                return UG_SUCCESS;
        }

        int loop,loop2,pos;
        int len = m_origMask.GetLength();
        csprochar *buf = new csprochar[len+1];
        int maskChar;

        #ifdef WIN32
                SetLimitText(len);
        #else
                SendMessage(EM_LIMITTEXT,len,0);
        #endif

        //get the parsed mask
        pos =0;
        for(loop =0;loop < len;loop++){
                //check to see if the next mask csprochar is a literal
                if(m_origMask.GetAt(loop)=='\\'){
                        loop++;
                        continue;
                }
                //check for each of the possible mask chars
                maskChar = FALSE;
                for(loop2 = 0 ;loop2 < 9;loop2++){
                        if(m_origMask.GetAt(loop) == m_maskChars[loop2]){
                                maskChar = TRUE;
                                break;
                        }
                }
                //copy the retults
                if(maskChar)
                        buf[pos] = m_origMask[loop];
                else
                        buf[pos] = 8;
                pos++;
        }
        //copy over the final mask string
        buf[pos]=0;
        m_mask = buf;


        //get the parsed literals
        pos =0;
        for(loop =0;loop < len;loop++){
                maskChar = FALSE;
                //check to see if the next mask csprochar is a literal
                if(m_origMask.GetAt(loop)=='\\'){
                        loop++;
                }
                else{
                        //check for each of the possible mask chars
                        for(loop2 = 0 ;loop2 < 9;loop2++){
                                if(m_origMask.GetAt(loop) == m_maskChars[loop2]){
                                        maskChar = TRUE;
                                        break;
                                }
                        }
                }
                //copy the retults
                if(maskChar)
                        buf[pos] = 8;
                else
                        buf[pos] = m_origMask[loop];
                pos++;
        }
        //copy over the final literal string
        buf[pos]=0;
        m_literals = buf;
        delete[] buf;

        OnUpdate();

        return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGMaskedEdit::MaskString(CString *string){

        //check to see if there is a mask
        if(m_useMask == FALSE)
                return 1;

        //make the mask
        int loop;
        int pos=0;      //original string pos
        int pos2=0; //new string pos
        int len = m_mask.GetLength();
        csprochar *buf = new csprochar[len+1];

        for(loop =0;loop <len;loop++){
                //check for literals
                if(m_mask.GetAt(loop) ==8){
                        buf[pos2] = m_literals.GetAt(loop);
                        pos2++;
                }
                //check to see if the string matches the mask
                else{
                        int c = m_mask.GetAt(loop);
                        if(pos == string->GetLength())
                                break;
                        int d = string->GetAt(pos);
                        int valid = FALSE;

                        switch(c){
                                case _T('0'):{
                                        if(_istdigit(d))
                                                valid = TRUE;
                                        break;
                                }
                                case _T('9'):{
                                        if(_istdigit(d))
                                                valid = TRUE;
                                        if(d == VK_SPACE)
                                                valid = TRUE;
                                        break;
                                }
                                case _T('#'):{
                                        if(_istdigit(d))
                                                valid = TRUE;
                                        if(d == VK_SPACE || d == VK_ADD || d == VK_SUBTRACT)
                                                valid = TRUE;
                                        break;
                                }
                                case _T('L'):{
                                        if(_istalpha(d))
                                                valid = TRUE;
                                        break;
                                }
                                case _T('?'):{
                                        if(_istalpha(d))
                                                valid = TRUE;
                                        if(d == VK_SPACE)
                                                valid = TRUE;
                                        break;
                                }
                                case _T('A'):{
                                        if(_istalnum(d))
                                                valid = TRUE;
                                        break;
                                }
                                case _T('a'):{
                                        if(_istalnum(d))
                                                valid = TRUE;
                                        if(d == VK_SPACE)
                                                valid = TRUE;
                                        break;
                                }
                                case _T('&'):{
                                        if(_istprint(d))
                                                valid = TRUE;
                                        break;
                                }
                                case _T('C'):{
                                        if(_istprint(d))
                                                valid = TRUE;
                                        break;
                                }
                        }
                        if(valid){
                                buf[pos2] = d;
                                pos2++;
                        }
                        else
                                loop--;

                        pos++;
                }
                if(pos == string->GetLength())
                        break;
        }


        buf[pos2]=0;
        *string = buf;
        delete[] buf;

        return UG_SUCCESS;
}

/***************************************************
****************************************************/
void CUGMaskedEdit::OnUpdate()
{
        if(m_useMask && m_MaskKeyInProgress == FALSE){
                int startPos,endPos;
                GetSel(startPos,endPos);
                CString s;

                GetWindowText(s);

                if(s.GetLength() >0){
                        m_MaskKeyInProgress = TRUE;
                        MaskString(&s);
                        SetWindowText(s);
                        SetSel(startPos,endPos);
                        m_MaskKeyInProgress = FALSE;
                }
        }
}

/***************************************************
****************************************************/
BOOL CUGMaskedEdit::PreCreateWindow(CREATESTRUCT& cs)
{
        cs.style |= ES_AUTOHSCROLL;
        return CEdit::PreCreateWindow(cs);
}

/***************************************************
****************************************************/
int CUGMaskedEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
        if (CEdit::OnCreate(lpCreateStruct) == -1)
                return -1;

        m_ctrl = (CUGCtrl*)GetParent();
        return 0;
}
