#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>
#include <zDictO/DDClass.h>

class Case;
class CaseConstructionReporter;
class CaseMetadata;
class ProcessSummary;
class Serializer;


class ZCASEO_API CaseAccess
{
public:
    CaseAccess(const CDataDict& dictionary);
    ~CaseAccess();

    static std::unique_ptr<CaseAccess> CreateAndInitializeFullCaseAccess(const CDataDict& dictionary);

    const CDataDict& GetDataDict() const { return m_dictionary; }

    void SetRequiresFullAccess();

    void SetUsesAllCaseAttributes();

    void SetUsesNotes()       { SetFlag(&m_usesNotes); }
    bool GetUsesNotes() const { return m_usesNotes; }

    void SetUsesStatuses()       { SetFlag(&m_usesStatuses); }
    bool GetUsesStatuses() const { return m_usesStatuses; }

    void SetUsesCaseLabels()       { SetFlag(&m_usesCaseLabels); }
    bool GetUsesCaseLabels() const { return m_usesCaseLabels; }

    void SetUsesGetBuffer()       { SetFlag(&m_usesGetBuffer); }
    bool GetUsesGetBuffer() const { return m_usesGetBuffer; }

    bool GetUseDictionaryItem(const CDictItem& dict_item) const;
    void SetUseDictionaryItem(const CDictItem& dict_item);
    void SetUseAllDictionaryItems();

    void Initialize();
    bool IsInitialized() const { return m_initialized; }

    const CaseMetadata& GetCaseMetadata() const;

    const CaseItem* LookupCaseItem(const CString& item_name) const;
    const CaseItem* LookupCaseItem(const CDictItem& dict_item) const;

    // Sets an object that can optionally receive reports issued by case construction operations.
    // Cases constructed using CaseAccess can be initialized with CaseAccess' case construction
    // reporter (if one has been set).
    void SetCaseConstructionReporter(std::shared_ptr<CaseConstructionReporter> case_construction_reporter)
    {
        m_caseConstructionReporter = std::move(case_construction_reporter);
    }

    CaseConstructionReporter* GetCaseConstructionReporter() const
    {
        return m_caseConstructionReporter.get();
    }

    std::shared_ptr<CaseConstructionReporter> GetSharedCaseConstructionReporter() const
    {
        return m_caseConstructionReporter;
    }

    std::optional<std::wstring> GetUnsupportedContentTypesString(const std::set<CaseItem::Type>& supported_case_item_types) const;

    void IssueWarningIfUsingUnsupportedCaseItems(const std::set<CaseItem::Type>& supported_case_item_types) const;

    std::unique_ptr<Case> CreateCase(bool set_case_construction_reporter = false) const;

    void serialize(Serializer& ar);

private:
    void SetFlag(bool* flag)
    {
        ASSERT(!IsInitialized());
        *flag = true;
    }

private:
    const CDataDict& m_dictionary;

    bool m_usesNotes;
    bool m_usesStatuses;
    bool m_usesCaseLabels;
    bool m_usesGetBuffer;

    // if m_usedDictItems is null, then all items are used
    std::unique_ptr<std::set<CString>> m_usedDictItems;

    bool m_initialized;
    std::unique_ptr<const CaseMetadata> m_caseMetadata;
    std::map<CString, const CaseItem*> m_caseItemLookup;

    std::shared_ptr<CaseConstructionReporter> m_caseConstructionReporter;
};
