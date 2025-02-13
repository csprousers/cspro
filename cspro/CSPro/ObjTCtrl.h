#pragma once

#include <CSPro/FileTreeNode.h>


class CObjTreeCtrl : public CTreeCtrl
{
public:
    CObjTreeCtrl();

    void InitImageList();

    FileTreeNode* GetFileTreeNode(HTREEITEM hItem) const { return ( hItem != nullptr ) ? reinterpret_cast<FileTreeNode*>(GetItemData(hItem)) :
                                                                                         nullptr; }

    FileTreeNode* FindNode(const CDocument* document) const;
    FileTreeNode* FindNode(wstring_view path) const;
    FileTreeNode* FindChildNodeRecursive(const FileTreeNode& parent_file_tree_node, wstring_view path) const;    

    FileTreeNode* GetActiveObject() { return m_pActiveObj; }
    void SetActiveObject()          { m_pActiveObj = GetSelectedFileTreeNode(); }

    HTREEITEM InsertNode(HTREEITEM hParentItem, std::unique_ptr<FileTreeNode> file_tree_node);
    HTREEITEM InsertFormNode(HTREEITEM hParentItem, std::wstring form_filename, AppFileType app_file_type);
    HTREEITEM InsertTableNode(HTREEITEM hParentItem, std::wstring tab_spec_filename);

    void DeleteNode(const FileTreeNode& tree_file_node);

    void DefaultExpand(HTREEITEM hItem, bool initialize_font = true);

    bool GetDictTypeArgs(CString& sAplFileName, CString& sDictFName, CString& sParentFName);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnClose();
    afx_msg void OnSave();
    afx_msg void OnDictType();
    afx_msg void OnCopyFullPath();
    afx_msg void OnOpenContainingFolder();
    afx_msg void OnPackApplication();

private:
    FileTreeNode* GetSelectedFileTreeNode() { return GetFileTreeNode(GetSelectedItem()); }
    CDocument* GetSelectedDocument();

    template<typename CF>
    FileTreeNode* FindNode(HTREEITEM hItem, CF callback_function) const;

    void InitializeFont();

    bool GetDictTypeState();

private:
    FileTreeNode* m_pActiveObj;
    CImageList m_imageList;
    std::map<AppFileType, int> m_imageIndexMap;
    std::vector<std::unique_ptr<FileTreeNode>> m_fileTreeNodes;
    static std::unique_ptr<LOGFONT> m_defaultLogfont;
    static CFont m_font;
};
