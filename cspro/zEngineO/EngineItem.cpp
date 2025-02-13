#include "stdafx.h"
#include "EngineItem.h"
#include <engine/VarT.h>


// --------------------------------------------------------------------------
// EngineItem
// --------------------------------------------------------------------------

EngineItem::EngineItem(CSymbolVar& vart)
    :   Symbol(vart.GetName(), SymbolType::Item),
        m_vart(vart),
        m_dictItem(*assert_cast<const CDictItem*>(m_vart.GetDictItem())),
        m_itemIndexHelper(m_dictItem)
{
    switch( m_dictItem.GetContentType() )
    {
        case ContentType::Audio:    m_wrappedSymbolType = SymbolType::Audio;    break;
        case ContentType::Document: m_wrappedSymbolType = SymbolType::Document; break;
        case ContentType::Geometry: m_wrappedSymbolType = SymbolType::Geometry; break;
        case ContentType::Image:    m_wrappedSymbolType = SymbolType::Image;    break;
        default:                    throw ProgrammingErrorException();
    }
}



// --------------------------------------------------------------------------
// EngineItemAccessor
// --------------------------------------------------------------------------

void EngineItemAccessor::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_engineItem != nullptr);
    const ItemIndexHelper& item_index_helper = m_engineItem->GetItemIndexHelper();

    json_writer.Key(JK::occurrences);
    item_index_helper.WriteJson(json_writer, m_itemIndex,
                                json_writer.Verbose() ? ItemIndexHelper::WriteJsonMode::FullOccurrences : ItemIndexHelper::WriteJsonMode::ApplicableOccurrences);
}
