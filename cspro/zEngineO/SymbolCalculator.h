#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/DataTypes.h>

class DictBase;


class ZENGINEO_API SymbolCalculator
{
public:
    // Returns the symbol's base name. This name will be the same as the symbol's name
    // except for dictionary-based objects, where the dictionary name will be returned.
    static std::wstring GetBaseName(const Symbol& symbol);

    // Returns the symbol's label if one exists. If not, it returns the symbol's name.
    static std::wstring GetLabel(const Symbol& symbol);

    // Returns the symbol's dictionary object (if applicable).
    static const DictBase* GetDictBase(const Symbol& symbol);

    // Returns whether or not there are multiple labels (in different languages) for the symbol.
    static bool DoMultipleLabelsExist(const Symbol& symbol);

    // Returns a symbol's DataType.
    static DataType GetDataType(const Symbol& symbol);

    // Returns a symbol's DataType only when the symbol uses one.
    static std::optional<DataType> GetOptionalDataType(const Symbol& symbol);

    // Returns the EngineDictionary associated with the symbol or null if none exists.
    static const EngineDictionary* GetEngineDictionary(const Symbol& symbol);
    static EngineDictionary* GetEngineDictionary(Symbol& symbol);

    // Returns the maximum number of occurrences for a repeating record, item, group, or block.
    static unsigned GetMaximumOccurrences(const Symbol& symbol);

    // If the symbol has occurrences, it is returned. Otherwise each parent symbol is examined
    // until one with occurrences is found. If no such symbol is found, the function returns null.
    static const Symbol* GetFirstSymbolWithOccurrences(const Symbol&symbol);

    // Returns the one-based level number of the symbol or -1 if none exists. The level number
    // of the _FF is 0 and the level number of no-level symbols with "code" is 9. These no-level
    // symbols are Application(PROC GLOBAL), Report, and UserFunction.
    static int GetLevelNumber_base1(const Symbol& symbol);

    // Returns whether the symbol's data is available on this one-based level number.
    static bool IsSymbolDataAccessible(const Symbol& symbol, int level_number);


    // --------------------------------------------------------------------------
    // the following non-static methods require access to the symbol table
    // --------------------------------------------------------------------------
    
    SymbolCalculator(const Logic::SymbolTable& symbol_table)
        : m_symbolTable(symbol_table)
    {
    }

    // Returns the DICT associated with the symbol or null if none exists.
    const DICT* GetDicT(const Symbol& symbol) const;
    DICT* GetDicT(Symbol& symbol) const;


private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

private:
    const Logic::SymbolTable& m_symbolTable;
};


// Returns whether the symbol's DataType is numeric.
inline bool IsNumeric(const Symbol& symbol) { return IsNumeric(SymbolCalculator::GetDataType(symbol)); }

// Returns whether the symbol's DataType is string.
inline bool IsString(const Symbol& symbol) { return IsString(SymbolCalculator::GetDataType(symbol)); }
