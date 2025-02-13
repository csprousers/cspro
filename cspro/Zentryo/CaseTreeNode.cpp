#include "StdAfx.h"
#include "CaseTreeNode.h"
#include <zFormO/FormFile.h>


CaseTreeNode::CaseTreeNode(int id, CDEFormBase* formItem, CString label, CString value, CaseTreeNode::Type type, CaseTreeNode::Color color,
    const std::array<int,3>& index, bool visible)
    : m_id(id),
      m_formItem(formItem),
      m_label(label),
      m_value(value),
      m_type(type),
      m_color(color),
      m_index(index),
      m_visible(visible)
{
}

const CString& CaseTreeNode::getName() const
{
    return m_formItem->GetName();
}

int CaseTreeNode::getFieldSymbol() const
{
    return m_formItem->GetSymbol();
}
