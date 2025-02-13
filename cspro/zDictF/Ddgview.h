#pragma once

//***************************************************************************
//  File name: DDGView.h
//
//  Description:
//       Header for Data Dictionary grid view application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zDictF/Dictgrid.h>
#include <zDictF/Levgrid.h>
#include <zDictF/Recgrid.h>
#include <zDictF/Itemgrid.h>
#include <zUtilO/Pgsetup.h>


namespace DictionaryGrid
{
    constexpr int Dictionary = 0;
    constexpr int Level      = 1;
    constexpr int Record     = 2;
    constexpr int Item       = 3;
}

constexpr long HEADER_ROW = -1;

#define BORDER_WIDTH         4


/////////////////////////////////////////////////////////////////////////////
// CDDGView view

class CLASS_DECL_ZDICTF CDDGView : public CView
{
    DECLARE_DYNCREATE(CDDGView)

protected:
    CDDGView(); // protected constructor used by dynamic creation

public:
    afx_msg void OnEditGenValueSet();
    afx_msg void OnUpdateEditGenValueSet(CCmdUI *pCmdUI);
    afx_msg void OnShiftF10();

    CDDGrid& GetCurrentGrid();
    void ResizeGrid();

    LRESULT OnFind(WPARAM wParam,LPARAM lParam);

protected:
    BOOL PreTranslateMessage(MSG* pMsg) override;
    void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL) override;
    void OnDraw(CDC* pDC) override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
    BOOL OnPreparePrinting(CPrintInfo* pInfo) override;
    void OnPrint(CDC* pDC, CPrintInfo* pInfo = NULL) override;
    void OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView) override;

private:
    void SortItems();
    int FormatNote(CDC* pDC, const CIMSAString& notes, CStringArray& acsText, int iMaxWidth);
    void PrintLevelHead(CDC* pDC, CPrintInfo* pInfo);
    void PrintRelationHead(CDC* pDC, CPrintInfo* pInfo);
    void PrintRecHead(CDC* pDC, CPrintInfo* pInfo, const DictLevel& dict_level, const CDictRecord* pRec, int i);
    void PrintToFile();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
    afx_msg void OnEditDelete();
    afx_msg void OnEditAdd();
    afx_msg void OnUpdateEditAdd(CCmdUI* pCmdUI);
    afx_msg void OnEditInsert();
    afx_msg void OnUpdateEditInsert(CCmdUI* pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnEditCut();
    afx_msg void OnEditPaste();
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditCopyCut(CCmdUI* pCmdUI);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnUpdateEditMakeSubitems(CCmdUI* pCmdUI);
    afx_msg void OnEditMakeSubitems();
    afx_msg void OnUpdateEditFlattenOccurrences(CCmdUI* pCmdUI);
    afx_msg void OnEditFlattenOccurrences();
    afx_msg void OnEditModify();
    afx_msg void OnUpdateEditModify(CCmdUI* pCmdUI);
    afx_msg void OnFilePrint();
    afx_msg void OnFilePrintPreview();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnEditNotes();
    afx_msg void OnUpdateEditNotes(CCmdUI* pCmdUI);
    afx_msg void OnEditOccurrenceLabels();
    afx_msg void OnUpdateEditOccurrenceLabels(CCmdUI* pCmdUI);
    afx_msg void OnEditPasteValueSetLink();
    afx_msg void OnUpdateEditPasteValueSetLink(CCmdUI* pCmdUI);

public:
    CDictGrid   m_gridDict;
    CLevelGrid  m_gridLevel;
    CRecordGrid m_gridRecord;
    CItemGrid   m_gridItem;
    int         m_iGrid;

private:
    int         m_iCount;

    CBitmap     m_bmNoteNo;
    CBitmap     m_bmNoteYes;
    CBitmap     m_bmNoteNoGrayed;
    CBitmap     m_bmNoteYesGrayed;

    LOGFONT     m_lf;

    CFolio      m_folio;

    bool        m_bPrintToFile;
    bool        m_bPrintBrief;
    bool        m_bPrintNameFirst;
    bool        m_bContinuePrinting;

    int         m_iHeight;
    int         m_iLineHeight;
    int         m_iPageWidth;
    int         m_iIndent;
    int         m_iYPos;
    UINT        m_uPage;

    int         m_iLabel;
    int         m_iName;
    int         m_iType;
    int         m_iReq;
    int         m_iMax;

    int         m_iStart;
    int         m_iLen;
    int         m_iDataType;
    int         m_iItemType;
    int         m_iOcc;
    int         m_iDec;
    int         m_iDecChar;
    int         m_iZeroFill;

    int         m_iPrimary;
    int         m_iPrimLink;
    int         m_iSecondary;
    int         m_iSecLink;

    CArray<ITEMSORT,ITEMSORT>   m_aItem;

    bool m_sizesSet;
};
