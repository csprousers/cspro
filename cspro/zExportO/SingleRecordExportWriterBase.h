#pragma once

#include <zExportO/ExportWriterBase.h>


class SingleRecordExportWriterBase : public ExportWriterBase
{
protected:
    SingleRecordExportWriterBase(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);

public:
    void WriteCase(const Case& data_case) override;

protected:
    void CreateExportRecordMappings() override;

protected:
    ExportRecordMapping* m_singleExportRecordMapping;
};
