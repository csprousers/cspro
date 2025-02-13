#pragma once

//***************************************************************************
//  File name: FormView.h
//
//  Description:
//       Forms view implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
// the BEGTEXTCOL #define is used in the build of a default form;
// specifically, for the individual fields; i'm trying to line up all data
// such that text is on the right, data on the left, in the following format:
//
//               ddddd  ttttttt
//                  dd  tttt
//             ddddddd  ttttttttttt
//***************************************************************************

#include <zformf/zFormF.h>
#include <zformf/DropRule.h>
#include <zformf/FormGrid.h>
#include <zformf/Tracker.h>
#include <zGrid2O/zGrid2O.h>

class DictTreeNode;

#define MIN_NUM_ROSTER_COLS 11  // 20120504 increased from 6 (5 cols + 1 for stub) because monitors
                                // are now bigger than when this code was originally designed
#define MIN_NUM_ROSTER_ROWS 11  // 10 fields + 1 for stub


class CLASS_DECL_ZFORMF CFormScrollView : public CScrollView
{
    DECLARE_DYNCREATE(CFormScrollView)

protected:
    CFormScrollView();           // protected constructor used by dynamic creation

public:
    virtual ~CFormScrollView();

    bool CanAddForm(CDEForm* pForm);
    void MyRenumberForms();
    void MakeFormCurrent(int iFormIndex);
    CFormGrid* GetGridWhereSelected();
    DictTreeNode* GetDictTreeNode(const CString& name);

    bool        IsOkToDrawTOs() const { return m_okToDrawTrackerOutlines; }
    void        IsOkToDrawTOs(bool b) { m_okToDrawTrackerOutlines = b; }

    CFormDoc*   GetDocument() const { return (CFormDoc*) CView::GetDocument(); }

    int         GetFormIndex() const { return m_iForm; }

    const CDEForm*  GetCurForm () const;
    CDEForm*        GetCurForm ();
    CDELevel*       GetCurLevel();
    CDEGroup*       GetCurGroup();
    CDEItemBase*    GetCurItem () { return m_pRightClickItem; }
    CDEFormFile*    GetFormFile();

    void SetCurItem(CDEItemBase* pItem) { m_pRightClickItem = pItem; }
    void SetCurItem();

    void SetCurTreeItem();

    int GetDropSpacing() const { return m_iDropSpacing; }
    void CalcFieldSpacing();

    const DragOptions& GetDragOptions();

    void RemoveAllTrackersAndRefresh();
    void RefreshTrackers();

    void RemoveSelectionsFromOtherRosters(CFormGrid* pSelectedRoster);

    bool IsItemBeingTracked (CDEItemBase* pItem, CPoint point);


//  funcs related to the tracker items; rename to reflect Items, rather than Boxes (once i get that part wkg)
    template<typename T>
    CFormTracker& AddTrackerT(const CRect& rect, int index);
    
    void    AddTracker(const CRect& rect, int i, bool bIsFldTxt=false, bool bIsBoxSel=false);
    void    AddTracker(int i, CRect rect);
    void    AddTracker(const CFormTracker& track) { m_aTrackerItems.Add(track); }

    void    SetTrackerAt(int i, CFormTracker t) { m_aTrackerItems.SetAt (i,t); }
    
    const CFormTracker& GetTracker(int i) const { return m_aTrackerItems[i]; }
    CFormTracker& GetTracker(int i)             { return m_aTrackerItems[i]; }

    int     GetNumTrackers() const { return m_aTrackerItems.GetSize(); }

    void    RemoveAllTrackers();

    bool    TrackerHitTest(CPoint point);

    void    DeleteTrackerRegionData();

    CRect   UnionAllTrackers();
    void    UpdateTrackerRectsAndRefresh();
    void    UpdateTrackerSh();


    bool    TrackerSetCursorTest (UINT nHitTest);

    void    UpdateTrackerBoxIndices     (int startIndex, int index);
    void    UpdateTrackerFieldIndices   (int startIndex, int index);

    bool        UserRightClickedOverTrackers (CDEItemBase* pRightClickItem, CPoint point);

    void        SetupFontRelatedOffsets();

//  stuff relating to drag options

    bool OkToDropSubitems();

//  other stuff

    void ScrollAsNecessary (CRect* rect);

    void KeepRectInClient (CRect& rectItem);

    void SetFormIndex   (int i) { m_iForm = i; }


    bool okToDrawBox();
    bool okToSelectItems();
    void okToSelectItems(bool b);
    BoxType GetCurBoxDrawType() const;


    bool ResetForm      (CDC* pDC);
    CPoint DetermineScrollSize (CRect rectForm);
    CPoint GetScrollOffset();

    void DrawTrackerOutlines(CDC* pDC, int x=0, int y=0);

    void SetPointers    (int formNum);

    void TreeSelectionChanged (std::vector<CFormID*> pIDs);

    CDEGroup*   OkToCreateNewForm();
    bool        OkToDeleteForm();
    void        CreateNewFF();

//  deleting the active item/tracker region

    void DeleteGroupAndForms        (CDEGroup* pGroup, bool bRemoveForm=true);
    void DeleteActiveGroup          (CDEGroup* pGroup);
    void DeleteBlock                (CDEBlock& form_block, bool delete_block_fields);
    void DeleteActiveItem           ();
    bool DeleteSingleItem           (CDEFormFile* pFF, CDEForm* pForm, CDEGroup* pGroup, CDEItemBase* pItem);
    void DeletionWrapUp             (bool bRebldTree,  CDEForm* pForm, CDEGroup* pGroup);

//  helper funcs to OnLButtonDown() :

    bool SelectSingleItem   (CDC* pDC, CPoint point, UINT nFlags=0);
    bool SelectMultipleElements (CRect crBox);
    bool SelectTrackerElements (CPoint point, CFormTracker localTracker);

    bool  MoveOrResizeTrackers(CPoint point);
    void  MoveItem      (CFormTracker track, CPoint cOffset, FormUndoStack& form_undo_stack); // actually, updates the dims
    void  MoveBox       (const CFormTracker& track, CPoint cOffset, FormUndoStack& form_undo_stack);
    void  MoveBlock     (UINT nChar, int i=1);  // move the tracker elems w/the arrow keys
    CRect MoveBlock     (CPoint cp);
    bool  BlockCanBeMoved(UINT nChar, int& moveAmount) const;

    void MarkDictTree(const DictTreeNode* starting_dict_tree_node = nullptr);

//  these funcs are all in support of dropping an item from the dictionary on to the form

    const CDictRecord* GetDictIDRecord(const CDictItem* pDI, int* iItem);

    bool DropADictFile(DictTreeNode* dict_tree_node);
    bool DropALevel(DictTreeNode* dict_tree_node);
    bool DropARecordUnroster(DictTreeNode* dict_tree_node,  CPoint dropPoint);
    void DropARecordRoster(DictTreeNode* dict_tree_node,  CPoint dropPoint);    // CSC 8/21/00

    void        DropMultItem_Unroster(DictTreeNode* dict_tree_node,  CPoint dropPoint);

    CDERoster*  DropMultItem_RosterOnItem(DictTreeNode* dict_tree_node, CPoint dropPoint);
    CDERoster*  DropMultItem_RosterOnItem(const CDictItem* pDI, const CDictRecord* pDR, CPoint dropPoint);

    CDERoster*  DropAnItemRoster(DictTreeNode* dict_tree_node, CPoint dropPoint);


    CDERoster*  DropItemOnRoster   (DictTreeNode* dict_tree_node,  CPoint dropPoint);
    void        DropARecordOnRoster(DictTreeNode* dict_tree_node,  CPoint adjustedDP);
    CDEField*   DropAnItem         (DictTreeNode* dict_tree_node,  CPoint dropPoint, bool bChangeLoopVals);
    CDEField*   DropAnItem         (const CDictItem* pDI, CPoint* dropPoint, bool bDropSubitems, eDropResult eDropOp = DropResultUndef);

    bool ItemsSubitemBeingKeyed(const CDataDict* pDD, const CDictItem* pDI, CDEFormFile* pFF); // helper func to DropARecordUnroster


//  this blk of funcs for supporting grids on the view

    void RepositionGrid (CDERoster* pRoster);  // CSC 8/2/00

    void RecreateGrids (int iFormNum);

    void UnselectRosterRowOrCol();

//  ***************************************************************************
//  this blk supports CSPro's implementation of a grid for a CDERoster obj

    void AddGrid   (CFormGrid* pGrid)     { m_aCSProGrid.Add (pGrid); }
    void AddGrid   (CGridWnd* pGrid)      { m_aCSProGrid.Add ((CFormGrid*) pGrid); }
    void RemoveGrid(CIMSAString sName);
    void RemoveGrid(CDEGroup* pGroup);

    void RemoveAllGrids();

    void CreateGrid(CDERoster* pRoster);  // this creates it and adds it to the array; was DoTheGridThang for old grid

    int  GetNumGrids() const { return  m_aCSProGrid.GetSize(); }

    CFormGrid* GetGrid(int i)             { return  m_aCSProGrid.GetAt(i); }
    const CFormGrid* GetGrid(int i) const { return m_aCSProGrid.GetAt(i); }

    CFormGrid* FindGrid(CDERoster* pRoster);

    void UnselectGridRowOrCol();

    void DeleteGridCol();
    void DeleteGridField();

    bool IsAnyGridFieldSelected() const;
    bool IsAnyGridColSelected() const;
    bool IsAnyGridRowSelected() const;
    bool IsAnyGridTrackerSelected() const;

    bool AreOnlyMultipleFieldsSelected(CDEItemBase* pRightClickItem); // 20120612
    void OnEditMultipleFieldProperties(std::vector<CDEField*>& fields); // 20120612

    CFormGrid* GetGridWhereTrackerSelected();
    void RefreshGridOccLabelStubs();

    void DrawField(CDEField* pField ,CDC* pDC);

    void UpdateDims();
    void ComputeDims (CDEGroup* pGroup );
    bool ComputeRect(CDEField* pField);

    CDEFormBase::TextLayout AdjustSpacingPrep(CFormDoc** pFD, CDEFormFile** pFF);
    void AdjustFieldSpacing(CDEField* pField, CDEFormBase::TextLayout eTextLayout, int iMaxRPoint);
    void AdjustSpacing();
    void AdjustSpacing(const std::vector<CDEField*>& fields);

    int GetMaxRPoint(CDEForm* pForm);
    int GetMaxRPoint(const std::vector<CDEField*>& fields);

    CDEFormBase::eItemType GetPropItemTypeFromTrackers();
    void ChangeFont(const PortableFont& font);
    void ChangeFont(CDEGroup* pGroup, const PortableFont& font);
    void DeselectTracker(CPoint point);
    void ChangeFont(CDERoster* pRoster, const PortableFont& font);
    void UpdateGridDecimalChar(TCHAR decimalChar);

    void OnInitialUpdate() override; // first time after construct

protected:
    void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL) override;

    void OnDraw(CDC* pDC) override;      // overridden to draw this view

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnEditFFProp();
    afx_msg void OnEditFieldProp();
    afx_msg void OnEditMultipleFieldProperties(); // 20120612
    afx_msg void OnEditLevelProp();
    afx_msg void OnEditTextProp();
    afx_msg void OnEditFormProp();
    afx_msg void OnEditGridAutoFit();
    afx_msg void OnEditGridProp();
    afx_msg void OnAddText();
    afx_msg void OnAddForm();
    afx_msg void OnGenerateFrm();
    afx_msg void OnDeleteItem();
    afx_msg void OnDeleteForm();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt); // 20110408
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnLayoutAlign(UINT nID);
    afx_msg void OnViewLogic();
    afx_msg void OnQSFEditor();
    afx_msg void OnDisplayBoxToolbar();
    afx_msg void OnUpdateEditGridProp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditLevelProp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditFFProp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditFieldProp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditTextProp(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDeleteItem(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDeleteForm(CCmdUI* pCmdUI);
    afx_msg void OnGlobalfont();
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg bool CheckRosterColumnAlignmentEnable();
    afx_msg void OnUpdateLayoutAlign(CCmdUI* pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnEditCut();
    afx_msg void OnFieldFont();
    afx_msg void OnFieldColors();
    afx_msg void OnLayoutSpaceEvenly(UINT nID);
    afx_msg void OnUpdateLayoutSpaceEvenly(CCmdUI* pCmdUI);
    afx_msg void OnLayoutHorizontallyCenter();
    afx_msg void OnUpdateLayoutHorizontallyCenter(CCmdUI* pCmdUI);
    afx_msg void OnLayoutVerticallyCenter();
    afx_msg void OnUpdateLayoutVerticallyCenter(CCmdUI* pCmdUI);
    afx_msg void OnShiftF10();
    afx_msg void OnProperties(); // 20120612
    LRESULT OnDropItem (WPARAM wParam,LPARAM lParam);
    LRESULT OnMarkDictItems (WPARAM wParam,LPARAM lParam);

private:
    TCHAR GetDecimalCharacter();

    bool AlignRosterColumns(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment);


public:
    int             m_iHeight;
    int             m_iBoxHght;

    int             m_iDropSpacing; // what's the amt of space between field & box on a drop?

    CDEItemBase*    m_pRightClickItem;   // ptr to the item over which a right click occurred
    CDERoster*      m_pRightClickRoster; // csc 9/11/00
    int             m_iRosterColIndex;   // smg 10-05-00; if editing the col, nds it's index
	
private:
    CFormDropRules  m_cRules;   // this member contains all the rules to eval a valid drop

// shld i keep an array of form and item indices??  basically for each form i shld retain
// the item index so when i switch back to the form, the box will be drawn around last
// active item...yea?  smg

    int             m_iForm;        // index to the active form

    //  application fonts

    COleDropTarget      m_dropTarget;   // nd for registering the view that it will
    COleDropSource      m_dropSource;   // accept drags and can be initiator of drags

    CArray<CFormTracker,CFormTracker>   m_aTrackerItems;    // for CDEItemBase

    CArray<CFormGrid*,CFormGrid*>       m_aCSProGrid;   // the *new*, *improved* grid, courtesy of chris

//  these next few vars i'm using to help me w/the property dialog boxes!
public:
    bool            m_bAddRFT;          // if T, i want the full dialog; if F, i want some stuff shaded
    CPoint          m_cAddRFTPoint;     // tells me where the user clicked when launching the dialog box
    bool            m_bAddForm;         // is the user trying to modify the curr form's props or do they want a new form?

//  and these vars i'm using to help w/a drop from the dictionary
private:
    CRect           m_pgRect;

    bool            m_okToDrawTrackerOutlines;	
};
