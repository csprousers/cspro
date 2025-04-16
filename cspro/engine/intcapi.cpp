//------------------------------------------------------------------------
//
//  INTCAPI.CPP        CSPRO CAPI INTERPRETER
//
//  History:    Date       Author   Comment
//              ---------------------------
//              07 Nov 02   RHF     Creation
//
//------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Entdrv.h"
#include "ProgramControl.h"
#include <zToolsO/Encoders.h>
#include <zCapiO/CapiName.h>
#include <zCapiO/CapiQuestionManager.h>
#include <sstream>


SharableString CIntDriver::EvaluateCapiText(const std::string& language_name, const bool is_question, const int symbol_index)
{
    const Symbol& symbol = NPT_Ref(symbol_index);

    const CapiQuestion* const question = assert_cast<const CEntryDriver*>(m_pEngineDriver)->GetQuestMgr()->GetQuestion(CapiName::Create(symbol));

    return ( question != nullptr ) ? EvaluateCapiText(*question, symbol, language_name, is_question) :
                                     SharableString();
}


SharableString CIntDriver::EvaluateCapiText(const CapiQuestion& question, const Symbol& symbol, const std::string& language_name, const bool is_question)
{
    const CapiCondition* matched_condition = nullptr;

    for( const CapiCondition& condition : question.GetConditions() )
    {
        if( condition.GetProgramIndex() == -1 ||
            EvaluateCapiLogic<bool>(symbol, condition.GetProgramIndex()) )
        {
            matched_condition = &condition;
            break;
        }
    }

    if( matched_condition == nullptr )
        return SharableString();

    const CapiText::Type text_type = is_question ? CapiText::Type::Question : CapiText::Type::Help;
    const CapiText* capi_text = matched_condition->GetText(language_name, text_type);

    if( capi_text == nullptr || capi_text->GetText()->empty() )
    {
        const Language& default_language = assert_cast<const CEntryDriver*>(m_pEngineDriver)->GetQuestMgr()->GetDefaultLanguage();
        capi_text = matched_condition->GetText(default_language.GetName(), text_type);

        if( capi_text == nullptr )
            return SharableString();
    }

    // if there are no replacement fills, we can return the text directly
    if( capi_text->GetProgramIndex() == -1 )
    {
        // process pre-8.1 text
        if( question.GetPre81FillExpressions() != nullptr && !question.GetPre81FillExpressions()->empty() )
            return EvaluatePre81CapiText(symbol, question, *capi_text);

        return capi_text->GetHtml();
    }

    return "MARKDOWN_TODO with fills";
}


template<typename T>
T CIntDriver::EvaluateCapiLogic(const Symbol& symbol, const int program_index)
{
    ASSERT(symbol.IsOneOf(SymbolType::Block, SymbolType::Variable));
    ASSERT(program_index != -1);

    // setup execution parameters
    m_iProgType = static_cast<int>(ProcType::OnFocus);
    m_iExSymbol = symbol.GetSymbolIndex();
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(symbol);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        // evaluate the condition's logic...
        if constexpr(std::is_same_v<T, bool>)
        {
            return EvaluateConditional(program_index);
        }

        // ...or the question's fill
        else
        {
            return EvaluateTextFill(program_index);
        }
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) { }

    return T();
}



// --------------------------------------------------------------------------
// Pre-CSPro 8.1 evaluation routines that were previously in:
//     - zCapiO/CapiFill.h
//     - zCapiO/CapiText.cpp
// --------------------------------------------------------------------------

namespace Pre81Capi
{
    class CapiFill
    {
    public:
        CapiFill(std::string text_to_replace, size_t delimiter_length, bool escape_fill);

        // Returns the complete fill text, including the delimiters.
        const std::string& GetTextToReplace() const { return m_textToReplace; }

        // Returns the fill text without the delimiters.
        std::string_view GetTextToEvaluate_sv() const;

        // Returns whether the fill should be escaped.
        bool EscapeFill() const { return m_escapeFill; }

        bool operator<(const CapiFill& rhs) const;

    private:
        std::string m_textToReplace;
        std::size_t m_delimiterLength;
        bool m_escapeFill;
    };


    CapiFill::CapiFill(std::string text_to_replace, const size_t delimiter_length, const bool escape_fill)
        :   m_textToReplace(std::move(text_to_replace)),
            m_delimiterLength(delimiter_length),
            m_escapeFill(escape_fill)
    {
        ASSERT(m_delimiterLength == 2 || m_delimiterLength == 3);
        ASSERT(m_textToReplace.length() >= ( m_delimiterLength * 2 ));
    }


    std::string_view CapiFill::GetTextToEvaluate_sv() const
    {
        return std::string_view(m_textToReplace.data() + m_delimiterLength,
                                m_textToReplace.length() - ( 2 * m_delimiterLength ));
    }


    bool CapiFill::operator<(const CapiFill& rhs) const
    {
        return ( m_textToReplace < rhs.m_textToReplace &&
                 m_delimiterLength < rhs.m_delimiterLength &&
                 m_escapeFill < rhs.m_escapeFill );
    }


    struct Delimiter
    {
        std::string_view characters_sv;
        bool escape_fill;
    };

    constexpr Delimiter DefaultDelimiters[] =
    {
        { "~~~", false },
        { "~~",  true }
    };

    struct NextDelimiter
    {
        const Delimiter* delimeter;
        size_t pos;
    };


    std::optional<NextDelimiter> FindNextDelimiter(const std::string_view text_sv, const size_t start)
    {
        std::optional<NextDelimiter> next_delimiter;

        for( const Delimiter& delimiter : DefaultDelimiters )
        {
            const size_t pos = text_sv.find(delimiter.characters_sv, start);

            if( ( pos != std::string_view::npos ) &&
                ( !next_delimiter.has_value() || pos < next_delimiter->pos ) )
            {
                next_delimiter = NextDelimiter { &delimiter, pos };
            }
        }

        return next_delimiter;
    }


    size_t FindEndDelimiter(const std::string_view text_sv, const NextDelimiter& start)
    {
        return text_sv.find(start.delimeter->characters_sv,
                            start.pos + start.delimeter->characters_sv.length());
    }


    std::vector<CapiFill> GetDelimitedParams(const std::string_view text_sv)
    {
        std::vector<CapiFill> params;

        std::optional<NextDelimiter> start = FindNextDelimiter(text_sv, 0);

        while( start.has_value() )
        {
            const size_t end = FindEndDelimiter(text_sv, *start);

            if( end == std::string_view::npos )
                break;

            const size_t delim_length = start->delimeter->characters_sv.length();

            if( end - start->pos > 1 )
            {
                params.emplace_back(std::string(text_sv.substr(start->pos, end - start->pos + delim_length)),
                                    delim_length,
                                    start->delimeter->escape_fill);
            }

            start = FindNextDelimiter(text_sv, end + delim_length);
        }

        return params;
    }


    std::string ReplaceFills(const std::string_view text_sv, const std::map<std::string, SharableString>& replacements)
    {
        std::stringstream ss;
        size_t pos = 0;

        while( pos < text_sv.length() )
        {
            const std::optional<NextDelimiter> next_delim = FindNextDelimiter(text_sv, pos);

            if( !next_delim.has_value() )
            {
                ss << text_sv.substr(pos);
                break;
            }

            ss << text_sv.substr(pos, next_delim->pos - pos);

            const size_t end = FindEndDelimiter(text_sv, *next_delim);

            if( end == std::string_view::npos )
            {
                ss << text_sv.substr(next_delim->pos);
                break;
            }

            const size_t delim_length = next_delim->delimeter->characters_sv.length();

            const std::string text_to_replace(text_sv.substr(next_delim->pos,
                                                             end - next_delim->pos + delim_length));

            const auto& replacement_lookup = replacements.find(text_to_replace);

            if( replacement_lookup != replacements.cend() )
            {
                if( next_delim->delimeter->escape_fill )
                {
                    ss << Encoders::ToHtml(SO::TrimRight(replacement_lookup->second.GetString()));
                }

                else
                {
                    ss << replacement_lookup->second.GetString();
                }
            }

            else
            {
                ss << text_to_replace;
            }

            pos = end + delim_length;
        }

        return ss.str();
    }
}


SharableString CIntDriver::EvaluatePre81CapiText(const Symbol& symbol, const CapiQuestion& question, const CapiText& capi_text)
{
    const std::map<std::string, int>* const pre81_fill_expressions = question.GetPre81FillExpressions();
    ASSERT(pre81_fill_expressions != nullptr && !pre81_fill_expressions->empty());
    ASSERT(capi_text.GetFormat() == CapiText::Format::Html);

    SharableString text = capi_text.GetText();

    const std::vector<Pre81Capi::CapiFill> params = Pre81Capi::GetDelimitedParams(*text);

    if( params.empty() )
        return text;

    std::map<std::string, SharableString> replacements;

    for( const Pre81Capi::CapiFill& fill : params )
    {
        replacements.try_emplace(fill.GetTextToReplace(),
                                 EvaluateCapiLogic<SharableString>(symbol, pre81_fill_expressions->at(fill.GetTextToReplace())));
    }

    return Pre81Capi::ReplaceFills(*text, replacements);
}
