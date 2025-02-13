#include "StandardSystemIncludes.h"
#include "QuestionTextParamCache.h"

const std::vector<ParsedCapiParam>* QuestionTextParamCache::Get(const CapiText& text) const
{
    auto match = m_cache.find(text.GetText());
    return match == m_cache.end() ? nullptr : &match->second;
}

void QuestionTextParamCache::Put(const CapiText& text, std::vector<ParsedCapiParam> params)
{
    m_cache.emplace(text.GetText(), std::move(params));
}
