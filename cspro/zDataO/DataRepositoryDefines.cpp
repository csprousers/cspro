#include "stdafx.h"
#include "DataRepositoryDefines.h"
#include "CSWebRepositoryJsonKeys.h"


// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

DEFINE_ENUM_JSON_SERIALIZER_CLASS(CaseIterationCaseStatus,
    { CaseIterationCaseStatus::All,            JV::all },
    { CaseIterationCaseStatus::NotDeletedOnly, JV::notDeletedOnly },
    { CaseIterationCaseStatus::PartialsOnly,   JV::partialsOnly },
    { CaseIterationCaseStatus::DuplicatesOnly, JV::duplicatesOnly })

DEFINE_ENUM_JSON_SERIALIZER_CLASS(CaseIterationMethod,
    { CaseIterationMethod::KeyOrder,        JK::key },
    { CaseIterationMethod::SequentialOrder, JK::position})



// --------------------------------------------------------------------------
// CaseIteratorParameters
// --------------------------------------------------------------------------

CaseIteratorParameters::CaseIteratorParameters(CaseIterationStartType start_type_,
                                               std::variant<std::string, double> first_key_or_position_,
                                               std::optional<std::string> key_prefix_)
    :   start_type(start_type_),
        first_key_or_position(std::move(first_key_or_position_)),
        key_prefix(std::move(key_prefix_))
{
}


CaseIteratorParameters CaseIteratorParameters::CreateForKey(const CaseIterationStartType start_type,
                                                            std::variant<std::string, double> first_key_or_position)
{
    return CaseIteratorParameters(start_type, std::move(first_key_or_position), std::nullopt);
}


CaseIteratorParameters CaseIteratorParameters::CreateForKeyPrefix(std::string key_prefix)
{
    // the start type is not used for startswith filters
    return CaseIteratorParameters(CaseIterationStartType::LessThan, -1.0, std::move(key_prefix));
}
