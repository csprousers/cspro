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

    std::variant<const BinaryData*, std::shared_ptr<BinaryDataReader>> GetBinaryDataOrReader_noexcept(Case& data_case) const noexcept;
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
            data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, true, exception.GetErrorMessage());
    }

    return nullptr;
}


inline std::variant<const BinaryData*, std::shared_ptr<BinaryDataReader>> BinaryStorageFor80::GetBinaryDataOrReader_noexcept(Case& data_case) const noexcept
{
    if( binary_data_accessor.IsDefined() )
    {
        std::shared_ptr<BinaryDataReader> binary_data_reader = binary_data_accessor.GetSharedBinaryDataReaderIfNotQueried();

        if( binary_data_reader != nullptr )
            return binary_data_reader;
    }

    return GetBinaryData_noexcept(data_case);
}
