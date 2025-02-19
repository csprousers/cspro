#pragma once

#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseConstructionReporter.h>

class CSymbolVar;
class Symbol;


// temporary binary dictionary item processing
struct BinaryStorageFor80
{
    BinaryDataAccessor binary_data_accessor;
    std::shared_ptr<Symbol> wrapped_symbol;

    static constexpr TCHAR BinaryCaseItemCharacterOffset = ' ' + 1;

    const BinaryData* GetBinaryData_noexcept(Case& data_case) const noexcept;
};


inline const BinaryData* BinaryStorageFor80::GetBinaryData_noexcept(Case& data_case) const noexcept
{
    try
    {
        if( binary_data_accessor.IsDefined() )
            return &binary_data_accessor.GetBinaryData();
    }

    catch( const CSProException& exception )
    {
        if( data_case.GetCaseConstructionReporter() != nullptr )
            data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, true, exception.what());
    }

    return nullptr;
}
