#include "StdAfx.h"
#include "FuzzyWuzzy.h"
#include <external/rapidfuzz/fuzz.hpp>


double FuzzyWuzzy::Ratio(const std::wstring& text1, const std::wstring& text2)
{
    return rapidfuzz::fuzz::ratio(text1, text2);
}


double FuzzyWuzzy::PartialRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff/* = 0*/)
{
    return rapidfuzz::fuzz::partial_ratio(text1, text2, score_cutoff);
}


double FuzzyWuzzy::TokenSortRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff/* = 0*/)
{
    return rapidfuzz::fuzz::token_sort_ratio(text1, text2, score_cutoff);
}


double FuzzyWuzzy::TokenSetRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff/* = 0*/)
{
    return rapidfuzz::fuzz::token_set_ratio(text1, text2, score_cutoff);
}



// --------------------------------------------------------------------------
// BestMatchProcessorScorer
// --------------------------------------------------------------------------

using BestMatchProcessorScorer_ScorerType = rapidfuzz::fuzz::CachedRatio<std::wstring::value_type>;


FuzzyWuzzy::BestMatchProcessorScorer::BestMatchProcessorScorer(const std::wstring& query, double score_cutoff)
    :   m_scorer(new BestMatchProcessorScorer_ScorerType(query)),
        m_matchCount(0),
        m_bestMatchScore(score_cutoff)
{
}


FuzzyWuzzy::BestMatchProcessorScorer::~BestMatchProcessorScorer()
{
    delete reinterpret_cast<BestMatchProcessorScorer_ScorerType*>(m_scorer);
};


bool FuzzyWuzzy::BestMatchProcessorScorer::ScoresHigher(const std::wstring& text)
{
    BestMatchProcessorScorer_ScorerType& scorer = *reinterpret_cast<BestMatchProcessorScorer_ScorerType*>(m_scorer);

    double score = scorer.similarity(text, m_bestMatchScore);

    if( score < m_bestMatchScore )
        return false;

    ++m_matchCount;
    m_bestMatchScore = score;

    return true;
}
