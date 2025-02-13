#include "StdAfx.h"
#include "CaseTreeBuilder.h"
#include <zDictO/ValueProcessor.h>
#include <zDictO/ValueSetResponse.h>
#include <ZBRIDGEO/npff.h>
#include <zEngineO/ResponseProcessor.h>
#include <zEngineO/ValueSet.h>
#include <Zentryo/CoreEntryPage.h>
#include <Zentryo/CoreEntryPageField.h>
#include <Zentryo/Runaple.h>


CaseTreeBuilder::CaseTreeBuilder(CRunAplEntry* pRunAplEntry)
    :   m_pRunAplEntry(pRunAplEntry),
        m_engineData(&pRunAplEntry->GetEntryDriver()->GetEngineData()),
        m_nextNodeId(1)
{
}


const Logic::SymbolTable& CaseTreeBuilder::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


std::shared_ptr<CaseTreeNode> CaseTreeBuilder::build(CDEFormFile* pFormFile, const CoreEntryPage* pCurrentPage)
{
    CDELevel* pLevel = pFormFile->GetLevel(0);

    std::shared_ptr<CaseTreeNode> root =
        std::make_shared<CaseTreeNode>(++m_nextNodeId, pLevel,
            pLevel->GetLabel(),
            CString(),
            CaseTreeNode::Type::LEVEL,
            CaseTreeNode::Color::PARENT,
            std::array<int, 3>{0},
            true);

    for (int iGroup = 0; iGroup < pLevel->GetNumGroups(); iGroup++) {
        CDEGroup* pGroup = pLevel->GetGroup(iGroup);

        buildSubTree(pGroup, pCurrentPage, root);
    }

    return root;
}


std::vector<CaseTreeUpdate> CaseTreeBuilder::update(const std::shared_ptr<CaseTreeNode>& root, const CoreEntryPage* pCurrentPage)
{
    std::vector<CaseTreeUpdate> updates;
    update(root, pCurrentPage, updates);
    return updates;
}


void CaseTreeBuilder::buildSubTree(CDEItemBase* pItem, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent)
{
    if (pItem->GetItemType() == CDEFormBase::Group || pItem->GetItemType() == CDEFormBase::Roster) {
        buildGroup(dynamic_cast<CDEGroup*>(pItem), pCurrentPage, parent);
    }
    else if (pItem->GetItemType() == CDEFormBase::Field) {
        buildField(dynamic_cast<CDEField*>(pItem), pCurrentPage, parent);
    }
}


void CaseTreeBuilder::buildGroup(CDEGroup* pGroup, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent)
{
    if (pGroup->GetMaxLoopOccs() > 1) { //processing groups which have maxoccs > 1

        // Group node place holder to show person (occCount)
        auto groupPlaceholder = buildGroupWithOccPlaceholderNode(pGroup, parent->getIndex());
        parent->getChildren().push_back(groupPlaceholder);

        // Now process the data occs
        std::array<int, 3> index = parent->getIndex();

        for (int iOcc = 0; iOcc < pGroup->GetDataOccs(); iOcc++) {
            setIndexForGroupRIType(index, pGroup, iOcc + 1);
            buildGroupOcc(pGroup, index, GPT(pGroup->GetSymbol())->IsOccVisible(iOcc), pCurrentPage, groupPlaceholder);
        }

    }
    else {//MaxOccs = 1. Here the group does not need a place holder for the occs as in person (30)
          //You get here if the group has one max occ like a id fields form group
          //the group itself could have a roster and it needs process occs for the roster/
          //get the childcount
        auto groupNode = buildSingleOccGroup(pGroup, parent->getIndex(), pCurrentPage);
        parent->getChildren().push_back(groupNode);
    }
}


void CaseTreeBuilder::buildGroupOcc(CDEGroup* pGroup, const std::array<int, 3>& index, bool visible,
    const CoreEntryPage* pCurrentPage,
    const std::shared_ptr<CaseTreeNode>& parent)
{
    std::shared_ptr<CaseTreeNode> groupOccNode = std::make_shared<CaseTreeNode>(++m_nextNodeId,
        pGroup,
        getGroupOccLabel(pGroup, index),
        CString(),
        CaseTreeNode::Type::GROUP_OCC,
        CaseTreeNode::Color::PARENT,
        index, visible);
    parent->getChildren().push_back(groupOccNode);

    for (int iItem = 0; iItem < pGroup->GetNumItems(); iItem++)
    {
        buildSubTree(pGroup->GetItem(iItem), pCurrentPage, groupOccNode);
    }
}


std::shared_ptr<CaseTreeNode> CaseTreeBuilder::buildGroupWithOccPlaceholderNode(CDEGroup* pGroup, const std::array<int, 3>& index)
{
    return std::make_shared<CaseTreeNode>(++m_nextNodeId,
        pGroup,
        pGroup->GetLabel(),
        CString(),
        CaseTreeNode::Type::GROUP_WITH_OCCURRENCES,
        CaseTreeNode::Color::PARENT,
        index,
        true);
}


std::shared_ptr<CaseTreeNode> CaseTreeBuilder::buildSingleOccGroup(CDEGroup* pGroup,
    const std::array<int, 3>& index,
    const CoreEntryPage* pCurrentPage)
{
    auto pGroupNode = std::make_shared<CaseTreeNode>(++m_nextNodeId,
        pGroup,
        pGroup->GetLabel(),
        CString(),
        CaseTreeNode::Type::GROUP_SINGLE,
        CaseTreeNode::Color::PARENT,
        index,
        GPT(pGroup->GetSymbol())->IsOccVisible(1));
    for (int iItem = 0; iItem < pGroup->GetNumItems(); iItem++)
    {
        buildSubTree(pGroup->GetItem(iItem), pCurrentPage, pGroupNode);
    }

    return pGroupNode;
}


void CaseTreeBuilder::buildField(CDEField* pField, const CoreEntryPage* pCurrentPage, const std::shared_ptr<CaseTreeNode>& parent)
{
    // Ignore mirror fields
    if (pField->IsMirror())
        return;

    std::shared_ptr<CaseTreeNode> fieldNode = std::make_shared<CaseTreeNode>(++m_nextNodeId,
        pField,
        pField->GetCDEText().GetText(),
        GetFieldValue(pField, parent->getIndex()),
        CaseTreeNode::CaseTreeNode::Type::FIELD,
        GetFieldColor(pField, parent->getIndex(), pCurrentPage),
        parent->getIndex(),
        !pField->IsHiddenInCaseTree());
    parent->getChildren().push_back(fieldNode);
}


void CaseTreeBuilder::update(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    switch (node->getType()) {
    case CaseTreeNode::Type::FIELD:
        updateField(node, pCurrentPage, updates);
        break;
    case CaseTreeNode::Type::GROUP_SINGLE:
        updateGroup(node, pCurrentPage, updates);
        break;
    case CaseTreeNode::Type::GROUP_WITH_OCCURRENCES:
        updateGroupPlaceholder(node, pCurrentPage, updates);
        break;
    case CaseTreeNode::Type::LEVEL:
        // Nothing to update for level nodes themselves - just update child nodes
        updateChildren(node, pCurrentPage, updates);
        break;
    case CaseTreeNode::Type::GROUP_OCC:
        // Should be update by parent group
        ASSERT(!"GROUP_OCC should be updated by parent group");
    default:
        ASSERT(!"Invalid node type");
    }
}


void CaseTreeBuilder::updateField(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    // For a field changes can be in value, color or visibility
    auto pField = dynamic_cast<CDEField*>(node->getFormItem());
    CString newValue = GetFieldValue(pField, node->getIndex());
    CaseTreeNode::Color newColor = GetFieldColor(pField, node->getIndex(), pCurrentPage);
    bool newVisible = !pField->IsHiddenInCaseTree();
    CString newLabel = pField->GetCDEText().GetText();
    if (newColor != node->getColor() || newVisible != node->getVisible() || newValue != node->getValue() || newLabel != node->getLabel()) {
        node->setValue(newValue);
        node->setColor(newColor);
        node->setVisible(newVisible);
        node->setLabel(newLabel);
        updates.push_back(CaseTreeUpdate::CreateModify(node));
    }
}


void CaseTreeBuilder::updateGroup(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    // This is a group node with no occurrences - only thing that can change is visibility
    const bool visible = GPT(node->getFieldSymbol())->IsOccVisible(1);
    if (visible != node->getVisible()) {
        node->setVisible(visible);
        updates.push_back(CaseTreeUpdate::CreateModify(node));
    }
    updateChildren(node, pCurrentPage, updates);
}


void CaseTreeBuilder::updateGroupPlaceholder(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    // Group node with occurrences - changes will only be in number of data occurrences

    auto pGroup = dynamic_cast<CDEGroup*>(node->getFormItem());

    // Process the data occs
    std::array<int, 3> index = node->getIndex();
    GROUPT* pGroupT = GPT(pGroup->GetSymbol());

    int iOcc = 0;
    int iChild = 0;
    while (iOcc < pGroup->GetDataOccs() || iChild < node->getChildren().size()) {

        if (iOcc < pGroup->GetDataOccs() && iChild < node->getChildren().size()) {
            // Occ exists in both old and existing tree
            updateGroupOcc(node->getChildren()[iChild], pGroupT->IsOccVisible(iOcc), pCurrentPage, updates);
        }
        else if (iOcc < pGroup->GetDataOccs()) {
            // Occ exists only in new tree - add to existing tree
            setIndexForGroupRIType(index, pGroup, iOcc + 1);
            buildGroupOcc(pGroup, index, pGroupT->IsOccVisible(iOcc), pCurrentPage, node);
            updates.push_back(CaseTreeUpdate::CreateAdd(node, static_cast<int>(node->getChildren().size()) - 1));
        }
        else {
            // Occ exists only in existing tree - remove from existing tree
            updates.push_back(CaseTreeUpdate::CreateRemove(node, static_cast<int>(node->getChildren().size()) - 1));
            node->getChildren().erase(node->getChildren().end() - 1);
        }

        ++iOcc;
        ++iChild;
    }
}


void CaseTreeBuilder::updateGroupOcc(const std::shared_ptr<CaseTreeNode>& node, bool visible, const CoreEntryPage * pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    // Only parts of group occ that can change are occ label and visibility
    auto pGroup = dynamic_cast<CDEGroup*>(node->getFormItem());
    CString occLabel = getGroupOccLabel(pGroup, node->getIndex());
    if (visible != node->getVisible() || occLabel != node->getLabel()) {
        node->setLabel(occLabel);
        node->setVisible(visible);
        updates.push_back(CaseTreeUpdate::CreateModify(node));
    }
    updateChildren(node, pCurrentPage, updates);
}


void CaseTreeBuilder::updateChildren(const std::shared_ptr<CaseTreeNode>& node, const CoreEntryPage* pCurrentPage, std::vector<CaseTreeUpdate>& updates)
{
    for (const auto& child : node->getChildren())
        update(child, pCurrentPage, updates);
}


namespace
{
    const DictValue* GetFieldValue_GetDictValueFromInput(std::vector<const ValueProcessor*>& value_processors, const CString& value)
    {
        const DictValue* dict_value = nullptr;

        for( const ValueProcessor* value_processor : value_processors )
        {
            dict_value = value_processor->GetDictValueFromInput(value);

            if( dict_value != nullptr )
                break;
        }

        return dict_value;
    }
}


CString CaseTreeBuilder::GetFieldValue(CDEField* pField, const std::array<int, 3>& index)
{
    const CDictItem* pItem = pField->GetDictItem();
    VART* pVarT = VPT(pField->GetSymbol());
    VARX* pVarX = pVarT->GetVarX();
    CNDIndexes index3D(ONE_BASED, index.data());
    CNDIndexes indexEngine = pVarX->PassIndexFrom3DToEngine(index3D);

    CString csData = pVarX->GetValue(indexEngine);

    // labels from the base value set will be prioritized
    std::vector<const ValueProcessor*> value_processors;

    if( pVarT->GetBaseValueSet() != nullptr )
        value_processors.push_back(&pVarT->GetBaseValueSet()->GetValueProcessor());

    if( pVarT->GetCurrentValueSet() != pVarT->GetBaseValueSet() )
        value_processors.push_back(&pVarT->GetCurrentValueSet()->GetValueProcessor());

    CString label;
    CIMSAString code = csData;

    bool show_labels_and_codes = m_pRunAplEntry->GetPifFile()->GetApplication()->GetDisplayCodesAlongsideLabels();

    // process non-checkboxes
    if( pVarT->GetEvaluatedCaptureInfo().GetCaptureType() != CaptureType::CheckBox )
    {
        const DictValue* dict_value = GetFieldValue_GetDictValueFromInput(value_processors, csData);
        const DictValuePair* first_dict_value_pair =
            ( dict_value != nullptr && dict_value->HasValuePairs() ) ? &dict_value->GetValuePair(0) : nullptr;

        // only use labels for discrete values
        if( first_dict_value_pair != nullptr && first_dict_value_pair->GetTo().IsEmpty() )
        {
            label = dict_value->GetLabel();

            // unless showing the codes with labels, don't show the code
            if( !show_labels_and_codes )
                code.Empty();
        }

        if( pVarT->IsNumeric() && ( label.IsEmpty() || show_labels_and_codes ) )
        {
            const ValueProcessor& value_processor = pVarT->GetCurrentValueProcessor();

            double numeric_value = value_processor.GetNumericFromInput(csData);

            if( numeric_value < MAXVALUE )
                code = WS2CS(ValueSetResponse::FormatValueForDisplay(*pItem, numeric_value));

            else if( first_dict_value_pair != nullptr )
                code = first_dict_value_pair->GetFrom();
        }
    }


    // process checkboxes
    else if( pVarT->GetCurrentValueSet() != nullptr )
    {
        // the code will not be shown for checkboxes
        code.Empty();

        const ResponseProcessor* response_processor = pVarT->GetCurrentValueSet()->GetResponseProcessor();
        int checkbox_width = response_processor->GetCheckboxWidth();

        for( int i = 0; i < csData.GetLength(); i += checkbox_width )
        {
            CString this_value = csData.Mid(i, checkbox_width);
            CString this_label = this_value;
            this_label.Trim();

            const DictValue* dict_value = GetFieldValue_GetDictValueFromInput(value_processors, this_value);

            if( dict_value != nullptr )
                this_label = dict_value->GetLabel();

            if( !this_label.IsEmpty() )
                label = label + ( label.IsEmpty() ? _T("") : _T(", ") ) + this_label;
        }
    }

    code.Trim();

    return label.IsEmpty()                                      ?    code :
           ( code.IsEmpty() || label.CompareNoCase(code) == 0 ) ?    label :
                                                                     ( label + _T(": ") + code );
}


CaseTreeNode::Color CaseTreeBuilder::GetFieldColor(CDEField* pField,
    const std::array<int, 3>& index, const CoreEntryPage* pCurrentPage)
{
    if (pField->IsProtected() || pField->IsMirror()) {
        return CaseTreeNode::Color::PROTECTED;
    }
    else if (pageContainsField(pCurrentPage, pField, index)) {
        return CaseTreeNode::Color::CURRENT;
    }
    else {
        VART* pVarT = VPT(pField->GetSymbol());
        VARX* pVarX = pVarT->GetVarX();
        CNDIndexes index3D(ONE_BASED, index.data());
        CNDIndexes indexEngine = pVarX->PassIndexFrom3DToEngine(index3D);

        DEFLD cField(pField->GetSymbol(), indexEngine);
        //          FLAG_NOLIGHT    0x00    never entered
        //          FLAG_MIDLIGHT   0x01    grey (PathOn) or yellow (PathOff)
        //          FLAG_HIGHLIGHT  0x02    green (highlight)
        int color = m_pRunAplEntry->GetStatus3(&cField);
        switch (color) {
        case FLAG_NOLIGHT:
        default:
            return CaseTreeNode::Color::NEVERVISITED;
        case FLAG_MIDLIGHT:
            return CaseTreeNode::Color::SKIPPED;
        case FLAG_HIGHLIGHT:
            return CaseTreeNode::Color::VISITED;
        }
    }
}


CString CaseTreeBuilder::getGroupOccLabel(CDEGroup* pGroup, const std::array<int, 3>& index)
{
    int occNum;

    CString sOccLabel;
    CDEField* pField = getFirstField(pGroup);
    if (pField) {
        if (pGroup->GetRIType() == CDEFormBase::Record) {
            occNum = index[0];
            sOccLabel = pField->GetDictItem()->GetRecord()->GetOccurrenceLabels().GetLabel(occNum - 1);
        }
        else if (pGroup->GetRIType() == CDEFormBase::Item) {
            occNum = index[1];
            if (pField->GetDictItem()->GetItemType() == ItemType::Subitem) {
                sOccLabel = pField->GetDictItem()->GetParentItem()->GetOccurrenceLabels().GetLabel(occNum - 1);
            }
            else {
                sOccLabel = pField->GetDictItem()->GetOccurrenceLabels().GetLabel(occNum - 1);
            }
        }
        else if (pGroup->GetRIType() == CDEFormBase::SubItem) {
            occNum = index[2];
            sOccLabel = pField->GetDictItem()->GetOccurrenceLabels().GetLabel(occNum - 1);
        }

        if (pGroup->GetItemType() == CDEFormBase::Roster) {
            //for rosters get the stub texts for labels if the stub text is not just the occ
            CString stubText = assert_cast<CDERoster*>(pGroup)->GetStubTextSet().GetText(occNum - 1).GetText();
            CString occToString = IntToString(occNum);
            bool bStubTextIsOcc = occToString.Compare(stubText) == 0;
            if (!stubText.IsEmpty() && !bStubTextIsOcc) {//use the custom stub text
                sOccLabel = stubText;
            }
        }
    }
    if (sOccLabel.IsEmpty()) {
        sOccLabel.Format(_T("%s (%d)"), (LPCTSTR)pGroup->GetLabel(), occNum);
    }
    return sOccLabel;
}


CDEField* CaseTreeBuilder::getFirstField(CDEGroup * pGroup)
{
    for (int iItem = 0; iItem < pGroup->GetNumItems(); iItem++) {
        if (pGroup->GetItem(iItem)->GetFormItemType() == CDEFormBase::Field)
            return dynamic_cast<CDEField*>(pGroup->GetItem(iItem));
    }

    return nullptr;
}


void CaseTreeBuilder::setIndexForGroupRIType(std::array<int, 3>& index, const CDEGroup* pGroup, int value)
{
    int i;

    switch (pGroup->GetRIType()) {
    case CDEFormBase::Record:
        i = 0;
        break;
    case CDEFormBase::Item:
        i = 1;
        break;
    case CDEFormBase::SubItem:
        i = 2;
        break;
    default:
        ASSERT(!"Not a valid group type");
    }
    index[i] = value;
}


bool CaseTreeBuilder::pageContainsField(const CoreEntryPage* pCurrentPage, const CDEField* pField, const std::array<int, 3>& index)
{
    C3DIndexes index3D(ONE_BASED, index.data());

    const std::vector<CoreEntryPageField>& page_fields = pCurrentPage->GetPageFields();

    const auto& lookup = std::find_if(page_fields.cbegin(), page_fields.cend(),
        [&](const CoreEntryPageField& page_field)
        {
            return ( page_field.GetSymbol() == pField->GetSymbol() &&
                     page_field.GetIndexes() == index3D );
        });

    return ( lookup != page_fields.cend() );
}
