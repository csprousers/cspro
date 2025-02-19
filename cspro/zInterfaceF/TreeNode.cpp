#include "StdAfx.h"
#include "TreeNode.h"


// --------------------------------------------------------------------------
// TreeNode
// --------------------------------------------------------------------------

TreeNode::TreeNode()
    :   m_document(nullptr),
        m_hItem(nullptr)
{
}


std::wstring TreeNode::GetName() const
{
    const std::optional<AppFileType> app_file_type = GetAppFileType();

    return app_file_type.has_value() ? UTF8_TODO::GetWide(ToString(*app_file_type)) :
                                       std::wstring();
}


std::wstring TreeNode::GetLabel() const
{
    return GetName();
}


const std::string& TreeNode::GetPath() const
{
    return SO::Empty_string;
}
