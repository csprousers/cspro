#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

class CDEFormFile;
class EngineDictionary;


class ZENGINEO_API Flow : public Symbol
{
public:
    Flow(std::shared_ptr<const CDEFormFile> form_file, SymbolSubType flow_subtype,
         std::shared_ptr<const EngineDictionary> engine_dictionary);

    const CDEFormFile& GetFormFile() const { return *m_formFile; }

    const EngineDictionary& GetEngineDictionary() const { return *m_engineDictionary; }

private:
    std::shared_ptr<const CDEFormFile> m_formFile;
    std::shared_ptr<const EngineDictionary> m_engineDictionary;
};
