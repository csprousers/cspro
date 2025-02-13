#pragma once

//***************************************************************************
//  File name: DDTrCtl.h
//
//  Description:
//       Data Dictionary tree control definitions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zDictF/zDictF.h>
#include <zDictF/DDDpTgt.H>

class CDDDoc;
class DictionaryDictTreeNode;
class DictTreeNode;

/////////////////////////////////////////////////////////////////////////////
//
//                          CDDTreeCtrl
//
/////////////////////////////////////////////////////////////////////////////

constexpr DWORD DRAG_DELAY = 40;

namespace DictIcon
{
    constexpr int Root         =  0;
    constexpr int Level        =  1;
    constexpr int Common       =  2;
    constexpr int Record       =  3;
    constexpr int Item         =  4;
    constexpr int ItemUsed     =  5;
    constexpr int Subitem      =  6;
    constexpr int SubitemUsed  =  7;
    constexpr int ValueSet     =  8;
    constexpr int Relation     =  9;
}

namespace DictCursor
{
    constexpr int Row            = 0;
    constexpr int RowPlus        = 1;
    constexpr int RowStar        = 2;
    constexpr int RowStarPlus    = 3;
    constexpr int Column         = 4;
    constexpr int ColumnPlus     = 5;
    constexpr int ColumnStar     = 6;
    constexpr int ColumnStarPlus = 7;
    constexpr int NoDrop         = 8;
    constexpr int Arrow          = 9;
}


class CLASS_DECL_ZDICTF CDDTreeCtrl : public CTreeCtrl
{
public:
    CDDTreeCtrl();

    void SetDDDoc(CDDDoc* pDoc) { m_pDoc = pDoc; }
    CDDDoc* GetDDDoc() const    { return m_pDoc; }

    void DeleteDrag();

    HTREEITEM InsertDictionary(const CString& sLabel, const CString& sDictFileName, CDDDoc* pDoc);
    bool OpenDictionary(const CString& dictionary_filename, bool bMakeVisible = true);

    void BuildTree(DictionaryDictTreeNode& dictionary_dict_tree_node, bool bRedraw = false);
    void BuildTreeRec(int iRel, int iLevel, int iRec, DictionaryDictTreeNode& dictionary_dict_tree_node, HTREEITEM htiLevel);
    void ReBuildTree(DictionaryDictTreeNode& dictionary_dict_tree_node, int iLevel = NONE, int iRec = NONE, int iItem = NONE);
    void SelectNode(const DictionaryDictTreeNode& dictionary_dict_tree_node, bool bExpandRec, int iLevel = NONE, int iRec = NONE, int iItem = NONE);
    void ResetTreeIcons();
    void DefaultExpand(HTREEITEM hItem);
    DictTreeNode* GetTreeNode(HTREEITEM hItem) const { return reinterpret_cast<DictTreeNode*>(GetItemData(hItem)); }
    DictionaryDictTreeNode* GetDictionaryTreeNode(CDocument& document) const;
    DictionaryDictTreeNode* GetDictionaryTreeNode(wstring_view filename) const;
    void RemoveDictIDs(HTREEITEM hItem);
    void SetDocTemplate(CDocTemplate* pTemplate) {m_pDocTemplate = pTemplate; }
    void Clean();
    void ReleaseDictionaryNode(DictionaryDictTreeNode& dictionary_dict_tree_node);
    void InitImageList();
    void ReleaseDoc(DictionaryDictTreeNode& dictionary_dict_tree_node);
    void SetUpdateAllViews(bool bUpdateAllViews = false) { m_bUpdateAllViews = bUpdateAllViews; }
    static int SetCursor4TabDrop(CPoint point ,bool bFromGrid = false);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDTreeDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnEditAdd();
    afx_msg void OnEditDelete();
    afx_msg void OnEditInsert();
    afx_msg void OnEditModify();
    afx_msg void OnEditAddNext();
    afx_msg void OnEditRelation();
    afx_msg void OnShiftF10();
    afx_msg void OnCopyName();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnViewDictionary();
    afx_msg void OnDictionaryMacros(); // 20101108
    afx_msg void OnEditLanguages();
    afx_msg void OnOpenTool(UINT nID);
    afx_msg void OnOpenDataFile(UINT nID);

private:
    CDataDict* GetSelectedDictionary(bool requires_saved_dictionary = true);

    // runs the callback function with the dictionary view; if the view is not currently
    // activated (e.g., because the form view is active), it will be activated prior to the call
    template<typename CF>
    void OnActivatedView(CF callback_function);

    void OnActivatedViewSendCommand(WPARAM wParam);

public:
    CWnd*           m_pDropWnd;
    std::unique_ptr<CImageList> m_pDragImage;

    CImageList      m_imageList;

    bool            m_bDragging;    // flag to tell me if i'm dragging or not!

    HTREEITEM       m_hDragItem;
    HTREEITEM       m_hDropItem;
    int             m_nDropIndex;
    DWORD           m_dwDragStart;

    bool            m_bFromTree;

private:
    CDocTemplate* m_pDocTemplate;
    CDDDoc*  m_pDoc;

    CDictOleDropTarget  m_dropTarget;
    bool                m_bUpdateAllViews;
    static LOGFONT      m_DefLogFont;
    static CFont        m_font;
    std::vector<CString> m_viewDataFilenames;
};
