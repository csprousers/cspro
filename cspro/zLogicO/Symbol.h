#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/SymbolType.h>
#include <zToolsO/CSProException.h>

class EngineItemAccessor;
class Serializer;
namespace Logic { class SymbolTable; }


class ZLOGICO_API Symbol
{
    friend class Logic::SymbolTable;

protected:
    Symbol(const std::wstring name, SymbolType symbol_type);

public:
    virtual ~Symbol() { }

    SymbolType GetType() const { return m_type; }

    SymbolSubType GetSubType() const       { return m_subtype; }
    void SetSubType(SymbolSubType subtype) { m_subtype = subtype; }

    bool IsA(SymbolType symbol_type) const { return ( m_type == symbol_type ); }

    bool IsOneOf(const std::vector<SymbolType>& allowable_symbol_types) const;

    template<typename... Arguments>
    bool IsOneOf(SymbolType first_type, Arguments... more_types) const;

    const std::wstring& GetName() const { return m_name; }

    int GetSymbolIndex() const { return m_symbolIndex; }

    // For symbols that wrap other symbols, such as EngineItem, this method
    // returns the underlying type of the wrapped data.
    virtual SymbolType GetWrappedType() const { return SymbolType::None; }

    // If a symbol is a wrapped symbol that is part of an EngineItem, this method
    // returns an accessor to that EngineItem; otherwise it returns null.
    virtual EngineItemAccessor* GetEngineItemAccessor() const { return nullptr; }

    // Finds a child symbol with the given name. For example, a dictionary will
    // search for the name in its sections, variables, value sets, etc.
    // If no child exists, the method returns nullptr.
    virtual Symbol* FindChildSymbol(const std::wstring& /*symbol_name*/) const { return nullptr; }


    // --------------------------------------------------------------------------
    // runtime-only methods
    // --------------------------------------------------------------------------

    // Creates a copy of the symbol containing the symbol's compilation attributes without copying any
    // of the symbol's runtime data. This method is used to copy the symbol for use in recursive
    // user-defined function calls. If the symbol does not have any modifiable runtime data,
    // the method returns nullptr (because it is not necessary to create a cloned copy).
    virtual std::unique_ptr<Symbol> CloneInInitialState() const;

    virtual void Reset();


    // --------------------------------------------------------------------------
    // serialization methods
    // --------------------------------------------------------------------------

    void serialize(Serializer& ar);
    virtual void serialize_subclass(Serializer& ar);

    // when updating a symbol's value from JSON, the JSON serialization routines will
    // throw NoUpdateValueFromJsonRoutine exceptions if no routine exists for the symbol;
    // if there is an error on deserialization, the routines can throw other CSProException-derived exceptions
    CREATE_CSPRO_EXCEPTION(NoUpdateValueFromJsonRoutine)

    enum class SymbolJsonOutput { Metadata, MetadataAndValue, Value };
    void WriteJson(JsonWriter& json_writer, SymbolJsonOutput symbol_json_output = SymbolJsonOutput::Metadata) const;

protected:
    // subclasses can write out definitional information (to the existing object)
    virtual void WriteJsonMetadata_subclass(JsonWriter& json_writer) const;

public:
    virtual void WriteValueToJson(JsonWriter& json_writer) const;

    virtual void UpdateValueFromJson(const JsonNode<wchar_t>& json_node);


    // --------------------------------------------------------------------------
    // informational methods
    // --------------------------------------------------------------------------

    // returns a map of the uppercase text used in logic to start a declaration
    // of a new instance of a symbol
    static const std::map<std::wstring, SymbolType>& GetDeclarationTextMap();

private:
    std::wstring m_name;
    SymbolType m_type;
    SymbolSubType m_subtype;
    int m_symbolIndex;
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Symbol::Symbol(std::wstring name, SymbolType symbol_type)
    :   m_name(std::move(name)),
        m_type(symbol_type),
        m_subtype(SymbolSubType::NoType),
        m_symbolIndex(-1)
{
}


inline bool Symbol::IsOneOf(const std::vector<SymbolType>& allowable_symbol_types) const
{
    for( SymbolType allowable_symbol_type : allowable_symbol_types )
    {
        if( IsA(allowable_symbol_type) )
            return true;
    }

    return false;
}


template<typename... Arguments>
bool Symbol::IsOneOf(SymbolType first_type, Arguments... more_types) const
{
    for( SymbolType symbol_type : { first_type, more_types... } )
    {
        if( IsA(symbol_type) )
            return true;
    }

    return false;
}
