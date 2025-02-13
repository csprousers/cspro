#pragma once

#include <zCapiO/zCapiO.h>


/////////////////////////////////////////////////////////////////////////////
// CSelectListCtrl window
/////////////////////////////////////////////////////////////////////////////

#define MAX_ITEMS 10000
#define MAX_COLUMNS 20

#ifdef WIN_DESKTOP

#define CSELLISTCTRLOPTIONS_KEYBOARDLEN   256

class CLASS_DECL_ZCAPIO CSelectListCtrlOptions
{
public:
    CSelectListCtrlOptions();

    bool Check();

    const std::vector<std::vector<CString>*>* m_paData; // the cells of data
    const std::vector<CString>* m_paColumnTitles; // the column titles (can be NULL)
    std::vector<bool>* m_pbaSelections; // the selected entries (can be NULL)
    const std::vector<PortableColor>* m_rowTextColors; // the row text colors (can be null)

    // Min and Max mark allowed
    int             m_iMinMark;     // 0 to n
    int             m_iMaxMark;     // -1 to n. -1 no limit
    bool            m_bEndOnLimits;

    // Title Bar
    bool            m_bUseTitle;
    CString         m_csTitle;

    // Column Title
    bool            m_bUseColTitle;

    // Highlight the first row
    bool            m_bHighLightFirst;

    // Ctrl-Key behaviour
    bool            m_bHasStickyCtrl;

    // The dialog font
    CFont*          m_pFont;

    // Keyboard Buffer
    int             m_iKeyBuffMaxLen;

    bool            m_bUseParentWindowLimitsForSizing;
};


enum CSELLISTCTRL_OPTION { CSELLISTCTRL_ON, CSELLISTCTRL_OFF, CSELLISTCTRL_TOGGLE };

class CLASS_DECL_ZCAPIO CSelectListCtrl : public CListCtrl
{
private:
    bool    m_bIgnoreLimits; // to avoid temporally over-marked
    int     m_iMarked;  // Actual number of elements marked
    bool    m_bHasPendingCtrl; // flag to control the ficticious ctrl-key pressed
    std::vector<bool> m_baSelections;
    CSelectListCtrlOptions* m_pOptions;

    CString  m_csKeyBuff; // Current keyboard buffer

    bool    m_bIgnoreEnd; // Flag indicating ignoring the maximum marked

    void    EnableStickyControlKey( bool bOnOff );

    void    ResetKeyBuff();

// Construction
public:
    CSelectListCtrl();
    virtual ~CSelectListCtrl();

    void SetOptions( CSelectListCtrlOptions* pOptions );
    void    Init();

// Attributes
public:
    COLORREF m_MarkedBkColor;
    COLORREF m_MarkedTxtColor;

    BOOL SetFullRowSel(BOOL bFullRowSel);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSelectListCtrl)
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
public:
    CFont*      m_pFont;
    bool        m_bFoundLight[MAX_ITEMS][MAX_COLUMNS];
    COLORREF    m_TextBkColor;
    COLORREF    m_TextColor;
    bool        m_bCaseSensitiveSearch;

    BOOL DoAutoWidth( CDC* pDC, int iMinWidth, int iMaxWidth, int iExtraHeightAvailable );
    void AdjustColWidth();

    void SetListStatus(const std::vector<bool>* pbaSelections);
    int  GetListStatus(std::vector<bool>* pbaSelections = NULL, CString* pcsMarkedOptions = NULL);
    void CalculateColumnWidths(std::vector<int>* piaColumnWidths);

    void LoadItems();

    void SetTextColor(COLORREF color);
    void SetMenuBar(BOOL OnOFF);

    void SetTextBkColor(COLORREF color);
    void SetMultiSelect(bool bOnOff = false);

    void ClearSrchMarks();
    int  GetColCount();
    int  FindText(CString csTextToFind = _T(""), BOOL bMark = TRUE);
    void SetMarks( CSELLISTCTRL_OPTION iAction=CSELLISTCTRL_TOGGLE, int iElem=-1 );
    void DrawFoundItem(int nItem);

    // Generated message map functions
protected:
    bool            m_bMenuBar;
    LOGFONT         m_NormalLogFont;
    int             m_cxClient;
    bool            m_bIsMoving;
    int             m_iNRows;
    int             m_iNCols;

    virtual void    DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    int             DrawSelection(int nItem, LPDRAWITEMSTRUCT lpDrawItemStruct = NULL);

    //{{AFX_MSG(CSelectListCtrl)
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnFinishedDialog(UINT wParam, LONG lParam);

    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};


#endif // WIN_DESKTOP
