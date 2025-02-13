#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>
#include <zDictO/ItemIndexHelper.h>

class CSymbolVar;


// --------------------------------------------------------------------------
// EngineItem
// --------------------------------------------------------------------------

class ZENGINEO_API EngineItem : public Symbol
{
public:
    EngineItem(CSymbolVar& vart);

    SymbolType GetWrappedType() const override { return m_wrappedSymbolType; }

    const CSymbolVar& GetVarT() const { return m_vart; }
    CSymbolVar& GetVarT()             { return m_vart; }

    const CDictItem& GetDictItem() const { return m_dictItem; }

    const ItemIndexHelper& GetItemIndexHelper() const { return m_itemIndexHelper; }

    // ENGINECR_TODO: FindChildSymbol (value sets), WriteJsonMetadata_subclass, WriteValueToJson, UpdateValueFromJson?

private:
    CSymbolVar& m_vart;
    const CDictItem& m_dictItem;
    SymbolType m_wrappedSymbolType;
    ItemIndexHelper m_itemIndexHelper;
};



// --------------------------------------------------------------------------
// EngineItemAccessor
// --------------------------------------------------------------------------

// because of the presence of VART_EngineItemAccessor, some of these methods
// are virtual or return pointers when they could eventually return references;
// look at EIA_TODO_REMOVE when removing VART_EngineItemAccessor

class EngineItemAccessor
{
protected:
    EngineItemAccessor(const EngineItem* engine_item, ItemIndex item_index); // EIA_TODO_REMOVE can make "const EngineItem&"

public:
    virtual ~EngineItemAccessor() { }

    // returns the EngineItem associated with the wrapped symbol
    const EngineItem* GetEngineItem() const { return m_engineItem; } // EIA_TODO_REMOVE can make "const EngineItem&"

    // returns the dictionary item associated with the wrapped symbol
    virtual const CDictItem& GetDictItem() const { ASSERT(m_engineItem != nullptr); return m_engineItem->GetDictItem(); } // EIA_TODO_REMOVE make non-virtual and remove the null check

    // returns the ItemIndex associated with the wrapped symbol
    const ItemIndex GetItemIndex() const { return m_itemIndex; }

    // writes item-related metadata
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const;


    virtual const CSymbolVar& GetVarT() const { ASSERT(m_engineItem != nullptr); return m_engineItem->GetVarT(); } // EIA_TODO_REMOVE entirely, as can call: GetEngineItem().GetVarT()


    // overridable methods
    // --------------------------------------------------------------------------

    // returns true if a symbol has value/content
    virtual bool HasValue() = 0;

    // returns true if a symbol's value is valid (using validation checks specific to the symbol type)
    virtual bool IsValid() = 0;

    // returns a label for the symbol's current value;
    // if a language is specified and the label supports multiple languages but does not contain the language, blank is returned
    virtual std::wstring GetValueLabel(const std::optional<std::wstring>& language) = 0;

private:
    const EngineItem* m_engineItem; // EIA_TODO_REMOVE can make "const EngineItem&"
    const ItemIndex m_itemIndex;
};


inline EngineItemAccessor::EngineItemAccessor(const EngineItem* engine_item, ItemIndex item_index)
    :   m_engineItem(engine_item),
        m_itemIndex(std::move(item_index))
{
}
