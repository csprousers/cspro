#pragma once

#include <zToolsO/zToolsO.h>


namespace FuzzyWuzzy
{
    // descriptions of the scoring functions: https://github.com/maxbachmann/RapidFuzz
    CLASS_DECL_ZTOOLSO double Ratio(const std::wstring& text1, const std::wstring& text2);
    CLASS_DECL_ZTOOLSO double PartialRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff = 0);
    CLASS_DECL_ZTOOLSO double TokenSortRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff = 0);
    CLASS_DECL_ZTOOLSO double TokenSetRatio(const std::wstring& text1, const std::wstring& text2, double score_cutoff = 0);


    // BestMatchProcessorScorer
    class CLASS_DECL_ZTOOLSO BestMatchProcessorScorer
    {
    public:
        BestMatchProcessorScorer(const std::wstring& query, double score_cutoff);
        ~BestMatchProcessorScorer();

        size_t GetMatchCount() const     { return m_matchCount; }
        double GetBestMatchScore() const { return m_bestMatchScore; }

        bool ScoresHigher(const std::wstring& text);

    private:
        void* m_scorer;
        size_t m_matchCount;
        double m_bestMatchScore;
    };


    // BestMatchProcessor
    template<typename T = std::wstring>
    class BestMatchProcessor
    {
    public:
        BestMatchProcessor(const std::wstring& query, double score_cutoff = 0)
            :   m_scorer(query, score_cutoff)
        {
        }

        size_t GetMatchCount() const                 { return m_scorer.GetMatchCount(); }
        double GetBestMatchScore() const             { return m_bestMatch.has_value() ? m_scorer.GetBestMatchScore() : 0; }
        const std::optional<T>& GetBestMatch() const { return m_bestMatch; }

        void Match(const std::wstring& text, const T& value)
        {
            if( m_scorer.ScoresHigher(text) )
                m_bestMatch = value;
        }

        template<typename = std::enable_if_t<std::is_base_of<std::wstring, T>::value>>
        void Match(const std::wstring& text)
        {
            Match(text, text);
        }

        template<typename = std::enable_if_t<std::is_base_of<std::wstring, T>::value>>
        void Match(const std::vector<std::wstring>& texts)
        {
            const std::wstring* top_scoring_text = nullptr;

            for( const std::wstring& text : texts )
            {
                if( m_scorer.ScoresHigher(text) )
                    top_scoring_text = &text;
            }

            if( top_scoring_text != nullptr )
                m_bestMatch = *top_scoring_text;
        }

    private:
        BestMatchProcessorScorer m_scorer;
        std::optional<T> m_bestMatch;
    };
};
