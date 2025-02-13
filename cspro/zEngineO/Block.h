#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/RunnableSymbol.h>
#include <zLogicO/Symbol.h>

class CDEBlock;
class CDEField;


class ZENGINEO_API EngineBlock : public Symbol, public RunnableSymbol
{
public:
    EngineBlock(const CDEBlock& form_block, const Logic::SymbolTable& symbol_table);

    const CDEBlock& GetFormBlock() const { return m_formBlock; }

    const std::vector<CDEField*>& GetFields() const { return m_fields; }
    std::vector<VART*> GetVarTs() const;
    VART* GetFirstVarT() const;
    GROUPT* GetGroupT() const;
    bool ContainsField(int symbol_index) const;

    // Symbol overrides
    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

private:
    const CDEBlock& m_formBlock;
    const Logic::SymbolTable& m_symbolTable;
    std::vector<CDEField*> m_fields;
};
