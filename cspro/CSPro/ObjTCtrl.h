#pragma once

#include <CSPro/FileTreeNode.h>
#include <zDesignerF/AppFileTypeImageList.h>


class CObjTreeCtrl : public CTreeCtrl
{
public:
    CObjTreeCtrl();

    static std::unique_ptr<AppFileTypeImageList> CreateAppFileTypeImageList();
    void InitImageList();

    FileTreeNode* GetFileTreeNode(HTREEITEM hItem) const { return ( hItem != nullptr ) ? reinterpret_cast<FileTreeNode*>(GetItemData(hItem)) :
                                                                                         nullptr; }

    FileTreeNode* FindNode(const CDocument* document) const;
    FileTreeNode* FindNode(std::string_view path_sv) const;
    FileTreeNode* FindChildNodeRecursive(const FileTreeNode& parent_file_tree_node, std::string_view path_sv) const;

    FileTreeNode* GetActiveObject() { return m_pActiveObj; }
    void SetActiveObject()          { m_pActiveObj = GetSelectedFileTreeNode(); }

    HTREEITEM InsertNode(HTREEITEM hParentItem, std::unique_ptr<FileTreeNode> file_tree_node, HTREEITEM hInsertAfter = TVI_LAST);

    // Inserts a node under the application, after all other nodes of the type identified by file_tree_node.
    HTREEITEM InsertNode(const FileTreeNode& application_file_tree_node, std::unique_ptr<FileTreeNode> file_tree_node);

    HTREEITEM InsertFormNode(std::variant<HTREEITEM, const FileTreeNode*> hParentItem_or_application_file_tree_node,
                             std::string form_file_path, AppFileType app_file_type);

    HTREEITEM InsertTableNode(HTREEITEM hParentItem, std::string table_spec_file_path);

    void DeleteNode(const FileTreeNode& tree_file_node);

    void DefaultExpand(HTREEITEM hItem, bool initialize_font = true);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnClose();
    afx_msg void OnSave();
    afx_msg void OnFileProperties();
    afx_msg void OnCopyFullPath();
    afx_msg void OnOpenContainingFolder();
    afx_msg void OnPackApplication();

private:
    FileTreeNode* GetSelectedFileTreeNode() { return GetFileTreeNode(GetSelectedItem()); }
    CDocument* GetSelectedDocument();

    template<typename CF>
    FileTreeNode* FindNode(HTREEITEM hItem, CF callback_function) const;

    void InitializeFont();

    // returns the application node containing the supplied node, returning null if not part of an application
    const FileTreeNode* GetApplicationFileTreeNode(const FileTreeNode* file_tree_node);

private:
    FileTreeNode* m_pActiveObj;
    std::unique_ptr<AppFileTypeImageList> m_appFileTypeImageList;
    std::vector<std::unique_ptr<FileTreeNode>> m_fileTreeNodes;
    static std::unique_ptr<LOGFONT> m_defaultLogfont;
    static CFont m_font;
};
