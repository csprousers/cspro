#pragma once

#include <zDataO/zDataO.h>


enum class DataRepositoryType
{
    Null,
    Text,
    SQLite,
    EncryptedSQLite,
    Memory,
    Json,
    CommaDelimited,
    SemicolonDelimited,
    TabDelimited,
    Excel,
    CSProExport,
    R,
    SAS,
    SPSS,
    Stata
};


enum class DataRepositoryNameType  { Full, Concise, ForListing };

enum class DataRepositoryOpenFlag  { CreateNew, OpenOrCreate, OpenMustExist };

enum class DataRepositoryAccess    { BatchInput, BatchOutput, BatchOutputAppend, ReadOnly, ReadWrite, EntryInput };

enum class CaseIterationStartType  { LessThan, LessThanEquals, GreaterThanEquals, GreaterThan };

enum class CaseIterationMethod     { KeyOrder, SequentialOrder };

enum class CaseIterationOrder      { Ascending, Descending };

enum class CaseIterationCaseStatus { All, NotDeletedOnly, PartialsOnly, DuplicatesOnly };

enum class CaseIterationContent    { CaseKey, CaseSummary, Case };

struct CaseIteratorParameters
{
    CaseIterationStartType start_type;
    std::variant<CString, double> first_key_or_position;
    std::optional<CString> key_prefix;

    CaseIteratorParameters(CaseIterationStartType start_type_, std::variant<CString, double> first_key_or_position_, std::optional<CString> key_prefix_)
        :   start_type(start_type_),
            first_key_or_position(std::move(first_key_or_position_)),
            key_prefix(std::move(key_prefix_))
    {
    }
};
