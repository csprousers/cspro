#pragma once

#include <Zentryo/zEntryO.h>
#include <Zentryo/CaseTreeNode.h>
#include <Zentryo/CaseTreeUpdate.h>

class CRunAplEntry;
class CDEGroup;
class CEntryDriver;
class CDEFormFile;
class CDEGroup;
class CDEItemBase;
class CDEField;
class CoreEntryPage;
struct EngineData;


/// <summary>
/// Utility class to build case tree from current form file.
/// </summary>

class CLASS_DECL_ZENTRYO CaseTreeBuilder
{
public:
    CaseTreeBuilder(CRunAplEntry* pRunAplEntry);

    std::shared_ptr<CaseTreeNode> build(CDEFormFile* pFormFile, const CoreEntryPage* pCurrentPage);

    std::vector<CaseTreeUpdate> update(const std::shared_ptr<CaseTreeNode>& root, const CoreEntryPage* pCurrentPage);

private:
    const Logic::SymbolTable& GetSymbolTable() const;

    void buildSubTree(CDEItemBase* pItem, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent);
    void buildGroup(CDEGroup* pGroup, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent);
    std::shared_ptr<CaseTreeNode> buildGroupWithOccPlaceholderNode(CDEGroup* pGroup, const std::array<int, 3>& index);
    std::shared_ptr<CaseTreeNode> buildSingleOccGroup(CDEGroup* pGroup, const std::array<int, 3>& index, const CoreEntryPage* pCurrentPage);
    void buildGroupOcc(CDEGroup* pGroup, const std::array<int, 3>& index, bool visible, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent);
    void buildField(CDEField* pField, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent);

    void update(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);
    void updateField(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);
    void updateGroup(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);
    void updateGroupPlaceholder(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);
    void updateGroupOcc(const std::shared_ptr<CaseTreeNode>& node, bool visible, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);
    void updateChildren(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates);

    CString GetFieldValue(CDEField* pField, const std::array<int, 3>& index);
    CaseTreeNode::Color GetFieldColor(CDEField* pField, const std::array<int, 3>& index, const CoreEntryPage* pCurrentPage);

    static CString getGroupOccLabel(CDEGroup* pGroup, const std::array<int, 3>& index);
    static CDEField* getFirstField(CDEGroup* pGroup);
    static void setIndexForGroupRIType(std::array<int, 3>& index, const CDEGroup* pGroup, int value);
    static bool pageContainsField(const CoreEntryPage* pCurrentPage, const CDEField* pField, const std::array<int, 3>& index);

private:
    CRunAplEntry* m_pRunAplEntry;
    EngineData* m_engineData;
    int m_nextNodeId;
};
