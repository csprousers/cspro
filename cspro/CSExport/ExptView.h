#pragma once
// ExptView.h : interface of the CExportView class
//
/////////////////////////////////////////////////////////////////////////////

class CExportDoc;
class CExportSplitterFrame;


class CExportView : public CFormView
{
protected: // create from serialization only
    CExportView();
    DECLARE_DYNCREATE(CExportView)

public:
    //{{AFX_DATA(CExportView)
    enum { IDD = IDD_EXPORT_FORM };
    //}}AFX_DATA
    CTreeCtrl   m_dicttree;
    CImageList  m_ilState,m_ilNormal;

// Attributes
public:
    CExportDoc* GetDocument();

    CString GetDetails(HTREEITEM hItem);
    void SetParentStates(HTREEITEM hItem);
    bool AddRecordIntree(int iRel, const CDictRecord* pRecord,HTREEITEM htreeLabel);
    bool AddItemInTree(int iRel, const CDictItem* pITem,HTREEITEM htreeLabel);
    void TreeItemClicked(HTREEITEM hItem,bool recursive = false);

    void FromHitems2DictItems(  CArray<HTREEITEM,HTREEITEM>&                                    aSelHitems,
                                CArray<const CDictItem*,const CDictItem*>&                      rItems,
                                CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&  rMapSelOccsByItem);


    const CDictItem* GetDictItem( HTREEITEM hItem );


    void    CheckRelationsOnlyInSepRecs();

    int     GetSelItems( bool bIncludeRelations, CArray<const CDictItem*,const CDictItem*>& raSelItems, CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem, bool* pbHasAnyMultiple = NULL, bool* pbHasAnySingle = NULL, bool* pbIgnoreChildsOfSingleRecords = NULL );
    int     GetChecked( HTREEITEM hitem, bool bRecursive, bool bIncludeRelations, CArray<HTREEITEM,HTREEITEM>& aCheckedItems );

    int     GetSelItemsByRelation(const DictRelation& dict_relation, CArray<const CDictItem*,const CDictItem*>& raSelItems,
                                  CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem );

    bool    IsFlatExport( const CDictRecord* pDictRecord, bool* pbHasAnyMultipleItem );

    bool    IsRelationSelected(const DictRelation& dict_relation);

    bool    IsSelectedAnyMultiple( bool bIncludeRelations, bool bIgnoreChildsOfSingleRecords = false );
    bool    IsSelectedAnySingle( bool bIncludeRelations );
    bool    IsSelectedAnyRelation();
    void    RefreshView();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CExportView)
public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnInitialUpdate();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CExportView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
    //{{AFX_MSG(CExportView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnClickDatadictTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownDatadictTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnTvCheckbox(WPARAM wp, LPARAM lp);
    afx_msg void OnToggle();
    afx_msg void OnUpdateToggle(CCmdUI* pCmdUI);
    
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    LONG OnInitializeView(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

    const DictRelation* GetRelation(HTREEITEM hItem);
    int                 GetSelItemsByRelation(const DictRelation& dict_relation, CArray<HTREEITEM,HTREEITEM>& raSelItems);

    CExportSplitterFrame*   m_pExportSplitterFrame;

    std::map<HTREEITEM, const DictRelation*> m_aMapRel_Relation_by_hitem;
    std::map<const DictRelation*, HTREEITEM> m_aMapRel_hitem_by_Relation;

    CMap<HTREEITEM,HTREEITEM,const CDictItem*,const CDictItem*> m_aMap_DictItem_by_hitem;
};

#ifndef _DEBUG  // debug version in ExptView.cpp
inline CExportDoc* CExportView::GetDocument()
   { return (CExportDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
