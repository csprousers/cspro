#include "StdAfx.h"
#include "CaseTreeUpdate.h"

CaseTreeUpdate::CaseTreeUpdate(UpdateType type, std::shared_ptr<CaseTreeNode> node, int childIndex)
    : m_type(type),
      m_node(node),
      m_childIndex(childIndex)
{
}

CaseTreeUpdate CaseTreeUpdate::CreateModify(std::shared_ptr<CaseTreeNode> node)
{
    return CaseTreeUpdate(UpdateType::NODE_MODIFIED, node, -1);
}

CaseTreeUpdate CaseTreeUpdate::CreateAdd(std::shared_ptr<CaseTreeNode> parentNode, int childIndex)
{
    return CaseTreeUpdate(UpdateType::NODE_ADDED, parentNode, childIndex);
}

CaseTreeUpdate CaseTreeUpdate::CreateRemove(std::shared_ptr<CaseTreeNode> parentNode, int childIndex)
{
    return CaseTreeUpdate(UpdateType::NODE_REMOVED, parentNode, childIndex);
}
