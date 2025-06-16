#pragma once

#include <Zentryo/zEntryO.h>
#include <Zentryo/CaseTreeNode.h>

class CLASS_DECL_ZENTRYO CaseTreeUpdate {

public:

    enum class UpdateType {
        NODE_MODIFIED,
        NODE_ADDED,
        NODE_REMOVED
    };

    static CaseTreeUpdate CreateModify(std::shared_ptr<CaseTreeNode> node);
    static CaseTreeUpdate CreateAdd(std::shared_ptr<CaseTreeNode> parentNode, int childIndex);
    static CaseTreeUpdate CreateRemove(std::shared_ptr<CaseTreeNode> parentNode, int childIndex);

    UpdateType getType() const
    {
        return m_type;
    }

    const std::shared_ptr<CaseTreeNode>& getNode() const
    {
        return m_node;
    }

    int getChildIndex() const
    {
        return m_childIndex;
    }

private:

    CaseTreeUpdate(UpdateType type,  std::shared_ptr<CaseTreeNode> node, int childIndex);

    UpdateType m_type;
    std::shared_ptr<CaseTreeNode> m_node;
    int m_childIndex;
};

