#include "stdafx.h"
#include "BinarySymbol.h"
#include "EngineItem.h"
#include <zLogicO/TokenCode.h>


// --------------------------------------------------------------------------
// BinarySymbol
// --------------------------------------------------------------------------

bool BinarySymbol::IsBinaryToken(const TokenCode token_code)
{
    return ( token_code == TokenCode::TOKAUDIO ||
             token_code == TokenCode::TOKDOCUMENT ||
             token_code == TokenCode::TOKGEOMETRY ||
             token_code == TokenCode::TOKIMAGE ||
             token_code == TokenCode::TOKVIDEO );
}


BinarySymbol::BinarySymbol(std::string name, const SymbolType symbol_type)
    :   Symbol(std::move(name), symbol_type)
{
}


BinarySymbol::BinarySymbol(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   Symbol(engine_item.GetName(), engine_item.GetWrappedType()),
        m_engineItemAccessor(CreateEngineItemAccessor(engine_item, std::move(item_index), *this)),
        m_binarySymbolData(std::move(binary_data_accessor))
{
}


BinarySymbol::BinarySymbol(const BinarySymbol& binary_symbol)
    :   Symbol(binary_symbol)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


void BinarySymbol::Reset()
{
    m_binarySymbolData.Reset();
}


void BinarySymbol::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    if( m_engineItemAccessor != nullptr )
    {
        json_writer.Write(JK::item, m_engineItemAccessor->GetDictItem());
        m_engineItemAccessor->WriteJsonMetadata_subclass(json_writer);
    }
}


void BinarySymbol::WriteValueToJson(JsonWriter& json_writer) const
{
    m_binarySymbolData.WriteSymbolValueToJson(*this, json_writer);
}


void BinarySymbol::SetValueFromJson(const JsonNode& json_node)
{
    m_binarySymbolData.SetSymbolValueFromJson(*this, json_node);
}



// --------------------------------------------------------------------------
// BinarySymbolEngineItemAccessor +
// BinarySymbol::CreateEngineItemAccessor
// --------------------------------------------------------------------------

class BinarySymbolEngineItemAccessor : public EngineItemAccessor
{
public:
    BinarySymbolEngineItemAccessor(const EngineItem& engine_item, ItemIndex item_index, const BinarySymbol& binary_symbol);

    bool HasValue() override;
    bool IsValid() override;
    std::string GetValueLabel(const std::optional<std::string>& language) override;

private:
    const BinarySymbol& m_binarySymbol;
};


BinarySymbolEngineItemAccessor::BinarySymbolEngineItemAccessor(const EngineItem& engine_item, ItemIndex item_index, const BinarySymbol& binary_symbol)
    :   EngineItemAccessor(&engine_item, std::move(item_index)),
        m_binarySymbol(binary_symbol)
{
}


bool BinarySymbolEngineItemAccessor::HasValue()
{
    return m_binarySymbol.HasContent();
}


bool BinarySymbolEngineItemAccessor::IsValid()
{
    // BINARY_TYPES_TO_ENGINE_TODO: once binary items can go on forms, we could also check the capture type to determine if the data matches the capture type (e.g., is the content a barcode?)
    return m_binarySymbol.HasValidContent();
}


std::string BinarySymbolEngineItemAccessor::GetValueLabel(const std::optional<std::string>& /*language*/)
{
    const BinarySymbolData& binary_symbol_data = m_binarySymbol.GetBinarySymbolData();
    return binary_symbol_data.IsDefined() ? binary_symbol_data.GetMetadata().GetEvaluatedLabel() :
                                            std::string();
}


std::unique_ptr<EngineItemAccessor> BinarySymbol::CreateEngineItemAccessor(const EngineItem& engine_item, ItemIndex item_index, const BinarySymbol& binary_symbol)
{
    return std::make_unique<BinarySymbolEngineItemAccessor>(engine_item, std::move(item_index), binary_symbol);
}
