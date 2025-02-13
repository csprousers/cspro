#pragma once

#include <zExportO/ExportWriter.h>
#include <zExportO/ExportMapping.h>
#include <zExportO/ExportDefinitions.h>
#include <zToolsO/Special.h>
#include <zCaseO/NumericCaseItem.h>


// Export Writer implementations can choose to derive from this base class that encapsulates some shared functionality.

class ExportWriterBase : public ExportWriter
{
public:
    void WriteCase(const Case& data_case) override;

protected:
    ExportWriterBase(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);

    virtual void CreateExportRecordMappings();

    std::wstring CreateUniqueName(wstring_view name_sv);

    void WriteCaseRecord(const ExportRecordMapping& export_record_mapping, const CaseRecord& case_record);

    // methods that must be implemented by derived classes
    virtual bool SupportsBinaryData() = 0;
    virtual bool IsReservedName(const std::wstring& name, bool record_name) = 0;

    virtual void StartRecord(const ExportRecordMapping& export_record_mapping) = 0;
    virtual void StartRow() = 0;
    virtual void EndRow() = 0;

    virtual void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) = 0;

    // helper methods for formatting values
    void ModifyValueForOutput(const NumericCaseItem& numeric_case_item, double& value);
    void ModifyValueForOutput(std::wstring& value);

private:
    ExportRecordMapping CreateExportRecordMapping(const std::vector<const CaseRecordMetadata*>& id_case_records,
                                                  const CaseRecordMetadata* case_record_metadata);

    std::wstring CreateFormattedName(std::set<std::wstring>& used_names, wstring_view name_sv,
                                     bool record_name, bool ensure_unique = false);

protected:
    const DataRepositoryType m_type;
    const std::shared_ptr<const CaseAccess> m_caseAccess;
    const ConnectionString m_connectionString;

    std::vector<std::vector<ExportRecordMapping>> m_exportRecordMappingByLevel;

    // options from the connection string
    bool m_suppressMappedSpecialValues;

private:
    std::set<CaseItem::Type> m_supportedCaseItemTypes;
    std::set<std::wstring> m_formattedRecordNames;
    std::set<std::wstring> m_allUsedNames;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void ExportWriterBase::ModifyValueForOutput(const NumericCaseItem& numeric_case_item, double& value)
{
    if( IsSpecial(value) && ( value == MISSING || value == REFUSED ) )
    {
        value = m_suppressMappedSpecialValues ? NOTAPPL :
                                                numeric_case_item.GetValueForOutput(value);
    }
}


inline void ExportWriterBase::ModifyValueForOutput(std::wstring& value)
{
    SO::MakeTrimRightSpace(value);
}
