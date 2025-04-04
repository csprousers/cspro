#include "StdAfx.h"
#include "CapiText.h"
#include <zToolsO/Encoders.h>
#include <sstream>


namespace
{
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


    std::shared_ptr<std::vector<CapiFill>> GetDelimitedParams(const std::string_view text_sv)
    {
        auto params = std::make_shared<std::vector<CapiFill>>();

        std::optional<NextDelimiter> start = FindNextDelimiter(text_sv, 0);

        while( start.has_value() )
        {
            const size_t end = FindEndDelimiter(text_sv, *start);

            if( end == std::string_view::npos )
                break;

            const size_t delim_length = start->delimeter->characters_sv.length();

            if( end - start->pos > 1 )
            {
                params->emplace_back(std::string(text_sv.substr(start->pos, end - start->pos + delim_length)),
                                     delim_length,
                                     start->delimeter->escape_fill);
            }

            start = FindNextDelimiter(text_sv, end + delim_length);
        }

        return params;
    }
}


const std::vector<CapiFill>& CapiText::GetFills() const
{
    if( m_params == nullptr )
        m_params = GetDelimitedParams(*m_text);

    return *m_params;
}


std::string CapiText::ReplaceFills(const std::string_view text_sv, const std::map<std::string, SharableString>& replacements)
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


void CapiText::WriteJson(JsonWriter& json_writer) const
{
    json_writer.Write(m_text);
}


void CapiText::serialize(Serializer& ar)
{
    if( ar.PredatesVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar & m_text.MakeModifiable();
    }

    else
    {
        ar & m_text;
    }
}
