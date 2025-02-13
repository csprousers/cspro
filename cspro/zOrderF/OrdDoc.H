#pragma once

#include <zOrderF/zOrderF.h>
#include <zFormO/FormFile.h>
#include <zDesignerF/FormFileBasedDoc.h>

class COrderTreeCtrl;
class COSourceEditView;


class CLASS_DECL_ZORDERF COrderDoc : public FormFileBasedDoc
{
    DECLARE_DYNCREATE(COrderDoc)

protected:
    COrderDoc(); // create from serialization only

public:
    ~COrderDoc();

    static constexpr const TCHAR* GetExtensionWithDot() { return FileExtensions::WithDot::Order; }

    bool IsOrderLoaded() const { return m_bOrderLoaded; }

    CDEForm* GetForm(int i)   { return m_orderSpec.GetForm(i); }
    CDEForm* GetCurForm()     { return m_orderSpec.GetForm(m_iCurOrderIndex); }

    CDELevel* GetLevel(int i) { return m_orderSpec.GetLevel(i); }
    CDELevel* GetCurLevel()   { return m_orderSpec.GetLevel(m_iCurLevelIndex); }

    int GetCurOrderIndex() const { return m_iCurOrderIndex; }
    void SetCurOrderIndex(int i) { m_iCurOrderIndex = i; }

    int GetCurLevelIndex() const { return m_iCurLevelIndex; }
    void SetCurLevelIndex(int i) { m_iCurLevelIndex = i; }

    int GetCurItemIndex() const  { return m_iCurItemIndex; }
    void SetCurItemIndex(int i)  { m_iCurItemIndex = i; }

    void ReleaseDicts();
         
    COrderTreeCtrl* GetOrderTreeCtrl() const         { return m_pOrderTreeCtrl; }
    void SetOrderTreeCtrl(COrderTreeCtrl* pTreeCtrl) { m_pOrderTreeCtrl = pTreeCtrl; }

    HTREEITEM BuildAllTrees(HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);

    bool LoadOrderSpecFile(const CString& csFileName);
    bool LoadDictSpecFile();

    bool InitTreeCtrl();

    COSourceEditView* GetView();

    void SaveAllDictionaries();
    BOOL IsOrderModified();

    // DictionaryBasedDoc overrides
    CTreeCtrl* GetTreeCtrl() override;
    std::shared_ptr<const CDataDict> GetSharedDictionary() const override;

public:
    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
    void DeleteContents() override;
    void OnCloseDocument() override;
    BOOL SaveModified() override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnCustomOrder();
    afx_msg void OnUpdateCustomOrder(CCmdUI* pCmdUI);

private:
    CDEFormFile& m_orderSpec; // for now, it's really a .fmf file; maybe later this will change,
                              // as it doesn't, for ex, need the [Form] blks, only the [Level]
    bool m_bOrderLoaded;

    COrderTreeCtrl* m_pOrderTreeCtrl;

    int m_iCurOrderIndex;
    int m_iCurLevelIndex;
    int m_iCurItemIndex;
};
