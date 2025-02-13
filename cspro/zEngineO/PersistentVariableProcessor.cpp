#include "stdafx.h"
#include "PersistentVariableProcessor.h"
#include "EngineAccessor.h"
#include "List.h"
#include "WorkString.h"
#include "WorkVariable.h"
#include <zUtilO/CommonStore.h>
#include <zMessageO/SystemMessageIssuer.h>


PersistentVariableProcessor::PersistentVariableProcessor(EngineData& engine_data)
    :   m_engineData(engine_data)
{
    ASSERT(engine_data.engine_accessor != nullptr);

    TransactionManager::Register(*this);
}


PersistentVariableProcessor::~PersistentVariableProcessor()
{
    TransactionManager::Deregister(*this);
}


void PersistentVariableProcessor::AddSymbol(const Symbol& symbol)
{
    ASSERT(symbol.IsOneOf(SymbolType::Array,
                          SymbolType::Audio,
                          SymbolType::Document,
                          SymbolType::Geometry,
                          SymbolType::HashMap,
                          SymbolType::Image,
                          SymbolType::List,
                          SymbolType::SystemApp,
                          SymbolType::ValueSet,
                          SymbolType::WorkString,
                          SymbolType::WorkVariable));

    m_symbolIndices.emplace_back(symbol.GetSymbolIndex());
}


bool PersistentVariableProcessor::HasSymbolWithSameName(const Symbol& symbol) const
{
    const auto& lookup = std::find_if(m_symbolIndices.cbegin(), m_symbolIndices.cend(),
                         [&](int symbol_index) { return SO::EqualsNoCase(symbol.GetName(), m_engineData.symbol_table.GetAt(symbol_index).GetName()); });

    return ( lookup != m_symbolIndices.cend() );
}


void PersistentVariableProcessor::serialize(Serializer& ar)
{
    ar & m_symbolIndices;
}


void PersistentVariableProcessor::OnStart()
{
    m_commonStore = m_engineData.engine_accessor->ea_CommonStore();

    if( m_commonStore == nullptr )
    {
        m_engineData.engine_accessor->ea_GetSystemMessageIssuer().Issue(MessageType::Error, 94107);
    }

    else
    {
        m_commonStore->SwitchTable(CommonStore::TableType::PersistentVariables);
    }

    std::set<int>& persistent_symbols_needing_reset_set = m_engineData.engine_accessor->ea_GetPersistentSymbolsNeedingResetSet();

    for( const int symbol_index : m_symbolIndices )
    {
        Symbol& symbol = m_engineData.symbol_table.GetAt(symbol_index);
        bool symbol_set = false;

        if( m_commonStore != nullptr )
        {
            std::optional<std::wstring> value = m_commonStore->GetString(symbol.GetName());

            if( value.has_value() )
            {
                try
                {
                    const JsonNode<wchar_t> json_node = Json::Parse(*value);
                    symbol.UpdateValueFromJson(json_node);
                    symbol_set = true;
                }

                catch( const CSProException& exception )
                {
                    m_engineData.engine_accessor->ea_GetSystemMessageIssuer().Issue(MessageType::Error, 94108,
                                                                                    symbol.GetName().c_str(),
                                                                                    exception.GetErrorMessage().c_str(),
                                                                                    value->c_str());
                }
            }
        }

        if( !symbol_set )
        {
            // mark the symbol as needing to be reset (in case it is a local variable with a default value)
            persistent_symbols_needing_reset_set.insert(symbol.GetSymbolIndex());
        }
    }
}


void PersistentVariableProcessor::OnStop()
{
    if( m_commonStore != nullptr )
        SaveSymbols();
}


bool PersistentVariableProcessor::CommitTransactions()
{
    // to make sure that the persistent variables tables is updated in the CommonStore when
    // launching another program (e.g., via execpff), we will save the symbols when the
    // TransactionManager indicates that transactions should be committed
    if( m_commonStore != nullptr )
    {
        SaveSymbols();
        return m_commonStore->CommitTransactions();
    }

    return true;
}


void PersistentVariableProcessor::SaveSymbols()
{
    ASSERT(m_commonStore != nullptr);

    m_commonStore->SwitchTable(CommonStore::TableType::PersistentVariables);

    // make sure the symbols are fully serialized so that they can be properly deserialized
    JsonProperties json_properties_for_complete_serialization;
    json_properties_for_complete_serialization.SetArrayFormat(JsonProperties::ArrayFormat::Sparse);
    json_properties_for_complete_serialization.SetHashMapFormat(JsonProperties::HashMapFormat::Array);
    json_properties_for_complete_serialization.SetBinaryDataFormat(JsonProperties::BinaryDataFormat::DataUrl);

    for( const int symbol_index : m_symbolIndices )
    {
        const Symbol& symbol = m_engineData.symbol_table.GetAt(symbol_index);

        try
        {
            auto json_writer = Json::CreateStringWriter();

            auto symbol_serializer_holder = json_writer->GetSerializerHelper().Register(std::make_shared<SymbolSerializerHelper>(json_properties_for_complete_serialization));

            symbol.WriteValueToJson(*json_writer);

            m_commonStore->PutString(symbol.GetName(), json_writer->GetString());
        }

        catch(...)
        {
            ASSERT(false);
        }
    }
}
