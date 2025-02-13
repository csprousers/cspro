#include "stdafx.h"
#include "Imputation.h"
#include "ValueSet.h"
#include <engine/VarT.h>


Imputation::Imputation(const std::wstring& compilation_unit, size_t line_number)
    :   m_compilationUnit(PortableFunctions::PathGetFilenameWithoutExtension(compilation_unit)),
        m_lineNumber(line_number),
        m_variable(nullptr),
        m_specific(false),
        m_valueSet(nullptr),
        m_usingStat(false),
        m_imputationFrequencyIndex(SIZE_MAX),
        m_statRecordIndex(SIZE_MAX)
{
}


void Imputation::serialize(Serializer& ar, EngineData& engine_data)
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return engine_data.symbol_table; };

    if( ar.IsLoading() )
    {
        for( size_t i = ar.Read<size_t>(); i != 0; --i )
        {
            std::wstring compilation_unit_name = ar.Read<std::wstring>();
            size_t line_number = ar.Read<size_t>();

            Imputation& imputation = *engine_data.imputations.emplace_back(std::make_unique<Imputation>(std::move(compilation_unit_name), line_number));

            ar >> imputation.m_specific;

            imputation.m_variable = VPT(ar.Read<int>());

            if( ar.Read<bool>() )
                imputation.m_title = ar.Read<std::wstring>();

            if( int value_set_symbol_index = ar.Read<int>(); value_set_symbol_index != -1 )
                imputation.m_valueSet = &GetSymbolValueSet(value_set_symbol_index);

            ar >> imputation.m_usingStat;

            for( size_t sv = ar.Read<size_t>(); sv != 0; --sv )
                imputation.m_statVariables.emplace_back(VPT(ar.Read<int>()));
        }
    }

    // saving
    else
    {
        ar.Write<size_t>(engine_data.imputations.size());

        for( const Imputation& imputation : VI_V(engine_data.imputations) )
        {
            ar << imputation.m_compilationUnit
               << imputation.m_lineNumber
               << imputation.m_specific;

            ar.Write<int>(imputation.m_variable->GetSymbolIndex());

            ar.Write<bool>(imputation.m_title.has_value());

            if( imputation.m_title.has_value() )
                ar << *imputation.m_title;

            ar.Write<int>(( imputation.m_valueSet != nullptr ) ? imputation.m_valueSet->GetSymbolIndex() : -1);

            ar << imputation.m_usingStat;

            ar.Write<size_t>(imputation.m_statVariables.size());

            for( const VART* stat_variable : imputation.m_statVariables )
                ar.Write<int>(stat_variable->GetSymbolIndex());
        }
    }
}
