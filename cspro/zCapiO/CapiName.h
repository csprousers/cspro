#pragma once

#include <zFormO/Field.h>
#include <engine/VarT.h>


// --------------------------------------------------------------------------
// CapiName
//
// Field names are preceeded by the dictionary name.
// --------------------------------------------------------------------------

class CapiName
{
public:
    static std::string Create(const CDEItemBase* const item_base)
    {
        if( item_base == nullptr )
        {
            return ReturnProgrammingError(std::string());
        }

        else if( item_base->isA(CDEFormBase::eItemType::Field) )
        {
            return assert_cast<const CDEField*>(item_base)->GetDictItem()->GetQualifiedName();
        }

        else if( item_base->isA(CDEFormBase::eItemType::Block) )
        {
            return UTF8_TODO::GetUtf8(item_base->GetName());
        }

        else
        {
            return std::string();
        }
    }

    static std::string Create(const Symbol& symbol)
    {
        return symbol.IsA(SymbolType::Variable) ? assert_cast<const VART&>(symbol).GetDictItem()->GetQualifiedName() :
                                                  symbol.GetName();
    }
};
