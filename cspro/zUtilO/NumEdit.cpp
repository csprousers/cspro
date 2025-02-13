// NumEdit.cpp : implementation file
//

#include "StdAfx.h"
#include "NumEdit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNumEdit

CNumEdit::CNumEdit()
{
        m_iMaxNumDecimals               = 0;
        m_iMaxTextLength                = 0;
        m_bNeedDecimalCharacter = false;
}

CNumEdit::~CNumEdit()
{
}


BEGIN_MESSAGE_MAP(CNumEdit, CEdit)
        //{{AFX_MSG_MAP(CNumEdit)
        ON_WM_CHAR()
        ON_WM_KEYDOWN()
        ON_WM_KEYUP()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumEdit message handlers

void CNumEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
        // TODO: Add your message handler code here and/or call default

        //it accepts only one ./+/-
        if( nChar==_T('.') || nChar==_T('+') || nChar=='-' ){
                CString csText;
                GetWindowText(csText);
                if(csText.Find((CString::XCHAR) nChar)!=-1){
                        return;
                }
        }

        //
        if( nChar==_T('.') && m_iMaxNumDecimals==0){
                return;
        }

        //+/- are only accepted if they are at the first place
        if( nChar==_T('+') || nChar=='-' ){
                CString csText;
                GetWindowText(csText);
                int nStartChar;
                int nEndChar;
                GetSel(nStartChar,nEndChar);
                if(nStartChar!=0){
                        return;
                }
        }

        //'.' is accepted only if the right side has less length than m_iMaxNumDecimals
        if( nChar=='.' ){
                CString csText;
                GetWindowText(csText);
                if( csText.Find('.')!=-1 ){
                        return;
                } else {
                        int nStartChar;
                        int nEndChar;
                        GetSel(nStartChar,nEndChar);
                        if(nStartChar==nEndChar && nStartChar<csText.GetLength()){
                                if(csText.Mid(nStartChar).GetLength()>m_iMaxNumDecimals){
                                        return;
                                }
                        } else if( nStartChar!=nEndChar && nEndChar<csText.GetLength()){
                                if( csText.Mid(nEndChar).GetLength()>m_iMaxNumDecimals){
                                        return;
                                }
                        }
                }
        }


    /*the ENTER and ESC  has been catched by OnKeyDown*/
    if(nChar==13/*ENTER*/ || nChar==27/*ESC*/){
        CEdit::OnChar(nChar, nRepCnt, nFlags);
        return;
    }

        //numeric => a->z A->Z not accepted
        //if( (65<=nChar && nChar<=90) || (97<=nChar && nChar<=122) ){
//                return;
        //}

#define VK_BACK_SPACE _T('\b')
    if( _tcschr( _T("01234567890.-+ "), (_TCHAR) nChar ) == NULL && nChar != VK_BACK_SPACE && nChar != VK_LEFT && nChar != VK_RIGHT )
        return;

    //there are some keys that breaks the waiting for a decimal character.
        bool    bBreakDecimalWait = (nChar=='.') || (nChar==8/*LEFT_ARROW_CLEAR_KEY_ASCII*/) || (nChar==32/*SPACE*/) || (nChar==13/*ENTER*/);
        if( m_bNeedDecimalCharacter && bBreakDecimalWait ){
                m_bNeedDecimalCharacter = false;
        }

        int             iMaxIntPartLength       = m_iMaxTextLength -  (m_iMaxNumDecimals>0?m_iMaxNumDecimals+1:0);
        bool    bIsDecChar                      = false;
        CString csDigits                        = _T("0123456789");
        if(csDigits.Find((CString::XCHAR) nChar)!=-1){

                //check length of int part
                int nStartChar;
                int nEndChar;
                GetSel(nStartChar,nEndChar);
                CString csText;
                GetWindowText(csText);
                int iDotIdx                     = csText.Find('.');
                int iCurIntLength       = iDotIdx!=-1 ? csText.Mid(0,iDotIdx).GetLength() : csText.GetLength();
                if( nStartChar==nEndChar ){
                        bIsDecChar = nStartChar>iDotIdx;
                        if( !bIsDecChar ){
                                if( iMaxIntPartLength - iCurIntLength < 1 ){
                                        return;
                                }
                        }
                }
        }

        //count decimals and decide about continue
        bool bCanContinue = true;
        if( m_iMaxNumDecimals>0){

                CString csText;
                GetWindowText(csText);
                if(csText.Find('.')!=-1){

                        int iCurrentTextLength = csText.GetLength();
                        bool bFound = false;
                        int  iNumDecimals = 0;
                        for(int i=iCurrentTextLength-1; i>=0 && !bFound; i--){
                                if(csText.GetAt(i)=='.'){
                                        bFound = true;
                                } else {
                                        iNumDecimals++;
                                }
                        }

                        if( iNumDecimals==m_iMaxNumDecimals && bIsDecChar){

                                bCanContinue = (nChar == 8/*LEFT_ARROW_CLEAR_KEY_ASCII*/) || (nChar==32/*SPACE*/) || (nChar==13/*ENTER*/);
                        }
                }
        }




        if( !m_bNeedDecimalCharacter && bCanContinue){

                CEdit::OnChar(nChar, nRepCnt, nFlags);

                if( nChar==32){
                        SetWindowText(_T(""));
                }


                CString csText;
                GetWindowText(csText);

                bool bCanHaveDot = m_iMaxNumDecimals>0;
                if( (csText.GetLength()==m_iMaxTextLength - 1 - m_iMaxNumDecimals) && csText.Find('.')==-1 ){
                        m_bNeedDecimalCharacter = bCanHaveDot ? true : false;
                }
        }
}

void CNumEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
        // TODO: Add your message handler code here and/or call default
        bool    bBreakDecimalWait = (nChar==46); //the other keys that causes the end of dot waiting are catched by OnChar
        if( m_bNeedDecimalCharacter && bBreakDecimalWait ){
                m_bNeedDecimalCharacter = false;
        }

        CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CNumEdit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
        // TODO: Add your message handler code here and/or call default

        CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
}

