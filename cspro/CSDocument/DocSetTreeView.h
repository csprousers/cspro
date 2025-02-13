#pragma once

#include <CSDocument/DocSetSpecDoc.h>
#include <afxcview.h>


class DocSetTreeView : public CTreeView
{
    DECLARE_DYNCREATE(DocSetTreeView)

protected:
    DocSetTreeView() { } // create from serialization only

public:
    DocSetSpecDoc& GetDocSetSpecDoc() { return *assert_cast<DocSetSpecDoc*>(GetDocument()); }
    DocSetSpec& GetDocSetSpec()       { return GetDocSetSpecDoc().GetDocSetSpec(); }

    DocSetComponent* GetDocSetComponent(HTREEITEM tree_item);
    DocSetComponent* GetSelectedDocSetComponent() { return GetDocSetComponent(GetTreeCtrl().GetSelectedItem()); }

    void RebuildTreeIfNecessary();

    struct SpecialDocumentType { std::wstring type; };
    using ExtendedDocSetComponentType = std::variant<DocSetComponent::Type, SpecialDocumentType>;

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;

    void OnRButtonDown(UINT nFlags, CPoint point);
    void OnShiftF10();

    void OnDoubleClickAndReturn(NMHDR* pNMHDR, LRESULT* pResult);

    void OnUpdateComponentMustHavePath(CCmdUI* pCmdUI);
    void OnEditComponent();
    void OnCopyFullPath();
    void OnCopyFilename();
    void OnOpenContainingFolder();

private:
    void ShowContextMenu(int x, int y);

    template<typename CF>
    void DoWithSelectedDocSetComponent(CF callback_function);

    struct DataForTree;
    DataForTree GetInitialDataForTree();
    void BuildTree(DataForTree data_for_tree);

    std::vector<std::tuple<ExtendedDocSetComponentType, const DocSetComponent*>> GetSortedDocSetComponents() const;

private:
    CImageList m_imageList;
    std::map<ExtendedDocSetComponentType, UINT> m_iconMapping;

    struct DataForTree
    {
        DataForTree() { }
        DataForTree(const DocSetSpec& doc_set_spec);

        std::optional<std::wstring> title;
        std::vector<std::shared_ptr<DocSetComponent>> doc_set_components;
        std::optional<DocSetTableOfContents> table_of_contents;
        std::shared_ptr<DocSetComponent> cover_page_document;
        std::shared_ptr<DocSetComponent> default_document;
    };

    DataForTree m_dataForCurrentTree;
};


bool operator==(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs);
bool operator!=(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs);
bool operator<(const DocSetTreeView::ExtendedDocSetComponentType& lhs, const DocSetTreeView::ExtendedDocSetComponentType& rhs);
