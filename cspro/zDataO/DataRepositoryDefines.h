#pragma once

#include <zDataO/zDataO.h>
#include <zJson/JsonSerializer.h>


// --------------------------------------------------------------------------
// Data repository definitions
// --------------------------------------------------------------------------

enum class DataRepositoryType
{
    Null,
    Text,
    SQLite,
    EncryptedSQLite,
    Memory,
    Json,
    CSWeb,
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


// --------------------------------------------------------------------------
// Case iteration definitions
// --------------------------------------------------------------------------

enum class CaseIterationStartType  { LessThan, LessThanEquals, GreaterThanEquals, GreaterThan };

enum class CaseIterationContent    { CaseKey, CaseSummary, Case };

enum class CaseIterationCaseStatus { All, NotDeletedOnly, PartialsOnly, DuplicatesOnly };

enum class CaseIterationMethod     { KeyOrder, SequentialOrder };

enum class CaseIterationOrder      { Ascending, Descending };


struct CaseIteratorParameters
{
    CaseIterationStartType start_type;
    std::variant<std::string, double> first_key_or_position;
    std::optional<std::string> key_prefix;

    ZDATAO_API CaseIteratorParameters(CaseIterationStartType start_type_,
                                      std::variant<std::string, double> first_key_or_position_,
                                      std::optional<std::string> key_prefix_);
};


// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

DECLARE_ENUM_JSON_SERIALIZER_CLASS(CaseIterationCaseStatus, ZDATAO_API)
DECLARE_ENUM_JSON_SERIALIZER_CLASS(CaseIterationMethod, ZDATAO_API)

constexpr const char* ToString(const CaseIterationStartType start_type)
{
    return ( start_type == CaseIterationStartType::LessThan )          ?   "<" :
           ( start_type == CaseIterationStartType::LessThanEquals )    ?   "<=" :
           ( start_type == CaseIterationStartType::GreaterThanEquals ) ?   ">=" :
         /*( start_type == CaseIterationStartType::GreaterThan )       ?*/ ">";
}
