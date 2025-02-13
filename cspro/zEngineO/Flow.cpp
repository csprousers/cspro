#include "stdafx.h"
#include "Flow.h"


Flow::Flow(std::shared_ptr<const CDEFormFile> form_file, SymbolSubType flow_subtype,
           std::shared_ptr<const EngineDictionary> engine_dictionary)
    :   Symbol(CS2WS(form_file->GetName()), SymbolType::Flow),
        m_formFile(std::move(form_file)),
        m_engineDictionary(std::move(engine_dictionary))
{
    SetSubType(flow_subtype);
}
