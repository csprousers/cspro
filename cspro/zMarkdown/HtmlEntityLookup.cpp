#include "StdAfx.h"
#include "HtmlEntityLookup.h"
#include <external/md4c/entity.h>


std::unique_ptr<HtmlEntityLookup> HtmlEntityLookup::Create()
{
    return std::make_unique<HtmlEntityLookup>();
}


bool HtmlEntityLookup::IsEntity(const std::string_view text_sv)
{
    return ( entity_lookup(text_sv.data(), text_sv.size()) != nullptr );
}
