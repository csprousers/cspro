#pragma once

// RunView.h : interface of the CEntryrunView class
//
/////////////////////////////////////////////////////////////////////////////

#include <CSEntry/Rundoc.h>
#include <CSEntry/DEGrid.h>
#include <zFormO/FieldColors.h>


class CEntryrunView : public CScrollView
{
    DECLARE_DYNCREATE(CEntryrunView)

protected: // create from serialization only
    CEntryrunView();

public:
    virtual ~CEntryrunView();

private:
    void ShowCapi(const CDEBaseEdit* pEdit) const;// RHF Jan 30, 2000

public:
    void ShowCapi( const CDEField* pField ) const; // RHF Jan 30, 2000

    CCapi* GetCapi() const;  // RHF INIC Jan 29, 2000
    void DeleteLabels() const;// RHF INIC Jan 29, 2000

    void ShowQuestion( const CDEField* pField ) const; // RHF Jan 14, 2000
    void ShowQuestion(const CDEBaseEdit* pEdit) const ; // RHF Jan 14, 2000

    void CEntryrunView::ShowLabels( const CDEField* pField ) const;// RHF Jan 14, 2000
    void CEntryrunView::ShowLabels(const CDEBaseEdit* pEdit) const;// RHF Jan 14, 2000

    void SetCurrentFormFileNum( CDEField* pField ); // RHF Jan 12, 2000
    int  GetCurrentFormFileNum( void ) { return m_iCurrentFormFileNum; } // RHF Jan 12, 2000

    CEntryrunDoc* GetDocument() { return assert_cast<CEntryrunDoc*>(m_pDocument); }

    int AddEdit(CDEBaseEdit* pEdit);
    void InsertEditAt(CDEBaseEdit* pEdit, int iIndex);
    void RemoveEdit(int iIndex);

    void RemoveAllEdits(void);
    void RemoveAllGrids2(bool bDelete = true);
    bool ShowGrid(CDERoster* pRoster , bool bShow = true);
    void ResetGrids(bool bForceReset = false);

    void ClearScreen(CDC* pDC);
    BOOL ResetForm();
    void DrawStaticItems();
    BOOL AddEditItem(CDEField* pField);
    bool SetDictItem(CDEField* pField);
    void GoToFld(CDEField* pField);
    void DoGoToFld(CDEField* pField);

    LONG OnEndgrp(WPARAM wParam, LPARAM lParam);
    LONG OnEndLevel(WPARAM wParam, LPARAM lParam);
    LONG OnNextLevelOcc(WPARAM wParam, LPARAM lParam);


    LONG OnEditChange (UINT wParam, LONG lParam);
    void OnEditEnter(CDEBaseEdit* pEdit);
    void OnEditPrev(CDEBaseEdit* pEdit);
    void UpdateFields();

    void ScrollToField(CDEField* pField);
    void ScrollToCell(CRect* pRect);


    void SetGridEdit(CDEField* pField );
    void UpdateGridEdits(CDEField* pField);

    BOOL  CheckToShow(const CDEItemBase* pTestBase);
    void ProcessFldAttrib(CDEField* pField);
    BOOL OutOfSequence(CDEBaseEdit* pEdit);

    void ProcessModifyMode();
    CDEBaseEdit* SearchEdit( const CDEField* pField ) const; // RHF Jan 14, 2000

    BOOL DoPageUpField(CDEField* pField);
    LONG OnPageUp(WPARAM wParam, LPARAM lParam);
    CDEField* FindFirstEdit(CDEGroup* pGroup, bool bSkipMirrorProtectedPersistent = true);
    CDEForm* FindPageUpForm(CDEField* pField);

    LONG OnPageDown(WPARAM wParam, LPARAM lParam);
    BOOL DoPageDownField(CDEField* pField);
    CDEForm* FindPageDownForm(CDEField* pField);

    LONG OnSlashKey(WPARAM wParam, LPARAM lParam);
    LONG OnInsertAfter(WPARAM wParam, LPARAM lParam);

    void ChkFrmChangeNUpdate(CDEField* pNewField);
    void PutEditValInBuffers(CDEBaseEdit* pEdit);
    BOOL ChkPProcReq(CDEBaseEdit* pEdit);

    CDEGrid* FindGrid(CDEGroup* pGroup) const;

    LONG OnPreviousPersistent(WPARAM wParam, LPARAM lParam);
    LONG OnCheatKey(WPARAM wParam, LPARAM lParam);
    LONG OnPlusKey(WPARAM wParam, LPARAM lParam);

    LONG OnAdvToEnd(WPARAM wParam, LPARAM lParam);

private:
    template<typename GFC>
    void GoToField(GFC get_field_callback);
public:
    void GoToField(CDEField* pField, int iOcc =-1);
    void GoToField(const CaseItemReference& case_item_reference);

    void SetupFieldColors(const CDEFormFile* form_file);
    COLORREF GetFieldBackgroundColor(int intensity) const { return std::get<0>(m_fieldBrushes[intensity]); }
    HBRUSH GetFieldBrush(int intensity) const             { return static_cast<HBRUSH>(*std::get<1>(m_fieldBrushes[intensity])); }

public:
    BOOL GetCheatKey() {return m_bCheatKey;}
    void SetCheatKey(BOOL bCheatKey) { m_bCheatKey = bCheatKey;}
    void ResetVerifyString(void) { m_sVerify.Empty(); }
    void SetVerifyStringFromControls(CString csString,bool bIsNumeric);
    void BuildGrids(void);
    void DrawScreenStats(CDC* pDC);

    void RosterEdit(CDEBaseEdit* pEdit,UINT uKey);
    void FindNextFieldXLMode(CDEField* pCurField,UINT uKey,CDEField*& pNextField,int& iOcc);

    CDEField* GetNextField(CDEField* pField);
    CDEField* GetPrevField(CDEField* pField);
    int       GetDynamicMaxOccs(CDEGroup* pGroup);
    CDEField* GetFirstNonProtectedField(CDEGroup* pGroup );
    CDEField* GetLastNonProtectedField(CDEGroup* pGroup);
    bool IsLastField(CDEField* pCurField);

public:
    void OnDraw(CDC* pDC) override;  // overridden to draw this view
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL) override;

protected:
    void OnInitialUpdate() override; // called first time after construct
    void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor); // RHF 20/8/99
    afx_msg void OnSize(UINT nType, int cx, int cy); // 20100423 for centering forms
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnNextgrp();
    afx_msg void OnUpdateEndgrp(CCmdUI* pCmdUI);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnInsertGroupocc();
    afx_msg void OnUpdateInsertGroupocc(CCmdUI* pCmdUI);
    afx_msg void OnDeleteGrpocc();
    afx_msg void OnUpdateDeleteGrpocc(CCmdUI* pCmdUI);
    afx_msg void OnSortgrpocc();
    afx_msg void OnInsertGroupoccAfter();               // victor Mar 26, 02
    afx_msg void OnUpdateSortgrpocc(CCmdUI* pCmdUI);
    afx_msg void OnPreviousPersistent();
    afx_msg void OnUpdatePreviousPersistent(CCmdUI* pCmdUI);

    afx_msg void OnUpdateNote(CCmdUI* pCmdUI);
    afx_msg void OnFieldNote();
    afx_msg void OnCaseNote();

    LONG OnRefreshData(WPARAM wParam, LPARAM lParam);
    LONG OnMoveToField(WPARAM wParam, LPARAM lParam);

    LONG OnShowCapi(WPARAM wParam, LPARAM lParam); // RHF Nov 22, 2002


private:
    CArray<CDEBaseEdit*, CDEBaseEdit*> m_aEdit;
    CArray<CDEGrid*, CDEGrid*> m_aGrid;

    FieldColors m_fieldColors;
    std::vector<std::tuple<COLORREF, std::unique_ptr<CBrush>>> m_fieldBrushes;

    CIMSAString         m_sVerify; //The verify string
    int                 m_iVerify; //The verify counter

    int                 m_iCurrentFormFileNum; // RHF Jan 12, 2000
    BOOL                m_bCheatKey;
    CDEField*           m_pOldField;
};
