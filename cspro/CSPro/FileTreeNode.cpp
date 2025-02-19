#include "StdAfx.h"
#include "FileTreeNode.h"


// --------------------------------------------------------------------------
// FileTreeNode
// --------------------------------------------------------------------------

FileTreeNode::FileTreeNode(const AppFileType app_file_type, std::string path)
    :   m_appFileType(app_file_type),
        m_path(std::move(path))
{
}


std::wstring FileTreeNode::GetName() const
{
    return SO::ConcatenateWS(TreeNode::GetName(),
                             L": ",
                             UTF8_TODO::GetWide(PortableFunctions::PathGetFilename(GetPath())));
}



// --------------------------------------------------------------------------
// ApplicationFileTreeNode
// --------------------------------------------------------------------------

ApplicationFileTreeNode::ApplicationFileTreeNode(std::shared_ptr<const Application> application)
    :   FileTreeNode(application->GetApplicationAppFileType(), application->GetApplicationFilePath()),
        m_application(std::move(application))
{
    ASSERT(IsApplicationType(m_application->GetApplicationAppFileType()));
}


std::wstring ApplicationFileTreeNode::GetName() const
{
    return UTF8_TODO::GetWide(m_application->GetLabel());
}



// --------------------------------------------------------------------------
// DictionaryFileTreeNode
// --------------------------------------------------------------------------

DictionaryFileTreeNode::DictionaryFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::Dictionary, std::move(path))
{
}


std::wstring DictionaryFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CDDTreeCtrl& DictTree = dlgBar.m_DictTree;
    DictionaryDictTreeNode* const dictionary_dict_tree_node = DictTree.GetDictionaryTreeNode(GetPath());

    return ( dictionary_dict_tree_node != nullptr ) ? CS2WS(DictTree.GetItemText(dictionary_dict_tree_node->GetHItem())) :
                                                      std::wstring();
}



// --------------------------------------------------------------------------
// FormFileTreeNode
// --------------------------------------------------------------------------

FormFileTreeNode::FormFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::Form, std::move(path))
{
}


std::wstring FormFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CFormTreeCtrl& FormTree = dlgBar.m_FormTree;
    CFormNodeID* const pNodeID = FormTree.GetFormNode(GetPath());

    return ( pNodeID != nullptr ) ? CS2WS(FormTree.GetItemText(pNodeID->GetHItem())) :
                                    std::wstring();
}



// --------------------------------------------------------------------------
// CodeFileTreeNode
// --------------------------------------------------------------------------

CodeFileTreeNode::CodeFileTreeNode(const CodeFile& code_file)
    :   FileTreeNode(AppFileType::Code, code_file.GetFilePath()),
        m_codeType(code_file.GetCodeType())
{
}


void CodeFileTreeNode::Update(const CodeFile& code_file)
{
    m_codeType = code_file.GetCodeType();
}


std::wstring CodeFileTreeNode::GetName() const
{
    if( m_codeType == CodeType::LogicMain )
    {
        return UTF8_TODO::GetWide(ToString(AppFileType::Code));
    }

    // for external code, append the filename to differentiate it from the main file
    else
    {
        return UTF8_TODO::GetWide(SO::Concatenate(ToString(m_codeType),
                                                  ": ",
                                                  Path::GetFilename(GetPath())));
    }
}



// --------------------------------------------------------------------------
// MessageFileTreeNode
// --------------------------------------------------------------------------

MessageFileTreeNode::MessageFileTreeNode(std::string path, const bool external_messages)
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

OrderFileTreeNode::OrderFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::Order, std::move(path))
{
}


std::wstring OrderFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    COrderTreeCtrl& OrderTree = dlgBar.m_OrderTree;
    const FormOrderAppTreeNode* const form_order_app_tree_node = OrderTree.GetFormOrderAppTreeNode(GetPath());

    return ( form_order_app_tree_node != nullptr ) ? CS2WS(OrderTree.GetItemText(form_order_app_tree_node->GetHItem())) :
                                                     std::wstring();
}



// --------------------------------------------------------------------------
// QuestionTextFileTreeNode
// --------------------------------------------------------------------------

QuestionTextFileTreeNode::QuestionTextFileTreeNode(std::string path)
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

ReportFileTreeNode::ReportFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::Report, std::move(path))
{
}



// --------------------------------------------------------------------------
// ResourceFileTreeNode
// --------------------------------------------------------------------------

ResourceFileTreeNode::ResourceFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::Resource, std::move(path))
{
}



// --------------------------------------------------------------------------
// TableSpecFileTreeNode
// --------------------------------------------------------------------------

TableSpecFileTreeNode::TableSpecFileTreeNode(std::string path)
    :   FileTreeNode(AppFileType::TableSpec, std::move(path))
{
}


std::wstring TableSpecFileTreeNode::GetName() const
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTabTreeCtrl& TableTree = dlgBar.m_TableTree;
    TableSpecTabTreeNode* const table_spec_tab_tree_node = TableTree.GetTableSpecTabTreeNode(GetPath());

    return ( table_spec_tab_tree_node != nullptr ) ? CS2WS(TableTree.GetItemText(table_spec_tab_tree_node->GetHItem())) :
                                                     std::wstring();
}
