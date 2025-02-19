#pragma once

#include <zToolsO/zToolsO.h>

namespace FuzzyWuzzy { class BestMatchProcessor; class BestMatchProcessorScorer; }


namespace FuzzyWuzzy
{
    // descriptions of the scoring functions: https://github.com/maxbachmann/RapidFuzz
    CLASS_DECL_ZTOOLSO double Ratio(const std::string& text1, const std::string& text2);
    CLASS_DECL_ZTOOLSO double PartialRatio(const std::string& text1, const std::string& text2, double score_cutoff = 0);
    CLASS_DECL_ZTOOLSO double TokenSortRatio(const std::string& text1, const std::string& text2, double score_cutoff = 0);
    CLASS_DECL_ZTOOLSO double TokenSetRatio(const std::string& text1, const std::string& text2, double score_cutoff = 0);
}



// --------------------------------------------------------------------------
// BestMatchProcessorScorer
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO FuzzyWuzzy::BestMatchProcessorScorer
{
public:
    BestMatchProcessorScorer(const std::string& query, double score_cutoff);
    ~BestMatchProcessorScorer();

    size_t GetMatchCount() const     { return m_matchCount; }
    double GetBestMatchScore() const { return m_bestMatchScore; }

    bool ScoresHigher(const std::string& text);

private:
    void* m_scorer;
    size_t m_matchCount;
    double m_bestMatchScore;
};



// --------------------------------------------------------------------------
// BestMatchProcessor
// --------------------------------------------------------------------------

class FuzzyWuzzy::BestMatchProcessor
{
public:
    BestMatchProcessor(const std::string& query, double score_cutoff = 0)
        :   m_scorer(query, score_cutoff)
    {
    }

    size_t GetMatchCount() const                           { return m_scorer.GetMatchCount(); }
    double GetBestMatchScore() const                       { return m_bestMatch.has_value() ? m_scorer.GetBestMatchScore() : 0; }
    const std::optional<std::string>& GetBestMatch() const { return m_bestMatch; }

    void Match(const std::string& text, const std::string& value)
    {
        if( m_scorer.ScoresHigher(text) )
            m_bestMatch = value;
    }

    void Match(const std::string& text)
    {
        Match(text, text);
    }

    void Match(const std::vector<std::string>& texts)
    {
        const std::string* top_scoring_text = nullptr;

        for( const std::string& text : texts )
        {
            if( m_scorer.ScoresHigher(text) )
                top_scoring_text = &text;
        }

        if( top_scoring_text != nullptr )
            m_bestMatch = *top_scoring_text;
    }

private:
    BestMatchProcessorScorer m_scorer;
    std::optional<std::string> m_bestMatch;
};
