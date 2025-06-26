#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include "HtmlTags.h"
#include <zUtilO/PortableColor.h>
#include <zMultimediaO/Image.h>
#include <zMultimediaO/QRCode.h>


namespace
{
    constexpr std::string_view TitleTag_sv                  = "title";
    constexpr std::string_view ContextTag_sv                = "context";
    constexpr std::string_view IndentTag_sv                 = "indent";
    constexpr std::string_view CenterTag_sv                 = "center";
    constexpr std::string_view BoldTag_sv                   = "b";
    constexpr std::string_view ItalicTag_sv                 = "i";
    constexpr std::string_view SuperscriptTag_sv            = "sup";
    constexpr std::string_view FontTag_sv                   = "font";
    constexpr std::string_view NoWrapTag_sv                 = "nowrap";
    constexpr std::string_view ListTag_sv                   = "list";
    constexpr std::string_view ListItemTag_sv               = "li";
    constexpr std::string_view SubheaderTag_sv              = "subheader";
    constexpr std::string_view ImageTag_sv                  = "image";
    constexpr std::string_view BarcodeTag_sv                = "barcode";
    constexpr std::string_view TopicTag_sv                  = "topic";
    constexpr std::string_view LinkTag_sv                   = "link";
    constexpr std::string_view SeeAlsoTag_sv                = "seealso";
    constexpr std::string_view ResourceTag_sv               = "resource";
    constexpr std::string_view TableTag_sv                  = "table";
    constexpr std::string_view TableCellTag_sv              = "cell";
    constexpr std::string_view LogicTag_sv                  = "logic";
    constexpr std::string_view LogicSyntaxTag_sv            = "logicsyntax";
    constexpr std::string_view LogicColorTag_sv             = "logiccolor";
    constexpr std::string_view LogicArgumentTag_sv          = "arg";
    constexpr std::string_view LogicTableTag_sv             = "logictable";
    constexpr std::string_view ActionTag_sv                 = "action";
    constexpr std::string_view MessageTag_sv                = "message";
    constexpr std::string_view ReportTag_sv                 = "report";
    constexpr std::string_view ColorTag_sv                  = "color";
    constexpr std::string_view ColorInlineTag_sv            = "colorinline";
    constexpr std::string_view ColorTagTag_sv               = "colortag";
    constexpr std::string_view PffTag_sv                    = "pff";
    constexpr std::string_view PffColorTag_sv               = "pffcolor";
    constexpr std::string_view HtmlTag_sv                   = "html";
    constexpr std::string_view MdTag_sv                     = "md";
    constexpr std::string_view NoteTag_sv                   = "note";
    constexpr std::string_view MetadataTag_sv               = "metadata";
    constexpr std::string_view DefinitionTag_sv             = "definition";
    constexpr std::string_view IncludeTag_sv                = "include";
    constexpr std::string_view CalloutTag_sv                = "callout";
    constexpr std::string_view PageBreakTag_sv              = "pagebreak";
    constexpr std::string_view BuildExtraTag_sv             = "build-extra";

    constexpr std::string_view TextAttribute_sv             = "text";
    constexpr std::string_view HeaderAttribute_sv           = "header";
    constexpr std::string_view NoHeaderAttribute_sv         = "noheader";
    constexpr std::string_view FontMonospaceAttribute_sv    = "monospace";
    constexpr std::string_view ImageNoChmAttribute_sv       = "nochm";
    constexpr std::string_view ImageWidthAttribute_sv       = "width";
    constexpr std::string_view ImageHeightAttribute_sv      = "height";
    constexpr std::string_view OrderedAttribute_sv          = "ordered";
    constexpr std::string_view NoWrapAttribute_sv           = "nowrap";
    constexpr std::string_view BorderAttribute_sv           = "border";

    constexpr std::string_view NewLineMarker_sv             = "~!~";
}


const CSDocCompilerWorker::SD& CSDocCompilerWorker::GetStaticData()
{
    static const SD sd = []()
    {
        // set up the tag definitions
        std::map<std::string_view, TagDefinition> tag_definitions =
        {
            { TitleTag_sv,         TagDefinition { true,   &TitleStartHandler, &TitleEndHandler, 0, 1 } },
            { ContextTag_sv,       TagDefinition { false,  &ContextStartHandler, { }, 1, SIZE_MAX } },
            { IndentTag_sv,        TagDefinition { true,   &IndentStartHandler, &EndTagWithContentsOfTextStack, 0, 1 } },
            { CenterTag_sv,        TagDefinition { true,   "<div align=\"center\">", "</div>" } },
            { BoldTag_sv,          TagDefinition { true,   HT::Bold[0], HT::Bold[1] } },
            { ItalicTag_sv,        TagDefinition { true,   HT::Italic[0], HT::Italic[1] } },
            { SuperscriptTag_sv,   TagDefinition { true,   "<sup>", "</sup>" } },
            { FontTag_sv,          TagDefinition { true,   &FontStartHandler, "</span>", 1, 3 } },
            { NoWrapTag_sv,        TagDefinition { true,   "<span style=\"white-space: nowrap;\">", "</span>" } },
            { ListTag_sv,          TagDefinition { true,   &ListStartHandler, &EndTagWithContentsOfTextStack, 0, 1 } },
            { ListItemTag_sv,      TagDefinition { true,   "<li>", "</li>" } },
            { SubheaderTag_sv,     TagDefinition { true,   HT::Subheader[0], HT::Subheader[1] } },
            { ImageTag_sv,         TagDefinition { false,  &ImageStartHandler, { }, 1, 6 } },
            { BarcodeTag_sv,       TagDefinition { false,  &BarcodeStartHandler, { }, 1, 8 } },
            { TopicTag_sv,         TagDefinition { false,  &TopicStartHandler, { }, 1, 1 } },
            { LinkTag_sv,          TagDefinition { true,   &LinkStartHandler, "</a>", 1, 1 } },
            { SeeAlsoTag_sv,       TagDefinition { false,  &SeeAlsoStartHandler, { }, 1, SIZE_MAX } },
            { ResourceTag_sv,      TagDefinition { true,   &ResourceStartHandler, "</a>", 1, 1 } },
            { TableTag_sv,         TagDefinition { true,   &TableStartHandler, &TableEndHandler, 1, 4 } },
            { TableCellTag_sv,     TagDefinition { true,   &TableCellStartHandler, &TableCellEndHandler, 0, 2 } },
            { LogicTag_sv,         TagDefinition { true,   { }, &LogicEndHandler } },
            { LogicSyntaxTag_sv,   TagDefinition { true,   &LogicObjectStartHandler, &LogicSyntaxEndHandler, 0, 1 } },
            { LogicColorTag_sv,    TagDefinition { true,   &LogicObjectStartHandler, &LogicColorEndHandler, 0, 1 } },
            { LogicArgumentTag_sv, TagDefinition { true,   "<span class=\"code_colorization_argument\">", "</span>" } },
            { LogicTableTag_sv,    TagDefinition { false,  &LogicTableStartHandler, { }, 1, 1 } },
            { ActionTag_sv,        TagDefinition { true,   { }, &ActionEndHandler } },
            { MessageTag_sv,       TagDefinition { true,   { }, &MessageEndHandler } },
            { ReportTag_sv,        TagDefinition { true,   &ReportStartHandler, &ReportEndHandler, 0, 2 } },
            { ColorTag_sv,         TagDefinition { true,   &ColorStartHandler, &ColorEndHandler, 1, 2 } },
            { ColorInlineTag_sv,   TagDefinition { true,   &ColorStartHandler, &ColorInlineEndHandler, 1, 1 } },
            { ColorTagTag_sv,      TagDefinition { true,   &ColorStartHandler, &ColorTagEndHandler, 1, 1 } },
            { PffTag_sv,           TagDefinition { true,   { }, &PffEndHandler } },
            { PffColorTag_sv,      TagDefinition { true,   { }, &PffColorEndHandler } },
            { HtmlTag_sv,          TagDefinition { true,   { }, { } } },
            { MdTag_sv,            TagDefinition { true,   { }, &MarkdownEndHandler } },
            { NoteTag_sv,          TagDefinition { false,  &NoteStartHandler, { }, 1, 2 } },
            { MetadataTag_sv,      TagDefinition { false,  &MetadataStartHandler, { }, 1, SIZE_MAX  } },
            { CalloutTag_sv,       TagDefinition { true,   "<div style=\"background-color: lightgrey;border:1px solid black;margin:10px;padding:10px\">", "</div>" } },
            { PageBreakTag_sv,     TagDefinition { false,  "<div class=\"new-page\" />" } },
            { BuildExtraTag_sv,    TagDefinition { false,  &BuildExtraStartHandler, { }, 1, SIZE_MAX } },
        };


        // set up the block tags
        std::set<std::string> block_tags;
        std::string block_tag_match_regex_text;

        for( const std::string_view& tag_sv : { LogicTag_sv,
                                                LogicSyntaxTag_sv,
                                                MessageTag_sv,
                                                ReportTag_sv,
                                                ColorTag_sv,
                                                ColorTagTag_sv,
                                                PffTag_sv,
                                                HtmlTag_sv,
                                                MdTag_sv } )
        {
            const std::string& tag = *block_tags.insert(std::string(tag_sv)).first;

            block_tag_match_regex_text.append(block_tag_match_regex_text.empty() ? ".*<(" : "|");
            block_tag_match_regex_text.append(Encoders::ToRegex(tag));
        }

        block_tag_match_regex_text.append(")(?:\\s.*|)>.*");

        return SD
        {
            std::move(tag_definitions),
            std::move(block_tags),
            std::regex(block_tag_match_regex_text)
        };
    }();

    return sd;
}


CSDocCompilerWorker::CSDocCompilerWorker(CSDocCompilerSettings& settings, const std::string_view text_sv)
    :   m_sd(GetStaticData()),
        m_settings(settings),
        m_inBlockTag(false)
{
    const std::string preprocessed_text = PreprocessTextForDefinitionsAndIncludes(text_sv);

    CreateParagraphsFromPreprocessedText(preprocessed_text);
}


std::string CSDocCompilerWorker::CreateHtml()
{
    ASSERT(m_html.empty());

    std::optional<size_t> title_pos;

    if( m_settings.AddHtmlHeader() )
    {
        m_html.append(HtmlWriter::DefaultHeader_sv);

        m_html.append("<title>");
        title_pos = m_html.length();
        m_html.append("</title>\n");

        m_html.append(m_settings.GetStylesheetsHtml());

        m_html.append("</head>\n<body>\n");
    }

    const auto [start_document_html, end_document_html] = m_settings.GetHtmlToWrapDocument();
    m_html.append(start_document_html);

    for( const std::string& paragraph : m_paragraphs )
        ProcessParagraph(paragraph);

    // by now the title has been processed
    if( !m_title.has_value() )
    {
        m_settings.ClearTitleForCompilationFilePath();

        if( m_settings.TitleIsRequired() )
            throw CSProException("A title must be specified for the document.");
    }

    if( title_pos.has_value() && !m_settings.GetCompilationFilePath().empty() )
    {
        // only insert the title when defined, suppressing errors getting a title if the title is not required
        try
        {
            m_html.insert(*title_pos, Encoders::ToHtml(m_settings.GetHtmlHeaderTitle(m_settings.GetCompilationFilePath())));
        }

        catch(...)
        {
            if( m_settings.TitleIsRequired() )
                throw;
        }
    }

    m_html.append(end_document_html);

    if( m_settings.AddHtmlFooter() )
        m_html.append("</body>\n</html>\n");

    return m_html;
}


CSDocCompilerWorker::TagPosition CSDocCompilerWorker::GetTagPosition(const std::string_view text_sv, const size_t start_pos/* = 0*/)
{
    TagPosition tag_position { text_sv.find('<', start_pos), std::string_view::npos };

    while( true )
    {
        if( tag_position.start == std::string_view::npos )
            return tag_position;

        // tags must be followed by a letter or by the end tag character
        const size_t next_pos = tag_position.start + 1;

        if( next_pos == text_sv.length() )
            return tag_position;

        const char next_ch = text_sv[next_pos];

        if( next_ch == '/' || std::isalpha(next_ch) )
            break;

        tag_position.start = text_sv.find('<', next_pos);
    }

    ASSERT(tag_position.start != std::string_view::npos);

    for( size_t pos = tag_position.start + 1; pos < text_sv.length(); ++pos )
    {
        const char ch = text_sv[pos];

        if( ch == '>' )
        {
            tag_position.end = pos;
            break;
        }

        else if( ch == '<' )
        {
            // in a block tag, the first < may have been a false tag (e.g., a logic statement like: AGE < 12)
            if( m_inBlockTag )
                return GetTagPosition(text_sv, pos);
        }

        else if( ch == '"' || ch == '\'' )
        {
            if( !m_inBlockTag )
            {
                ParseStringLiteral(text_sv, pos);
                ASSERT(text_sv[pos] == ch);
            }
        }
    }

    return tag_position;
}


std::vector<std::string> CSDocCompilerWorker::GetTagComponents(const std::string_view text_sv, const TagPosition& tag_position) const
{
    ASSERT(text_sv[tag_position.start] == '<' && text_sv[tag_position.end] == '>');

    const std::string_view tag_sv = text_sv.substr(tag_position.start + 1, tag_position.end - tag_position.start - 1);

    // see if this is a version 8 tag
    const auto& first_whitespace_pos = std::find_if(tag_sv.cbegin(), tag_sv.cend(),
                                                    [](const char ch) { return std::isspace(ch); });

    if( first_whitespace_pos != tag_sv.cend() )
    {
        const size_t first_whitespace_index = std::distance(tag_sv.cbegin(), first_whitespace_pos);
        const std::string_view tag_name_sv = tag_sv.substr(0, first_whitespace_index);
        const auto& tag_definition_lookup = m_sd.tag_definitions.find(tag_name_sv);

        if( tag_definition_lookup != m_sd.tag_definitions.cend() &&
            std::holds_alternative<TagDefinition::StartHandlerFunctionV8>(tag_definition_lookup->second.start_handler) )
        {
            return GetTagComponentsV8(tag_name_sv, tag_sv.substr(first_whitespace_index));
        }
    }

    return GetTagComponentsV0(tag_sv);
}


std::vector<std::string> CSDocCompilerWorker::GetTagComponentsV0(const std::string_view tag_sv)
{
    const size_t quotemark_pos = tag_sv.find_first_of("\"'");

    // when the tag components do not use quotes, we do not have to process the tag in any special fashion
    if( quotemark_pos == std::string_view::npos )
        return SO::SplitString(tag_sv, ' ', true, false);

    std::vector<std::string> tag_components;
    bool add_new_tag_component_with_next_ch = true;

    for( size_t pos = 0; pos < tag_sv.length(); ++pos )
    {
        const char ch = tag_sv[pos];

        if( ch == '"' || ch == '\'' )
        {
            tag_components.emplace_back(ParseStringLiteral(tag_sv, pos));
            add_new_tag_component_with_next_ch = true;
        }

        else if( std::isspace(ch) )
        {
            add_new_tag_component_with_next_ch = true;
        }

        else
        {
            if( add_new_tag_component_with_next_ch )
            {
                tag_components.emplace_back();
                add_new_tag_component_with_next_ch = false;
            }

            tag_components.back().push_back(ch);
        }
    }

    return tag_components;
}


std::vector<std::string> CSDocCompilerWorker::GetTagComponentsV8(const std::string_view tag_name_sv, const std::string_view rest_of_tag_sv)
{
    std::vector<std::string> tag_components = { std::string(tag_name_sv) };

    enum class ExpectedEntity { NameStart, NameContinue, Equals, Value, NothingAsTagIsEnded };
    ExpectedEntity expected_entity = ExpectedEntity::NameStart;

    auto ensure_attribute_has_value = [&]()
    {
        // add a blank value for an attribute when no value was explicitly specified
        if( tag_components.size() % 2 == 0 )
            tag_components.emplace_back();
    };

    for( size_t pos = 0; pos < rest_of_tag_sv.length(); ++pos )
    {
        const char ch = rest_of_tag_sv[pos];

        if( expected_entity == ExpectedEntity::NothingAsTagIsEnded )
        {
            throw CSProException("No additional text can appear after the tag closing character '/'.");
        }

        else if( std::isspace(ch) )
        {
            if( expected_entity == ExpectedEntity::NameContinue )
                expected_entity = ExpectedEntity::Equals;
        }

        else if( ch == '/' )
        {
            ensure_attribute_has_value();
            tag_components.emplace_back("/");
            expected_entity = ExpectedEntity::NothingAsTagIsEnded;
        }

        else if( ch == '=' )
        {
            if( expected_entity != ExpectedEntity::NameContinue &&
                expected_entity != ExpectedEntity::Equals )
            {
                throw CSProException("An equals character can only follow the specification of the attribute name.");
            }

            expected_entity = ExpectedEntity::Value;
        }

        else if( expected_entity == ExpectedEntity::Value )
        {
            if( !is_quotemark(ch) )
                throw CSProException("An attribute value must be specified as a single- or double-quoted string.");

            tag_components.emplace_back(ParseStringLiteral(rest_of_tag_sv, pos));
            expected_entity = ExpectedEntity::NameStart;
        }

        else
        {
            ASSERT(expected_entity == ExpectedEntity::NameStart ||
                   expected_entity == ExpectedEntity::NameContinue ||
                   expected_entity == ExpectedEntity::Equals);

            // handle value-less attributes
            if( expected_entity == ExpectedEntity::Equals )
                ensure_attribute_has_value();

            if( !is_tokch(ch) )
                throw CSProException("A tag attribute cannot contain the character '%c'.", ch);

            if( expected_entity == ExpectedEntity::NameContinue )
            {
                tag_components.back().push_back(ch);
            }

            else
            {
                tag_components.emplace_back(1, ch);
                expected_entity = ExpectedEntity::NameContinue;
            }
        }
    }

    if( expected_entity == ExpectedEntity::NameContinue ||
        expected_entity == ExpectedEntity::Equals )
    {
        ensure_attribute_has_value();
    }

    else if( expected_entity == ExpectedEntity::Value )
    {
        throw CSProException("A tag value must be specified after the '=' following the tag attribute '%s'.",
                             tag_components.back().c_str());
    }

    ASSERT(tag_components.size() % 2 == 1 || tag_components.back() == "/");

    // make sure each attribute name is unique
    for( size_t i = 3; i < tag_components.size(); i += 2 )
    {
        const std::string& this_tag_component = tag_components[i];

        for( size_t j = 1; j < i; j += 2 )
        {
            if( SO::EqualsNoCase(this_tag_component, tag_components[j]) )
            {
                throw CSProException("More than one tag attribute with the name '%s' cannot be specified.",
                                     this_tag_component.c_str());
            }
        }
    }

    return tag_components;
}


std::map<std::string_view, std::string_view> CSDocCompilerWorker::ProcessTagComponentsV8(const cs::span<const std::string> tag_components)
{
    std::map<std::string_view, std::string_view> mapped_tag_components;

    ASSERT(tag_components.size() % 2 == 0);
    const auto& tag_components_end = tag_components.end();

    for( auto tag_components_itr = tag_components.begin(); tag_components_itr != tag_components_end; tag_components_itr += 2 )
    {
        ASSERT(!tag_components_itr->empty());
        mapped_tag_components.try_emplace(*tag_components_itr, *( tag_components_itr + 1 ));
    }

    return mapped_tag_components;
}


void CSDocCompilerWorker::ValidateTagComponentsV8(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components,
                                                  const cs::span<const std::string_view> required_attribute_names,
                                                  const cs::span<const std::string_view> optional_attribute_names/* = cs::span<const std::string_view>()*/,
                                                  const cs::span<const std::string_view> attribute_names_where_value_can_be_blank/* = cs::span<const std::string_view>()*/)
{
    auto index_in_span = [](const std::string_view& name_sv, const cs::span<const std::string_view>& attribute_names)
    {
        size_t index = 0;

        for( const std::string_view& attribute_name_sv : attribute_names )
        {
            if( name_sv == attribute_name_sv )
                return index;

            ++index;
        }

        return SIZE_MAX;
    };

    std::vector<bool> required_attributes_present(required_attribute_names.size(), false);

    for( const auto& [name_sv, value_sv] : tag_components )
    {
        const size_t index_in_required_attribute_names = index_in_span(name_sv, required_attribute_names);

        if( index_in_required_attribute_names != SIZE_MAX )
        {
            required_attributes_present[index_in_required_attribute_names] = true;
        }

        else if( index_in_span(name_sv, optional_attribute_names) == SIZE_MAX )
        {
            throw CSProException("The '%s' tag contains an unrecognized attribute '%s'.",
                                 start_tag.c_str(),
                                 std::string(name_sv).c_str());
        }

        if( SO::IsWhitespace(value_sv) && index_in_span(name_sv, attribute_names_where_value_can_be_blank) == SIZE_MAX )
        {
            throw CSProException("The '%s' tag contains an attribute '%s' with a blank value. The value must be non-blank.",
                                 start_tag.c_str(),
                                 std::string(name_sv).c_str());
        }
    }

    const auto& missing_required_attribute_lookup = std::find(required_attributes_present.cbegin(), required_attributes_present.cend(), false);

    if( missing_required_attribute_lookup != required_attributes_present.cend() )
    {
        const size_t index = std::distance(required_attributes_present.cbegin(), missing_required_attribute_lookup);

        throw CSProException("The '%s' tag requires the specification of the attribute '%s'.",
                             start_tag.c_str(), std::string(required_attribute_names[index]).c_str());
    }
}


template<typename CF>
void CSDocCompilerWorker::ExecuteWithTagValue(const std::map<std::string_view, std::string_view>& tag_components,
                                              const std::string_view attribute_name_sv, const CF callback_function)
{
    const auto& lookup = tag_components.find(attribute_name_sv);

    if( lookup != tag_components.cend() )
        callback_function(lookup->second);
}


std::string CSDocCompilerWorker::PreprocessTextForDefinitionsAndIncludes(const std::string_view text_sv)
{
    std::optional<size_t> next_tag_offset = 0;

    while( next_tag_offset.has_value() )
    {
        const size_t start_tag_position = text_sv.find('<', *next_tag_offset);
        next_tag_offset.reset();

        if( start_tag_position == std::string_view::npos )
            return std::string(text_sv);

        // see if this is an actual preprocessor tag
        const std::string_view start_tag_sv = SO::TrimLeft(text_sv.substr(start_tag_position + 1));

        if( !SO::StartsWith(start_tag_sv, DefinitionTag_sv) &&
            !SO::StartsWith(start_tag_sv, IncludeTag_sv) )
        {
            // continue preprocessing text following the beginning of the start tag
            next_tag_offset = start_tag_position + 1;
            continue;
        }

        const TagPosition tag_position = GetTagPosition(text_sv, start_tag_position);

        if( tag_position.end == std::string_view::npos )
            return std::string(text_sv);

        const std::vector<std::string> tag_components = GetTagComponents(text_sv, tag_position);

        std::optional<std::string> preprocessed_text;

        if( tag_components.size() == 3 && tag_components.back() == "/" )
        {
            const std::string& tag_name = tag_components.front();
            const std::string& tag_value = tag_components[1];

            try
            {
                if( tag_name == DefinitionTag_sv )
                {
                    constexpr std::string_view SpecialDefinitionIndicator_sv = "::";
                    const size_t double_colon_pos = tag_value.find(SpecialDefinitionIndicator_sv);

                    if( double_colon_pos != std::string::npos )
                    {
                        preprocessed_text = m_settings.GetSpecialDefinition(tag_value.substr(0, double_colon_pos),
                                                                            tag_value.substr(double_colon_pos + SpecialDefinitionIndicator_sv.length()));
                    }

                    else
                    {
                        preprocessed_text = m_settings.GetDefinition(tag_value);
                    }
                }

                else if( tag_name == IncludeTag_sv )
                {
                    const std::string include_path = m_settings.EvaluatePath(tag_value);
                    preprocessed_text = PreprocessTextForDefinitionsAndIncludes(FileIO::ReadText(include_path));
                }
            }

            catch(...)
            {
                if( !m_settings.SuppressPreprocessorExceptions() )
                    throw;
            }
        }

        // if nothing was preprocessed, continue preprocessing text following the end of the start tag
        if( !preprocessed_text.has_value() )
        {
            next_tag_offset = tag_position.end + 1;
            continue;
        }

        // otherwise include the text prior to the start tag, the preprocessed text, and then preprocess all text following the end tag
        return SO::Concatenate(text_sv.substr(0, tag_position.start),
                               *preprocessed_text +
                               PreprocessTextForDefinitionsAndIncludes(text_sv.substr(tag_position.end + 1)));
    }

    return ReturnProgrammingError(std::string());
}


void CSDocCompilerWorker::CreateParagraphsFromPreprocessedText(const std::string& preprocessed_text)
{
    ASSERT(m_paragraphs.empty());

    std::string paragraph;
    std::optional<std::string> in_block_end_tag;

    auto end_paragraph = [&]()
    {
        if( !paragraph.empty() )
        {
            m_paragraphs.emplace_back(paragraph);
            paragraph.clear();
        }
    };

    SO::ForeachLine<std::string>(preprocessed_text, true,
        [&](std::string line)
        {
            SO::MakeTrimRight(line);

            // if in a block, append the line to the current paragraph along with a newline
            if( in_block_end_tag.has_value() )
            {
                paragraph.push_back('\n');
                paragraph.append(line);
            }

            // if not in a block, blank lines will end the current paragraph
            else if( line.empty() )
            {
                end_paragraph();
            }

            // if not in a block, append non-empty lines to the current paragraph along with a new line marker...
            else if( !line.empty() )
            {
                SO::AppendWithSeparator(paragraph, SO::TrimLeft(line), NewLineMarker_sv);

                // ...and check if there is a new block tag specifier
                std::smatch matches;

                if( std::regex_match(line, matches, m_sd.block_tag_match_regex) )
                {
                    ASSERT(matches.size() == 2);
                    in_block_end_tag = "</" + matches.str(1) + ">";
                }
            }

            // if in a block, check if it has ended
            if( in_block_end_tag.has_value() && line.find(*in_block_end_tag) != std::string_view::npos )
                in_block_end_tag.reset();
        });

    // end the last paragraph
    end_paragraph();

    if( in_block_end_tag.has_value() )
        throw CSProException("The block ending tag '%s' was not found.", in_block_end_tag->c_str());
}


void CSDocCompilerWorker::ProcessParagraph(std::string paragraph)
{
    ASSERT(m_tagStack.empty());
    ASSERT(!m_inBlockTag);
    ASSERT(m_endTagTextStack.empty());
    ASSERT(m_tableStack.empty());

    std::string text;

    try
    {
        while( !paragraph.empty()  )
            text.append(ProcessText(paragraph));

        if( !m_tagStack.empty() )
            throw CSProException("Missing end tag '%s' at the end of the paragraph.", m_tagStack.top().c_str());
    }

    catch( const CSProException& exception )
    {
        throw CSProException("Error '%s' processing:\n\n%s", exception.what(), paragraph.c_str());
    }

    if( text.empty() )
        return;

    // some tags, like note, don't result in any output, but because of how CreateParagraphsFromPreprocessedText adds
    // ~!~ newline markers into the paragraph text, a paragraph could contain only newline markers and no actual output;
    // these should not be processed
    if( text.length() % NewLineMarker_sv.length() == 0 )
    {
        for( std::string_view text_check_sv = text; SO::StartsWith(text_check_sv, NewLineMarker_sv); )
        {
            text_check_sv.remove_prefix(NewLineMarker_sv.length());

            if( text_check_sv.empty() )
                return;
        }
    }

    // replace newlines with breaks and wrap the paragraph's HTML in a div
    m_html.append(HT::ParagraphDiv_sv[0]);
    m_html.append(ReplaceNewlinesWithBreaks(text));
    m_html.append(HT::ParagraphDiv_sv[1]);
}


std::string& CSDocCompilerWorker::ReplaceNewlinesWithBreaks(std::string& text)
{
    static_assert(std::string::npos + 1 == 0);
    size_t newline_pos = std::string::npos;

    // if a newline immediately follows or preceeds a tag, it won't be considered a break
    while( ( ( newline_pos + 1 ) < text.size() ) &&
           ( ( newline_pos = text.find(NewLineMarker_sv, newline_pos + 1) ) != std::string::npos ) )
    {
        if( ( newline_pos > 0 && text[newline_pos - 1] == '>' ) ||
            ( ( newline_pos + NewLineMarker_sv.length() ) < text.length() && text[newline_pos + NewLineMarker_sv.length()] == '<' ) )
        {
            text.erase(newline_pos, NewLineMarker_sv.length());
        }
    }

    return SO::Replace(text, NewLineMarker_sv, "<br>\n");
}


std::string CSDocCompilerWorker::ProcessText(std::string& text)
{
    std::optional<size_t> next_tag_offset = 0;

    while( next_tag_offset.has_value() )
    {
        const TagPosition tag_position = GetTagPosition(text, *next_tag_offset);
        next_tag_offset.reset();

        // return if there are no more tags
        if( tag_position.start == std::string_view::npos )
            return std::exchange(text, std::string());

        if( tag_position.end == std::string_view::npos )
            throw CSProException("Invalid tag construction around: %s", text.substr(tag_position.start).c_str());

        const std::vector<std::string> tag_components = GetTagComponents(text, tag_position);

        if( tag_components.empty() && !m_inBlockTag )
            throw CSProException("Empty tag around: %s", text.substr(tag_position.start).c_str());

        std::string before_tag_text = text.substr(0, tag_position.start);

        // process end tags...
        if( !tag_components.empty() && SO::StartsWith(tag_components.front(), "/")  )
        {
            if( m_tagStack.empty() )
                throw CSProException("End tag without a start tag around: %s", text.substr(tag_position.start).c_str());

            const std::string_view end_tag_sv = std::string_view(tag_components.front()).substr(1);

            // if in a block, ignore end tags unless they are ending the block tag
            if( m_inBlockTag && end_tag_sv != m_tagStack.top() )
            {
                next_tag_offset = tag_position.end + 1;
                continue;
            }

            if( end_tag_sv != m_tagStack.top() )
            {
                throw CSProException("End tag </%s> does not match the last start tag <%s> around: %s",
                                     std::string(end_tag_sv).c_str(), m_tagStack.top().c_str(), text.substr(tag_position.start).c_str());
            }

            m_tagStack.pop();
            m_inBlockTag = false;

            text.erase(0, tag_position.end + 1);

            return before_tag_text;
        }


        // process start tags...

        // tags are ignored while in a block
        if( m_inBlockTag )
        {
            next_tag_offset = tag_position.start + 1;
            continue;
        }

        ASSERT(!tag_components.empty());

        const std::string& start_tag = tag_components.front();
        const auto& tag_definition_lookup = m_sd.tag_definitions.find(start_tag);

        if( tag_definition_lookup == m_sd.tag_definitions.cend() )
            throw CSProException("Invalid tag around: %s", text.substr(tag_position.start).c_str());

        // see if this is a block tag
        if( m_sd.block_tags.find(start_tag) != m_sd.block_tags.cend() )
            m_inBlockTag = true;

        const TagDefinition& tag_definition = tag_definition_lookup->second;

        // check that the tag ends correctly
        if( !tag_definition.paired )
        {
            if( tag_components.size() == 1 || tag_components.back() != "/" )
            {
                throw CSProException("The tag '%s' is not paired and must end with / around: %s",
                                     start_tag.c_str(), text.substr(tag_position.start).c_str());
            }
        }

        // get only the real tag components
        const std::string* const first_real_tag_component = tag_components.data() + 1;
        const std::string* const last_real_tag_component = first_real_tag_component + tag_components.size() - 1 - ( tag_definition.paired ? 0 : 1 );

        const cs::span<const std::string> real_tag_components(first_real_tag_component, last_real_tag_component - first_real_tag_component);

        // a routine to confirm that the number of tag components is valid
        auto check_num_tag_components = [&](const size_t num_tag_components)
        {
            if( num_tag_components < tag_definition.min_components )
            {
                throw CSProException("The tag '%s' must have at least %d argument%s around: %s",
                                     start_tag.c_str(), static_cast<int>(tag_definition.min_components),
                                     PluralizeWord(tag_definition.min_components), text.substr(tag_position.start).c_str());
            }

            if( num_tag_components > tag_definition.max_components )
            {
                throw CSProException("The tag '%s' must have at most %d argument%s around: %s",
                                     start_tag.c_str(), static_cast<int>(tag_definition.max_components),
                                     PluralizeWord(tag_definition.max_components), text.substr(tag_position.start).c_str());
            }
        };

        // process the start tag (using version 0 or 8 processing)
        std::string output;

        if( std::holds_alternative<TagDefinition::StartHandlerFunctionV8>(tag_definition.start_handler) )
        {
            const std::map<std::string_view, std::string_view> mapped_tag_components = ProcessTagComponentsV8(real_tag_components);
            check_num_tag_components(mapped_tag_components.size());

            output = (this->*std::get<TagDefinition::StartHandlerFunctionV8>(tag_definition.start_handler))(start_tag, mapped_tag_components);
        }

        else
        {
            check_num_tag_components(real_tag_components.size());
            output = StartTagV0(tag_definition, real_tag_components);
        }

        std::string after_tag_text = text.substr(tag_position.end + 1);

        if( tag_definition.paired )
        {
            m_tagStack.push(start_tag);

            const std::string inner_text = ProcessText(after_tag_text);
            output.append(EndTag(tag_definition, inner_text));
        }

        output = before_tag_text + output + ProcessText(after_tag_text);

        text = after_tag_text;

        return output;
    }

    return ReturnProgrammingError(std::string());
}


std::string CSDocCompilerWorker::StartTagV0(const TagDefinition& tag_definition, const cs::span<const std::string> tag_components)
{
    if( std::holds_alternative<std::monostate>(tag_definition.start_handler) )
    {
        return std::string();
    }

    else if( std::holds_alternative<std::string>(tag_definition.start_handler) )
    {
        return std::get<std::string>(tag_definition.start_handler);
    }

    else
    {
        ASSERT(std::holds_alternative<TagDefinition::StartHandlerFunctionV0>(tag_definition.start_handler));
        return (this->*std::get<TagDefinition::StartHandlerFunctionV0>(tag_definition.start_handler))(tag_components);
    }
}


std::string CSDocCompilerWorker::EndTag(const TagDefinition& tag_definition, const std::string& inner_text)
{
    if( std::holds_alternative<std::monostate>(tag_definition.end_handler) )
    {
        return inner_text;
    }

    else if( std::holds_alternative<std::string>(tag_definition.end_handler) )
    {
        return inner_text + std::get<std::string>(tag_definition.end_handler);
    }

    else
    {
        ASSERT(std::holds_alternative<TagDefinition::EndHandlerFunction>(tag_definition.end_handler));
        return (this->*std::get<TagDefinition::EndHandlerFunction>(tag_definition.end_handler))(inner_text);
    }
}


std::string CSDocCompilerWorker::ParseStringLiteral(const std::string_view text_sv, size_t& pos)
{
    const char quotemark = text_sv[pos];
    ASSERT(quotemark == '"' || quotemark == '\'');

    std::string string_literal;
    bool last_character_was_an_escape = false;

    for( ++pos; pos < text_sv.length(); ++pos )
    {
        const char ch = text_sv[pos];

        if( last_character_was_an_escape )
        {
            const char escaped_representation = Encoders::GetEscapedRepresentation(ch);

            // invalid escape sequence
            if( escaped_representation == 0 )
                throw CSProException("Invalid escape sequence, \\%c, in the string literal.", ch);

            string_literal.push_back(escaped_representation);
            last_character_was_an_escape = false;
        }

        else if( ch == quotemark )
        {
            return string_literal;
        }

        else if( ch == '\\' )
        {
            last_character_was_an_escape = true;
        }

        else
        {
            string_literal.push_back(ch);
        }
    }

    throw CSProException("Missing end quote (%c) in the string literal.", quotemark);
}


PortableColor CSDocCompilerWorker::ParsePortableColor(const std::string_view tag_or_attribute_name_sv, const bool first_argument_is_tag, const std::string& text)
{
    std::optional<PortableColor> color = PortableColor::FromString(text);

    if( !color.has_value() )
    {
        throw CSProException("The '%s' %s contains an invalid color value: %s",
                             std::string(tag_or_attribute_name_sv).c_str(),
                             first_argument_is_tag ? "tag" : "attribute",
                             text.c_str());
    }

    return std::move(*color);
}


int CSDocCompilerWorker::ParseInt(const std::string_view attribute_name_sv, const std::string_view text_sv)
{
    if( !CIMSAString::IsInteger(text_sv) )
    {
        throw CSProException("The '%s' attribute contains an invalid integer: %s",
                             std::string(attribute_name_sv).c_str(),
                             std::string(text_sv).c_str());
    }

    return static_cast<int>(CIMSAString::Val(text_sv));
}


int CSDocCompilerWorker::ParseInt(const std::string_view attribute_name_sv, const std::string_view text_sv, const int min_value)
{
    const int value = ParseInt(attribute_name_sv, text_sv);

    if( value < min_value )
    {
        throw CSProException("The '%s' attribute must contain an integer greater than or equal to %d, not: %s",
                             std::string(attribute_name_sv).c_str(),
                             min_value,
                             std::string(text_sv).c_str());
    }

    return value;
}


bool CSDocCompilerWorker::TryParseInt(const std::string_view text_sv, int& value)
{
    if( CIMSAString::IsInteger(text_sv) )
    {
        value = static_cast<int>(CIMSAString::Val(text_sv));
        return true;
    }

    return false;
}


bool CSDocCompilerWorker::TryParseDouble(const std::string_view text_sv, double& value)
{
    if( CIMSAString::IsNumeric(text_sv) )
    {
        value = CIMSAString::fVal(text_sv);
        return true;
    }

    return false;
}


void CSDocCompilerWorker::AppendImageWidthHeight(std::string& html, const std::optional<int> width, const std::optional<int> height, const bool close_tag)
{
    if( width.has_value() )
        html.append(FormatText(" \" width=\"%d\"", *width));

    if( height.has_value() )
        html.append(FormatText("\" height=\"%d\"", *height));

    if( close_tag )
        html.append(" />");
}


std::string CSDocCompilerWorker::EndTagWithContentsOfTextStack(const std::string& inner_text)
{
    ASSERT(!m_endTagTextStack.empty());
    std::string result = inner_text + m_endTagTextStack.top();
    m_endTagTextStack.pop();
    return result;
}


std::string CSDocCompilerWorker::TitleStartHandler(const cs::span<const std::string> tag_components)
{
    if( m_title.has_value() )
        throw CSProException("Only one title can be defined.");

    if( tag_components.empty() )
    {
        m_title.emplace();
    }

    else if( tag_components.front() == NoHeaderAttribute_sv )
    {
        m_title.emplace(NoHeaderAttribute_sv);
    }

    else
    {
        throw CSProException("Unknown 'title' tag: %s", tag_components.front().c_str());
    }

    return std::string();
}


std::string CSDocCompilerWorker::ContextStartHandler(const cs::span<const std::string> tag_components)
{
    for( cs::cref_optional<std::string> tag_component : tag_components )
    {
        const bool use_if_exists = ( tag_component->front() == '!' );

        if( use_if_exists )
            tag_component = tag_component->substr(1);

        m_settings.GetContextId(*tag_component, use_if_exists);
    }

    return std::string();
}


std::string CSDocCompilerWorker::TitleEndHandler(const std::string& inner_text)
{
    return CreateTitleHtml(inner_text, Encoders::ToHtml(inner_text));
}


std::string CSDocCompilerWorker::CreateTitleHtml(std::string raw_title, const std::string& title_html)
{
    ASSERT(m_title->empty() || *m_title == NoHeaderAttribute_sv);

    std::string header;

    if( *m_title != NoHeaderAttribute_sv && m_settings.AddTitleToDocument() )
    {
        header = "<h2><span class=\"header_size header\">" + title_html + "</span></h2>";

        const std::string url = m_settings.CreateUrlForTitle(m_settings.GetCompilationFilePath());

        if( !url.empty() )
        {
            const std::string a_tag_start = CreateHyperlinkStart(url, true, false);
            header = a_tag_start + " style=\"text-decoration: none;\">" + header + "</a>";
        }
    }

    m_title = std::move(raw_title);

    // update the database of titles
    m_settings.SetTitleForCompilationFilePath(*m_title);

    return header;
}


std::string CSDocCompilerWorker::IndentStartHandler(const cs::span<const std::string> tag_components)
{
    int indents = 1;

    if( !tag_components.empty() && ( !TryParseInt(tag_components.front(), indents) || indents < 1 ) )
        throw CSProException("The 'indent' tag has an invalid attribute: %s", tag_components.front().c_str());

    std::string text;
    std::string end_text;

    while( indents-- != 0 )
    {
        text.append("<div class=\"indent\">");
        end_text.append("</div>");
    }

    m_endTagTextStack.push(end_text);

    return text;
}


std::string CSDocCompilerWorker::FontStartHandler(const cs::span<const std::string> tag_components)
{
    std::string classes;
    std::string styles;
    int types = 0;
    int sizes = 0;
    int colors = 0;

    for( const std::string& tag_component : tag_components )
    {
        double em;

        // check for the font type
        if( tag_component == FontMonospaceAttribute_sv )
        {
            classes.append("monospace ");
            ++types;
        }

        // check for the font size
        else if( tag_component == HeaderAttribute_sv || tag_component == SubheaderTag_sv )
        {
            classes.append(tag_component + "_size ");
            ++sizes;
        }

        else if( TryParseDouble(tag_component, em) )
        {
            styles.append("font-size: " + DoubleToString(em) + "em; ");
            ++sizes;
        }

        // check for font color
        else
        {
            const PortableColor color = ParsePortableColor("font", true, tag_component);
            styles.append("color: ")
                  .append(color.ToString())
                  .append("; ");
            ++colors;
        }

        if( types > 1 || sizes > 1 || colors > 1 )
            throw CSProException("The 'font' tag cannot have more than one type, size, or color attribute.");
    }

    std::string result = "<span";

    if( !classes.empty() )
    {
        result.append(" class=\"")
              .append(classes)
              .append("\"");
    }

    if( !styles.empty() )
    {
        result.append(" style=\"")
              .append(styles)
              .append("\"");
    }

    result.push_back('>');

    return result;
}


std::string CSDocCompilerWorker::ListStartHandler(const cs::span<const std::string> tag_components)
{
    bool unordered_list = true;

    if( !tag_components.empty() )
    {
        if( tag_components.front() != OrderedAttribute_sv )
            throw CSProException("The 'list' tag has an invalid attribute: %s", tag_components.front().c_str());

        unordered_list = false;
    }

    m_endTagTextStack.push(unordered_list ? "</ul>" : "</ol>");

    return unordered_list ? "<ul>" : "<ol>";
}


std::string CSDocCompilerWorker::ImageStartHandler(const cs::span<const std::string> tag_components)
{
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int>* dimension_specifying = nullptr;
    std::string image_path;
    bool nochm = false;

    for( const std::string& tag_component : tag_components )
    {
        if( dimension_specifying != nullptr )
        {
            if( !TryParseInt(tag_component, dimension_specifying->emplace()) || *dimension_specifying <= 0 )
                throw CSProException("The image width and height must be positive integers (not '%s').", tag_component.c_str());

            dimension_specifying = nullptr;
        }

        else if( tag_component == ImageWidthAttribute_sv && !width.has_value() )
        {
            dimension_specifying = &width;
        }

        else if( tag_component == ImageHeightAttribute_sv && !height.has_value() )
        {
            dimension_specifying = &height;
        }

        else if( tag_component == ImageNoChmAttribute_sv && !nochm )
        {
            nochm = true;
        }

        else if( image_path.empty() )
        {
            image_path = tag_component;
        }

        else
        {
            throw CSProException("The 'image' tag has an invalid or duplicated attribute: %s", tag_component.c_str());
        }
    }

    if( dimension_specifying != nullptr )
        throw CSProException("The image width or height were not specified.");

    if( nochm && m_settings.CompilingForCompiledHtmlHelp() )
        return std::string();

    std::string html = CreateImageStartHtml(image_path);

    // for accessibility, use the name of the image, replacing underscores with spaces
    std::string alt = Path::GetFilenameWithoutExtension(image_path);
    SO::Replace(alt, '_', ' ');
    html.append(Encoders::ToHtmlTagValue(alt)).append("\"");

    AppendImageWidthHeight(html, width, height, true);

    return html;
}


std::string CSDocCompilerWorker::CreateImageStartHtml(const std::string& image_path)
{
    if( image_path.empty() )
        throw CSProException("The image location must be specified.");

    const std::string& evaluated_image_path = m_settings.EvaluateImagePath(image_path);

    if( !PortableFunctions::FileIsRegular(evaluated_image_path) )
        throw CSProException("The image could not be located: %s", evaluated_image_path.c_str());

    return SO::Concatenate("<img src=\"",
                           Encoders::ToHtmlTagValue(m_settings.CreateUrlForImageFile(evaluated_image_path)),
                           "\" alt=\"");
}


std::string CSDocCompilerWorker::BarcodeStartHandler(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components)
{
    constexpr std::string_view ErrorCorrectionAttribute_sv = "errorCorrection";
    constexpr std::string_view ScaleAttribute_sv           = "scale";
    constexpr std::string_view QuietZoneAttribute_sv       = "quietZone";
    constexpr std::string_view DarkColorAttribute_sv       = "darkColor";
    constexpr std::string_view LightColorAttribute_sv      = "lightColor";

    ValidateTagComponentsV8(start_tag, tag_components,
                            { TextAttribute_sv },
                            { ErrorCorrectionAttribute_sv, ScaleAttribute_sv, QuietZoneAttribute_sv, DarkColorAttribute_sv, LightColorAttribute_sv, ImageWidthAttribute_sv, ImageHeightAttribute_sv },
                            { TextAttribute_sv });

    Multimedia::QRCode qr_code;
    const std::string text(tag_components.at(TextAttribute_sv));
    std::optional<int> width;
    std::optional<int> height;

    ExecuteWithTagValue(tag_components, ErrorCorrectionAttribute_sv,
                        [&](const std::string_view value_sv) { qr_code.SetErrorCorrectionLevel(value_sv); });

    ExecuteWithTagValue(tag_components, ScaleAttribute_sv,
                        [&](const std::string_view value_sv) { qr_code.SetScale(ParseInt(ScaleAttribute_sv, value_sv)); });

    ExecuteWithTagValue(tag_components, QuietZoneAttribute_sv,
                        [&](const std::string_view value_sv) { qr_code.SetQuietZone(ParseInt(QuietZoneAttribute_sv, value_sv)); });

    ExecuteWithTagValue(tag_components, DarkColorAttribute_sv,
                        [&](const std::string_view value_sv) { qr_code.SetDarkColor(ParsePortableColor(DarkColorAttribute_sv, false, std::string(value_sv))); });

    ExecuteWithTagValue(tag_components, LightColorAttribute_sv,
                        [&](const std::string_view value_sv) { qr_code.SetLightColor(ParsePortableColor(LightColorAttribute_sv, false, std::string(value_sv))); });

    ExecuteWithTagValue(tag_components, ImageWidthAttribute_sv,
                        [&](const std::string_view value_sv) { width = ParseInt(ImageWidthAttribute_sv, value_sv, 1); });

    ExecuteWithTagValue(tag_components, ImageHeightAttribute_sv,
                        [&](const std::string_view value_sv) { height = ParseInt(ImageHeightAttribute_sv, value_sv, 1); });

    qr_code.Create(text);

    const std::unique_ptr<Multimedia::Image> qr_code_bitmap = qr_code.GetImage();
    const std::unique_ptr<std::vector<std::byte>> qr_code_png = qr_code_bitmap->ToBuffer(ImageType::Png);

    if( qr_code_png == nullptr )
        throw CSProException("There was an error generating a QR code for text: %s", text.c_str());

    const std::string data_url = Encoders::ToDataUrl(*qr_code_png, MimeType::Type::ImagePng);
    ASSERT(data_url == Encoders::ToHtmlTagValue(data_url));

    std::string html = SO::Concatenate("<img src=\"", data_url,
                                       "\" title=\"", Encoders::ToHtmlTagValue(text), "\"");

    AppendImageWidthHeight(html, width, height, true);

    return html;
}


CSDocCompilerWorker::PathAndProject CSDocCompilerWorker::GetPathAndProjectForTopicComponent(CSDocCompilerSettings& settings, const std::string& topic_component)
{
    if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(topic_component), FileExtensions::CSDocument) )
        throw CSProException("The file cannot be linked as it is not a CSPro Document: %s", topic_component.c_str());

    PathAndProject path_and_project;

    constexpr std::string_view ProjectIndicator_sv = "::";
    const size_t double_colon_pos = topic_component.find(ProjectIndicator_sv);

    if( double_colon_pos != std::string::npos )
    {
        path_and_project.project = topic_component.substr(0, double_colon_pos);
        path_and_project.path = settings.EvaluateTopicPath(path_and_project.project,
                                                           topic_component.substr(double_colon_pos + ProjectIndicator_sv.length()));
    }

    else
    {
        path_and_project.path = settings.EvaluateTopicPath(topic_component);
    }

    if( !PortableFunctions::FileIsRegular(path_and_project.path) )
        throw CSProException("The document could not be located: %s", topic_component.c_str());

    return path_and_project;
}


std::string CSDocCompilerWorker::EvaluateAndCreateUrlForTopicComponent(CSDocCompilerSettings& settings, const std::string& topic_component)
{
    const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(settings, topic_component);
    return settings.CreateUrlForTopic(path_and_project.project, path_and_project.path);
}


std::string CSDocCompilerWorker::CreateHyperlinkStart(std::string_view CreateUrlForFile_result_sv,
                                                      const bool target_blank/* = false*/, const bool end_tag/* = true*/)
{
    constexpr char OnClickIndicator = '!';

    if( CreateUrlForFile_result_sv.empty() )
        return "<a>";

    std::string html = "<a href=\"";

    if( CreateUrlForFile_result_sv.front() == OnClickIndicator )
    {
        html.push_back('#');

        if( !CreateUrlForFile_result_sv.empty() )
        {
            html.append("\" onclick=\"");
            CreateUrlForFile_result_sv.remove_prefix(1);
        }
    }

    html.append(Encoders::ToHtmlTagValue(CreateUrlForFile_result_sv));

    if( target_blank )
        html.append("\" target=\"_blank");

    html.append(end_tag ? "\">" :
                          "\"");

    return html;
}


std::string CSDocCompilerWorker::TopicStartHandler(const cs::span<const std::string> tag_components)
{
    const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(tag_components.front());
    std::string title = m_settings.GetTitle(path_and_project.path);

    return SO::Concatenate(CreateHyperlinkStart(m_settings.CreateUrlForTopic(path_and_project.project, path_and_project.path)),
                           Encoders::ToHtml(std::move(title)),
                           "</a>");
}


std::string CSDocCompilerWorker::LinkStartHandler(const cs::span<const std::string> tag_components)
{
    return CreateLinkStartHtml(tag_components.front(), true);
}


std::string CSDocCompilerWorker::CreateLinkStartHtml(std::string url, const bool end_tag)
{
    bool target_blank = false;

    if( Encoders::IsDataOrHttpUrl(url) || SO::StartsWith(url, "mailto") )
    {
        target_blank = m_settings.OpenExternalLinksInSeparateWindow();
    }

    else
    {
        const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(url);
        url = m_settings.CreateUrlForTopic(path_and_project.project, path_and_project.path);
    }

    return CreateHyperlinkStart(url, target_blank, end_tag);
}


std::string CSDocCompilerWorker::SeeAlsoStartHandler(const cs::span<const std::string> tag_components)
{
    std::vector<std::tuple<std::string, std::string>> urls_and_titles;
    bool ordered_list = false;

    for( const std::string& tag_component : tag_components )
    {
        if( tag_component == OrderedAttribute_sv )
        {
            if( ordered_list || !urls_and_titles.empty() )
                throw CSProException("The 'starttag''ordered' attribute can only appear once and must appear first.");

            ordered_list = true;
        }

        else
        {
            const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(tag_component);
            urls_and_titles.emplace_back(m_settings.CreateUrlForTopic(path_and_project.project, path_and_project.path),
                                         m_settings.GetTitle(path_and_project.path));
        }
    }

    if( urls_and_titles.empty() )
        throw CSProException("You must provide at least one topic.");

    // order by title
    if( ordered_list )
    {
        std::sort(urls_and_titles.begin(), urls_and_titles.end(),
                 [&](const auto& pt1, const auto& pt2) { return ( SO::CompareNoCase(std::get<1>(pt1), std::get<1>(pt2)) < 0 ); });
    }

    std::string html;

    for( const auto& [url, title] : urls_and_titles )
    {
        html.append(html.empty() ? "<b>See also</b>: " : ", ")
            .append(CreateHyperlinkStart(url))
            .append(Encoders::ToHtml(title))
            .append("</a>");
    }

    return html;
}


std::string CSDocCompilerWorker::ResourceStartHandler(const cs::span<const std::string> tag_components)
{
    return CreateHyperlinkStart(m_settings.CreateUrlForResource(tag_components.front()),
                                m_settings.OpenExternalLinksInSeparateWindow());
}


struct CSDocCompilerWorker::TableSettings
{
    int columns = 0;
    bool header = false;
    bool nowrap = false;
    bool center = false;
    bool border = false;
    int cells = 0;
};


CSDocCompilerWorker::TableSettings& CSDocCompilerWorker::GetCurrentTable()
{
    if( m_tableStack.empty() )
        throw CSProException("You cannot have a table cell without being in a table.");

    return *m_tableStack.top();
}


std::string CSDocCompilerWorker::TableStartHandler(const cs::span<const std::string> tag_components)
{
    auto table_settings = std::make_shared<TableSettings>();

    for( const std::string& tag_component : tag_components )
    {
        if( !table_settings->header && tag_component == HeaderAttribute_sv )
        {
            table_settings->header = true;
        }

        else if( !table_settings->nowrap && tag_component == NoWrapAttribute_sv )
        {
            table_settings->nowrap = true;
        }

        else if( !table_settings->center && tag_component == CenterTag_sv )
        {
            table_settings->center = true;
        }

        else if( !table_settings->border && tag_component == BorderAttribute_sv )
        {
            table_settings->border = true;
        }

        else if( table_settings->columns == 0 && TryParseInt(tag_component, table_settings->columns) )
        {
            if( table_settings->columns < 1 )
                throw CSProException("The number of columns in a table must be a positive integer");
        }

        else
        {
            throw CSProException("The 'table' tag has an invalid attribute: %s", tag_component.c_str());
        }
    }

    if( table_settings->columns == 0 )
        throw CSProException("The number of columns in a table must be specified.");

    m_tableStack.push(table_settings);

    return table_settings->border ? "<table class=\"bordered_table\">" :
                                    "<table>";
}


std::string CSDocCompilerWorker::TableEndHandler(const std::string& inner_text)
{
    const TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;

    if( cell_index != 0 )
        throw CSProException("You cannot end the table without specifying an additional '%d' cells.", table_settings.columns - cell_index);

    m_tableStack.pop();

    return inner_text + "</table>";
}


std::string CSDocCompilerWorker::TableCellStartHandler(const cs::span<const std::string> tag_components)
{
    TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;
    const bool is_header_row = ( table_settings.header && table_settings.cells < table_settings.columns );
    const bool is_first_cell_in_row = ( cell_index == 0 );
    bool nowrap = ( table_settings.nowrap && is_first_cell_in_row );
    int columns = 1;

    for( const std::string& tag_component : tag_components )
    {
        if( tag_component == NoWrapAttribute_sv )
        {
            nowrap = true;
        }

        else if( !TryParseInt(tag_component, columns) || columns < 1 )
        {
            throw CSProException("The number of columns in a table must be a positive integer.");
        }
    }

    if( ( cell_index + columns ) > table_settings.columns )
        throw CSProException("The number of columns including a span cannot exceed the number of columns.");

    table_settings.cells += columns;

    const std::string row_prefix = is_first_cell_in_row ? "<tr>" : std::string();
    const std::string cell_type = is_header_row ? "th" : "td";
    const std::string span = ( columns > 1 ) ? FormatText(" colspan=\"%d\"", columns) : std::string();

    std::string style;

    if( nowrap )
        style.append("white-space: nowrap; ");

    style.append("text-align: ")
         .append(table_settings.center ? "center;" : "left;");

    const std::string class_str = table_settings.border ? " class=\"bordered_table_cell\"" : std::string();

    if( !style.empty() )
    {
        SO::MakeTrim(style);
        style = SO::Concatenate(" style=\"", style, "\"");
    }

    return SO::Concatenate(row_prefix, "<", cell_type, style, span, class_str, ">");
}


std::string CSDocCompilerWorker::TableCellEndHandler(const std::string& inner_text)
{
    const TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;
    const bool is_header_row = ( table_settings.header && table_settings.cells <= table_settings.columns );
    const bool is_last_cell_in_row = ( cell_index == 0 );

    return inner_text + ( is_header_row       ? "</th>"   : "</td>" ) +
                        ( is_last_cell_in_row ? "</tr>\n" : "" );
}


std::string CSDocCompilerWorker::NoteStartHandler(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components)
{
    constexpr std::string_view TypeAttribute_sv    = "type";
    constexpr std::string_view TypeErrorValue_sv   = "error";
    constexpr std::string_view TypeWarningValue_sv = "warning";
    constexpr std::string_view TypeTodoValue_sv    = "todo";
    constexpr std::string_view TypeCommentValue_sv = "comment";

    ValidateTagComponentsV8(start_tag, tag_components,
                            { TextAttribute_sv },
                            { TypeAttribute_sv });

    std::string text;
    ExecuteWithTagValue(tag_components, TextAttribute_sv,
                        [&](const std::string_view value_sv) { text = value_sv; });

    enum class NoteType { Error, Warning, Todo, Comment };
    NoteType note_type = NoteType::Comment;
    ExecuteWithTagValue(tag_components, TypeAttribute_sv,
        [&](const std::string_view value_sv)
        {
            note_type = ( value_sv == TypeErrorValue_sv )   ? NoteType::Error :
                        ( value_sv == TypeWarningValue_sv ) ? NoteType::Warning :
                        ( value_sv == TypeTodoValue_sv )    ? NoteType::Todo:
                        ( value_sv == TypeCommentValue_sv ) ? NoteType::Comment :
                                                              throw CSProException("Invalid note type: '%s'", std::string(value_sv).c_str());
        });

    switch( note_type )
    {
        case NoteType::Error:
            throw CSProException(text);

        case NoteType::Warning:
            m_settings.AddCompilerMessage(CompilerMessageType::Warning, text);
            break;

        case NoteType::Todo:
            m_settings.AddCompilerMessage(CompilerMessageType::Info, "TODO: " + text);
            break;

        default:
            ASSERT(note_type == NoteType::Comment);
            break;
    }

    return std::string();
}


std::string CSDocCompilerWorker::MetadataStartHandler(const std::string& /*start_tag*/, const std::map<std::string_view, std::string_view>& tag_components)
{
    for( const auto& [attribute_sv, value_sv] : tag_components )
        m_settings.AddMetadata(attribute_sv, value_sv);

    return std::string();
}


std::string CSDocCompilerWorker::BuildExtraStartHandler(const cs::span<const std::string> tag_components)
{
    for( const std::string& tag_component : tag_components )
        m_settings.EvaluateBuildExtra(tag_component);

    return std::string();
}
