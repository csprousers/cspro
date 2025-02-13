#include "stdafx.h"
#include "Block.h"
#include <engine/VarT.h>
#include <Zissalib/GroupT.h>


EngineBlock::EngineBlock(const CDEBlock& form_block, const Logic::SymbolTable& symbol_table)
    :   Symbol(CS2WS(form_block.GetName()), SymbolType::Block),
        m_formBlock(form_block),
        m_symbolTable(symbol_table)
{
    const CDEGroup* parent_form_group = m_formBlock.GetParent();
    ASSERT(parent_form_group != nullptr);

    // get the fields and then set their block
    m_fields = parent_form_group->GetBlockFields(m_formBlock);

    for( const CDEField& form_field : VI_V(m_fields) )
    {
        // don't try to access the VART for fields without symbols (like mirror fields)
        if( form_field.GetSymbol() > 0 )
        {
            VART* pVarT = VPT(form_field.GetSymbol());
            pVarT->SetEngineBlock(*this);
        }
    }
}


std::vector<VART*> EngineBlock::GetVarTs() const
{
    std::vector<VART*> varts;

    for( const CDEField& form_field : VI_V(m_fields) )
    {
        if( form_field.GetSymbol() > 0 )
            varts.emplace_back(VPT(form_field.GetSymbol()));
    }

    return varts;
}


VART* EngineBlock::GetFirstVarT() const
{
    // skip past any mirror fields to find the first field with a symbol
    for( const CDEField& form_field : VI_V(m_fields) )
    {
        if( form_field.GetSymbol() > 0 )
            return VPT(form_field.GetSymbol());
    }

    return nullptr;
}


GROUPT* EngineBlock::GetGroupT() const
{
    const CDEGroup* parent_form_group = m_formBlock.GetParent();
    return GPT_Positive(parent_form_group->GetSymbol());
}


bool EngineBlock::ContainsField(int symbol_index) const
{
    const auto& lookup = std::find_if(m_fields.begin(), m_fields.end(),
                                      [&](const CDEField* form_field) { return ( form_field->GetSymbol() == symbol_index); });

    return ( lookup != m_fields.end() );
}


void EngineBlock::serialize_subclass(Serializer& ar)
{
    RunnableSymbol::serialize(ar);
}


void EngineBlock::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::label, m_formBlock.GetLabel())
               .Write(JK::properties, m_formBlock.GetProperties());

    json_writer.WriteArray(JK::fields, m_fields,
        [&](const CDEField* form_field)
        {
            json_writer.BeginObject()
                       .WriteIfNotBlank(JK::dictionary, form_field->GetItemDict())
                       .Write(JK::name, form_field->GetName())
                       .EndObject();
        });
}
