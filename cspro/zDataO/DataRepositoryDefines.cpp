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
