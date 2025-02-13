#pragma once

#include <zDiffO/zDiffO.h>

class Case;
class CaseAccess;
class CaseItem;
class CaseItemPrinter;
class CaseLevel;
class CDataDict;
class CDictRecord;
class ConnectionString;
class CStdioFileUnicode;
class DiffSpec;
class PFF;


class ZDIFFO_API Differ
{
public:
    Differ(std::shared_ptr<DiffSpec> diff_spec = nullptr);
    virtual ~Differ();

    bool Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);

protected:
    virtual void HandleNoDifferences(const PFF& pff);

private:
    void InitializeComparison();

    void Run(const ConnectionString& input_connection_string, const ConnectionString& output_connection_string);

    void CompareCase(const Case& input_case, const Case& reference_case);

    void CompareLevel(const CaseLevel& input_case_level, const CaseLevel& reference_case_level);

private:
    std::shared_ptr<DiffSpec> m_diffSpec;

    std::unique_ptr<CStdioFileUnicode> m_log;

    std::unique_ptr<CaseItemPrinter> m_caseItemPrinter;
    std::shared_ptr<CaseAccess> m_caseAccess;
    std::map<const CDictRecord*, std::vector<std::tuple<const CaseItem*, size_t>>> m_recordsCaseItemsMap;

    bool m_differencesExist;
};
