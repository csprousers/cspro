#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbolData.h>
#include <zEngineO/EngineItem.h>
#include <zLogicO/Symbol.h>

enum TokenCode : int;


class ZENGINEO_API BinarySymbol : public Symbol
{
public:
    static bool IsBinaryToken(TokenCode token_code);
    static bool IsBinarySymbol(const Symbol& symbol);

protected:
    BinarySymbol(std::wstring name, SymbolType symbol_type);
    BinarySymbol(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);
    BinarySymbol(const BinarySymbol& binary_symbol);

public:
    const BinarySymbolData& GetBinarySymbolData() const { return m_binarySymbolData; }

    BinaryDataMetadata& GetMetadataForModification() { return m_binarySymbolData.GetMetadataForModification(); }

    // returns whether the symbol has content
    bool HasContent() const { return m_binarySymbolData.IsDefined(); }

    // returns the symbol's file path if it has content and a path on the disk exists, or an empty string otherwise
    const std::wstring& GetPath() const;

    // returns the symbol's filename (file name only) if it has content and a filename exists, or an empty string otherwise
    std::wstring GetFilenameOnly() const;

    // Symbol overrides
    EngineItemAccessor* GetEngineItemAccessor() const override { return m_engineItemAccessor.get(); }

    void Reset() override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    // overridable methods
    // --------------------------------------------------------------------------
public:
    // returns whether the symbol has content that is valid
    virtual bool HasValidContent() const { return HasContent(); }

private:
    static std::unique_ptr<EngineItemAccessor> CreateEngineItemAccessor(const EngineItem& engine_item, ItemIndex item_index, const BinarySymbol& binary_symbol);

protected:
    BinarySymbolData m_binarySymbolData;

private:
    std::unique_ptr<EngineItemAccessor> m_engineItemAccessor;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool BinarySymbol::IsBinarySymbol(const Symbol& symbol)
{
    return symbol.IsOneOf(SymbolType::Audio,
                          SymbolType::Document,
                          SymbolType::Geometry,
                          SymbolType::Image);
}


inline const std::wstring& BinarySymbol::GetPath() const
{
    return m_binarySymbolData.IsDefined() ? m_binarySymbolData.GetPath() :
                                            SO::EmptyString;
}


inline std::wstring BinarySymbol::GetFilenameOnly() const
{
    return m_binarySymbolData.IsDefined() ? m_binarySymbolData.GetFilenameOnly() :
                                            std::wstring();
}
