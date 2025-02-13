#pragma once

#include <zFormO/FormFile.h>
#include <Zentryo/Runaple.h>

// CDEBaseEdit
const int  CARET_HEIGHT =1;
#define VK_BACK_SPACE _T('\b')


class CDEBaseEdit : public CEdit
{
    DECLARE_DYNAMIC(CDEBaseEdit)

protected:
    COLORREF                m_clrText;      //Text Color
    COLORREF                m_clrBkgnd;     //Background color
    CBrush                  m_brBkgnd;      //BackGround Color


    CDEField*               m_pField;       //Associated Field;
    bool                    m_bRemoveText; //For the first time entry
    bool                    m_bFirstDecimal;// First decimal
    static CFont*           m_pFont; //SAVY&&& TODO see if this has to be static
    static CSize            m_szUnit; //Unit size //SAVY&&& TODO see if this has to be static
    static CDEBaseEdit*     m_pLastEdit; // RHF Oct 18, 2005

    bool                    m_bModified;
    int                     m_iCheckFocus;
    bool                    m_bUseSequential; //for roster . to decide when to update from buffers and when to pick from sData of pField

    static bool             m_bTrapNextHelp; // GHM 20120405

    // Construction
public:
    CDEBaseEdit();
    virtual ~CDEBaseEdit();

    static CDEBaseEdit* GetLastEdit();
    static void SetLastEdit( CDEBaseEdit*pEdit );

    void SetField(CDEField* pField) { m_pField = pField; }
    CDEField* GetField(){ return m_pField;}

     void SetColor( CDC* pDC, COLORREF cColorFg, COLORREF cColorBg ); // RHF 20/8/99

     static void InitSize(CSize& size) { m_szUnit = size;}

     static void SetFieldFont(CFont* pFont) { m_pFont = pFont;}
     CFont* GetFieldFont(void) {ASSERT(m_pFont); return m_pFont;}

     virtual void SetModifiedFlag(bool bFlag) { m_bModified = bFlag;}
     virtual  bool GetModifiedFlag()const {return m_bModified;}

     bool GetRemoveTxtFlag()const {return m_bRemoveText;}
     void SetRemoveTxtFlag(bool bFlag) { m_bRemoveText = bFlag;}




    void SetUseSequential(bool bFlag) {m_bUseSequential = bFlag;}
    bool GetUseSequential(){return  m_bUseSequential;}

    void SetCheckFocus( int iCheckFocus ) {m_iCheckFocus = iCheckFocus;}
    int GetCheckFocus() {return m_iCheckFocus;}

    static UINT RemapChar( CRunAplEntry* pRunApl, UINT nChar, int* iCtrlFlags );
    static void   ResetSpecialKeys( bool bShift, bool bCtrl, bool bAlt );

    virtual CRunAplEntry* GetRunAplEntry();
    virtual void SetSel(int iStart, int iEnd) {return CEdit::SetSel(iStart,iEnd) ;}

    virtual bool IsValidChar(UINT nChar) const =0;
    virtual void ProcessCharKey(UINT& nChar) =0;
    virtual void LimitText(int nChars) { CEdit::LimitText(nChars);}
    virtual DWORD GetEditStyle(CDEField* pField) {return 0;}
    virtual void  SetWindowText(const CString& sString)=0;
    virtual void GetWindowText(CString& rString) const { return CEdit::GetWindowText(rString);}
    virtual BOOL Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID)=0;

public:
    // RHF INIC Dec 17, 2003
    static int              m_iRemappedKeyDown;
    static int              m_iRemappedKeyChar;
    static bool             m_bShift;
    static bool             m_bControl;
    static bool             m_bAlt;
    //static int                m_iTickWidth;

protected:

    LONG OnRefreshSelected(WPARAM wParam, LPARAM lParam);

    LRESULT OnSimulatedKeyDown( WPARAM wParam, LPARAM lParam ); //FABN Jan 16, 2003
    LRESULT OnControlsSetWindowText(WPARAM wParam,LPARAM lParam); // GHM 201006016

    DECLARE_MESSAGE_MAP()

public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual BOOL OnHelpInfo(HELPINFO *lpHelpInfo);
};


