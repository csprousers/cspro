#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/SymbolType.h>
#include <zToolsO/CSProException.h>

class EngineItemAccessor;
namespace JavaScript { class Executor; class Value; }
namespace Logic { class SymbolTable; }


class ZLOGICO_API Symbol
{
    friend class Logic::SymbolTable;

protected:
    Symbol(const std::string name, SymbolType symbol_type);

public:
    virtual ~Symbol() { }

    SymbolType GetType() const { return m_type; }

    SymbolSubType GetSubType() const       { return m_subtype; }
    void SetSubType(SymbolSubType subtype) { m_subtype = subtype; }

    bool IsA(SymbolType symbol_type) const { return ( m_type == symbol_type ); }

    template<typename T>
    bool IsOneOf(const T& allowable_symbol_types) const;

    template<typename... Arguments>
    bool IsOneOf(SymbolType first_type, Arguments... more_types) const;

    const std::string& GetName() const { return m_name; }

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
    virtual Symbol* FindChildSymbol(std::string_view /*symbol_name_sv*/) const { return nullptr; }


    // --------------------------------------------------------------------------
    // User-defined function parameter management
    // --------------------------------------------------------------------------

    CREATE_CSPRO_EXCEPTION(CompareDeclarationAttributesException);

    // Compares the declaration attributes for this symbol with another symbol.
    // It is guaranteed that this method will only be called with a symbol of the same type.
    // If there is a difference, a description of the difference is thrown using CompareDeclarationAttributesException.
    // The base class implementation does nothing.
    // This method is currently only used on symbols that are valid user-defined function parameters.
    virtual void CompareDeclarationAttributes(const Symbol& symbol) const;

    // Copies compile-time attributes from another symbol to this symbol.
    // It is guaranteed that this method will only be called with a symbol of the same type.
    // The base class implementation does nothing.
    // This method is currently only used on symbols that are valid user-defined function parameters.
    virtual void CopyCompileTimeAttributes(const Symbol& symbol);


    // --------------------------------------------------------------------------
    // Runtime-only methods
    // --------------------------------------------------------------------------

    // Creates a copy of the symbol containing the symbol's compilation attributes without copying any
    // of the symbol's runtime data. This method is used to copy the symbol for use in recursive
    // user-defined function calls. If the symbol does not have any modifiable runtime data,
    // the method returns nullptr (because it is not necessary to create a cloned copy).
    virtual std::unique_ptr<Symbol> CloneInInitialState() const;

    virtual void Reset();


    // --------------------------------------------------------------------------
    // Serialization methods
    // --------------------------------------------------------------------------

    void serialize(Serializer& ar);
    virtual void serialize_subclass(Serializer& ar);

    // When updating a symbol's value from JSON, the JSON serialization routines will
    // throw NoSetValueFromJsonRoutine exceptions if no routine exists for the symbol.
    // If there is an error on deserialization, the routines can throw other CSProException-derived exceptions.
    CREATE_CSPRO_EXCEPTION(NoSetValueFromJsonRoutine);

    enum class SymbolJsonOutput { Metadata, MetadataAndValue, Value };
    void WriteJson(JsonWriter& json_writer, SymbolJsonOutput symbol_json_output = SymbolJsonOutput::Metadata) const;

protected:
    // Subclasses can write out definitional information (to the existing object).
    virtual void WriteJsonMetadata_subclass(JsonWriter& json_writer) const;

public:
    virtual void WriteValueToJson(JsonWriter& json_writer) const;

    virtual void SetValueFromJson(const JsonNode& json_node);


    // --------------------------------------------------------------------------
    // JavaScript conversion methods
    // --------------------------------------------------------------------------

    // Converts the symbol's value to a JavaScript value.
    // The base class implementation throwns an exception.
    virtual JavaScript::Value GetJavaScriptValue(JavaScript::Executor& executor) const;

    // Sets the symbol's value from a JavaScript value, throwing an exception on error.
    // The base class implementation throwns an exception.
    virtual void SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value);


    // --------------------------------------------------------------------------
    // Informational methods
    // --------------------------------------------------------------------------

    // Returns a map of the text used in logic to start a declaration of a new instance of a symbol.
    static const std::map<std::string, SymbolType>& GetDeclarationTextMap();


private:
    std::string m_name;
    SymbolType m_type;
    SymbolSubType m_subtype;
    int m_symbolIndex;
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Symbol::Symbol(std::string name, const SymbolType symbol_type)
    :   m_name(std::move(name)),
        m_type(symbol_type),
        m_subtype(SymbolSubType::NoType),
        m_symbolIndex(-1)
{
}


template<typename T>
bool Symbol::IsOneOf(const T& allowable_symbol_types) const
{
    for( const SymbolType allowable_symbol_type : allowable_symbol_types )
    {
        if( IsA(allowable_symbol_type) )
            return true;
    }

    return false;
}


template<typename... Arguments>
bool Symbol::IsOneOf(const SymbolType first_type, Arguments... more_types) const
{
    for( const SymbolType symbol_type : { first_type, more_types... } )
    {
        if( IsA(symbol_type) )
            return true;
    }

    return false;
}
