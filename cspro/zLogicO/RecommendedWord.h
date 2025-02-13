#pragma once

#include <zLogicO/ReservedWords.h>
#include <zToolsO/FuzzyWuzzy.h>


namespace Logic
{
    class RecommendedWordCalculator
    {
    public:
        static constexpr double MinRequiredScore                         = 75;
        static constexpr double StricterSuggestedScore                   = 85;
        static constexpr double MinRequiredScoreWhenMultipleMatchesExist = 90;

        RecommendedWordCalculator(const std::wstring& word, double min_score_required = MinRequiredScore)
            :   m_fuzzyMatcher(word, min_score_required)
        {
            // iterate through the reserved words
            m_fuzzyMatcher.Match(ReservedWords::GetAllReservedWords());
        }

        void Match(const std::wstring& word)
        {
            m_fuzzyMatcher.Match(word);
        }

        std::wstring GetRecommendedWord() const
        {
            // only return a suggestion if there was one match, or more than one match with a very high score
            if( m_fuzzyMatcher.GetMatchCount() == 1 || m_fuzzyMatcher.GetBestMatchScore() >= MinRequiredScoreWhenMultipleMatchesExist )
                return *m_fuzzyMatcher.GetBestMatch();

            return std::wstring();
        }

    private:
        FuzzyWuzzy::BestMatchProcessor<std::wstring> m_fuzzyMatcher;
    };
}
