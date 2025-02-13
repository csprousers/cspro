#include "StdAfx.h"
#include "FileTreeNode.h"


// --------------------------------------------------------------------------
// FileTreeNode
// --------------------------------------------------------------------------

FileTreeNode::FileTreeNode(AppFileType app_file_type, std::wstring path)
    :   m_appFileType(app_file_type),
        m_path(std::move(path))
{
}


std::wstring FileTreeNode::GetName() const
{
    return SO::Concatenate(TreeNode::GetName(),
                           _T(": "),
                           PortableFunctions::PathGetFilenameWithoutExtension(GetPath()));
}



// --------------------------------------------------------------------------
// ApplicationFileTreeNode
// --------------------------------------------------------------------------

ApplicationFileTreeNode::ApplicationFileTreeNode(AppFileType app_file_type, std::wstring path)
    :   FileTreeNode(app_file_type, std::move(path))
{
    ASSERT(IsApplicationType(app_file_type));
}


std::wstring ApplicationFileTreeNode::GetName() const
{
    const CAplDoc* application_doc = assert_nullable_cast<const CAplDoc*>(GetDocument());

    return ( application_doc != nullptr && !application_doc->m_bIsClosing ) ? CS2WS(application_doc->GetAppObject().GetLabel()) :
                                                                              std::wstring();
}



// --------------------------------------------------------------------------
// DictionaryFileTreeNode
// --------------------------------------------------------------------------

DictionaryFileTreeNode::DictionaryFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::Dictionary, std::move(path))
{
}


std::wstring DictionaryFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CDDTreeCtrl& DictTree = dlgBar.m_DictTree;
    DictionaryDictTreeNode* dictionary_dict_tree_node = DictTree.GetDictionaryTreeNode(GetPath());

    return ( dictionary_dict_tree_node != nullptr ) ? CS2WS(DictTree.GetItemText(dictionary_dict_tree_node->GetHItem())) :
                                                      std::wstring();
}



// --------------------------------------------------------------------------
// FormFileTreeNode
// --------------------------------------------------------------------------

FormFileTreeNode::FormFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::Form, std::move(path))
{
}


std::wstring FormFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CFormTreeCtrl& FormTree = dlgBar.m_FormTree;
    CFormNodeID* pNodeID = FormTree.GetFormNode(GetPath());

    return ( pNodeID != nullptr ) ? CS2WS(FormTree.GetItemText(pNodeID->GetHItem())) :
                                    std::wstring();
}



// --------------------------------------------------------------------------
// CodeFileTreeNode
// --------------------------------------------------------------------------

CodeFileTreeNode::CodeFileTreeNode(const CodeFile& code_file)
    :   FileTreeNode(AppFileType::Code, code_file.GetFilename()),
        m_codeType(code_file.GetCodeType())
{
}


std::wstring CodeFileTreeNode::GetName() const
{
    if( m_codeType == CodeType::LogicMain )
    {
        return ToString(AppFileType::Code);
    }

    // for external code, append the filename to differentiate it from the main file
    else
    {
        return SO::Concatenate(ToString(m_codeType),
                               _T(": "),
                               PortableFunctions::PathGetFilenameWithoutExtension(GetPath()));
    }
}



// --------------------------------------------------------------------------
// MessageFileTreeNode
// --------------------------------------------------------------------------

MessageFileTreeNode::MessageFileTreeNode(std::wstring path, bool external_messages)
    :   FileTreeNode(AppFileType::Message, std::move(path)),
        m_externalMessages(external_messages)
{
}


std::wstring MessageFileTreeNode::GetName() const
{
    // only display the filename for external messages
    return m_externalMessages ? FileTreeNode::GetName() :
                                TreeNode::GetName();
}



// --------------------------------------------------------------------------
// OrderFileTreeNode
// --------------------------------------------------------------------------

OrderFileTreeNode::OrderFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::Order, std::move(path))
{
}


std::wstring OrderFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    COrderTreeCtrl& OrderTree = dlgBar.m_OrderTree;
    const FormOrderAppTreeNode* form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(GetPath());

    return ( form_order_app_tree_node != nullptr ) ? CS2WS(OrderTree.GetItemText(form_order_app_tree_node->GetHItem())) :
                                                     std::wstring();
}



// --------------------------------------------------------------------------
// QuestionTextFileTreeNode
// --------------------------------------------------------------------------

QuestionTextFileTreeNode::QuestionTextFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::QuestionText, std::move(path))
{
}


std::wstring QuestionTextFileTreeNode::GetName() const
{
    // don't display the filename since there is only one question text file per application
    return TreeNode::GetName();
}



// --------------------------------------------------------------------------
// ReportFileTreeNode
// --------------------------------------------------------------------------

ReportFileTreeNode::ReportFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::Report, std::move(path))
{
}



// --------------------------------------------------------------------------
// ResourceFolderTreeNode
// --------------------------------------------------------------------------

ResourceFolderTreeNode::ResourceFolderTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::ResourceFolder, std::move(path))
{
}



// --------------------------------------------------------------------------
// TableSpecFileTreeNode
// --------------------------------------------------------------------------

TableSpecFileTreeNode::TableSpecFileTreeNode(std::wstring path)
    :   FileTreeNode(AppFileType::TableSpec, std::move(path))
{
}


std::wstring TableSpecFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTabTreeCtrl& TableTree = dlgBar.m_TableTree;
    TableSpecTabTreeNode* table_spec_tab_tree_node = TableTree.GetTableSpecTabTreeNode(GetPath());

    return ( table_spec_tab_tree_node != nullptr ) ? CS2WS(TableTree.GetItemText(table_spec_tab_tree_node->GetHItem())) :
                                                     std::wstring();
}
