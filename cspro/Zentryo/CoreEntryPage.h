#pragma once

#include <Zentryo/zEntryO.h>
#include <Zentryo/CoreEntryPageField.h>

class CDEField;
class CoreEntryEngineInterface;


class CLASS_DECL_ZENTRYO CoreEntryPage
{
public:
    CoreEntryPage(CoreEntryEngineInterface* core_entry_engine_interface, CDEField* pField);

    CDEField* GetField() const { return m_pField; }

    const CString& GetOccurrenceLabel() const { return m_occurrenceLabel; }
    const CString& GetBlockName() const       { return m_blockName; }
    const CString& GetBlockLabel() const      { return m_blockLabel; }

    std::optional<std::wstring> GetBlockQuestionTextUrl() const { return m_blockCapiContentVirtualFileMapping.GetQuestionTextUrl(); }
    std::optional<std::wstring> GetBlockHelpTextUrl() const     { return m_blockCapiContentVirtualFileMapping.GetHelpTextUrl(); }

    const std::vector<CoreEntryPageField>& GetPageFields() const { return m_pageFields; }
    CoreEntryPageField& GetPageField(size_t index)               { ASSERT(index < m_pageFields.size()); return m_pageFields[index]; }
    CoreEntryPageField* GetPageField(const std::wstring& field_name);

    bool AreAnyFieldsFilled() const;

private:
    CDEField* m_pField;
    CString m_occurrenceLabel;
    CString m_blockName;
    CString m_blockLabel;
    CapiContentVirtualFileMapping m_blockCapiContentVirtualFileMapping;
    CoreEntryPageField* m_currentPageField;
    std::vector<CoreEntryPageField> m_pageFields;
};
