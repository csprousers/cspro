#include "stdafx.h"
#include "PreinitializedVariable.h"
#include "Array.h"
#include "EngineAccessor.h"
#include "List.h"
#include "WorkString.h"
#include "WorkVariable.h"


PreinitializedVariable::PreinitializedVariable(const EngineData& engine_data)
    :   m_engineData(&engine_data),
        m_symbol(nullptr),
        m_specialProcessing(SpecialProcessing::None)
{
    // only called on deserialization
}


PreinitializedVariable::PreinitializedVariable(const EngineData& engine_data,
                                               Symbol& symbol, std::vector<int> data,
                                               SpecialProcessing special_processing/* = SpecialProcessing::None*/)
    :   m_engineData(&engine_data),
        m_symbol(&symbol),
        m_data(std::move(data)),
        m_specialProcessing(special_processing)
{
}


void PreinitializedVariable::serialize(Serializer& ar)
{
    auto serialize_symbol = [&]()
    {
        ASSERT(ar.IsLoading() == ( m_symbol == nullptr ));

        if( ar.IsSaving() )
        {
            ar.Write<int>(m_symbol->GetSymbolIndex());
        }

        else
        {
            m_symbol = &m_engineData->symbol_table.GetAt(ar.Read<int>());
        }
    };

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        serialize_symbol();
        ar & m_data;
        ar.SerializeEnum(m_specialProcessing);
    }

    else
    {
        ar.SerializeEnum(m_specialProcessing);
        serialize_symbol();
        ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_encryptionType
        ar & m_data;
    }
}


void PreinitializedVariable::OnStart()
{
    // numeric
    if( m_symbol->IsA(SymbolType::WorkVariable) )
    {
        ASSERT(m_data.size() == 1);
        double value = GetNumericConstant(m_data.front());
        assert_cast<WorkVariable*>(m_symbol)->SetValue(value);
    }


    // string
    else if( m_symbol->IsOneOf(SymbolType::WorkString, SymbolType::Variable) )
    {
        ASSERT(m_data.size() == 1);
        std::wstring value = GetStringLiteral(m_data.front());

        if( m_specialProcessing == SpecialProcessing::EncryptedString )
            value = Encryptor(PreinitializedVariable::GetEncryptionType()).Decrypt(value);

        if( m_symbol->IsA(SymbolType::WorkString) )
        {
            assert_cast<WorkString*>(m_symbol)->SetString(std::move(value));
        }

        else
        {
            m_engineData->engine_accessor->ea_SetVarTValue(m_symbol, std::move(value));
        }
    }


    // arrays
    else if( m_symbol->IsA(SymbolType::Array) )
    {
        LogicArray* logic_array = assert_cast<LogicArray*>(m_symbol);
        bool repeat_values = ( m_specialProcessing == SpecialProcessing::RepeatingArray );
        ASSERT(!m_data.empty());

        if( logic_array->IsNumeric() )
        {
            std::vector<double> initial_values;

            for( int array_value : m_data )
                initial_values.emplace_back(GetNumericConstant(array_value));

            logic_array->SetInitialValues(std::move(initial_values), repeat_values);
        }

        else
        {
            std::vector<std::wstring> initial_values;

            for( int array_value : m_data )
                initial_values.emplace_back(GetStringLiteral(array_value));

            logic_array->SetInitialValues(std::move(initial_values), repeat_values);
        }
    }


    // lists
    else if( m_symbol->IsA(SymbolType::List) )
    {
        LogicList* logic_list = assert_cast<LogicList*>(m_symbol);
        ASSERT(!m_data.empty());

        for( int array_value : m_data )
        {
            if( logic_list->IsNumeric() )
            {
                logic_list->AddValue(GetNumericConstant(array_value));
            }

            else
            {
                logic_list->AddString(GetStringLiteral(array_value));
            }
        }
    }


    // unknown symbol
    else
    {
        ASSERT(false);
    }
}
