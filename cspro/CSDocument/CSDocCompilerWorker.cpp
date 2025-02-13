#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include <zUtilO/PortableColor.h>
#include <zMultimediaO/Image.h>
#include <zMultimediaO/QRCode.h>


namespace
{
    constexpr wstring_view TitleTag_sv                  = _T("title");
    constexpr wstring_view ContextTag_sv                = _T("context");
    constexpr wstring_view IndentTag_sv                 = _T("indent");
    constexpr wstring_view CenterTag_sv                 = _T("center");
    constexpr wstring_view BoldTag_sv                   = _T("b");
    constexpr wstring_view ItalicsTag_sv                = _T("i");
    constexpr wstring_view SuperscriptTag_sv            = _T("sup");
    constexpr wstring_view FontTag_sv                   = _T("font");
    constexpr wstring_view ListTag_sv                   = _T("list");
    constexpr wstring_view ListItemTag_sv               = _T("li");
    constexpr wstring_view SubheaderTag_sv              = _T("subheader");
    constexpr wstring_view ImageTag_sv                  = _T("image");
    constexpr wstring_view BarcodeTag_sv                = _T("barcode");
    constexpr wstring_view TopicTag_sv                  = _T("topic");
    constexpr wstring_view LinkTag_sv                   = _T("link");
    constexpr wstring_view SeeAlsoTag_sv                = _T("seealso");
    constexpr wstring_view TableTag_sv                  = _T("table");
    constexpr wstring_view TableCellTag_sv              = _T("cell");
    constexpr wstring_view LogicTag_sv                  = _T("logic");
    constexpr wstring_view LogicSyntaxTag_sv            = _T("logicsyntax");
    constexpr wstring_view LogicColorTag_sv             = _T("logiccolor");
    constexpr wstring_view LogicArgumentTag_sv          = _T("arg");
    constexpr wstring_view LogicTableTag_sv             = _T("logictable");
    constexpr wstring_view ActionTag_sv                 = _T("action");
    constexpr wstring_view MessageTag_sv                = _T("message");
    constexpr wstring_view ReportTag_sv                 = _T("report");
    constexpr wstring_view ColorTag_sv                  = _T("color");
    constexpr wstring_view ColorInlineTag_sv            = _T("colorinline");
    constexpr wstring_view PffTag_sv                    = _T("pff");
    constexpr wstring_view PffColorTag_sv               = _T("pffcolor");
    constexpr wstring_view HtmlTag_sv                   = _T("html");
    constexpr wstring_view NoteTag_sv                   = _T("note");
    constexpr wstring_view DefinitionTag_sv             = _T("definition");
    constexpr wstring_view IncludeTag_sv                = _T("include");
    constexpr wstring_view CalloutTag_sv                = _T("callout");
    constexpr wstring_view PageBreakTag_sv              = _T("pagebreak");
    constexpr wstring_view BuildExtraTag_sv             = _T("build-extra");

    constexpr wstring_view TextAttribute_sv             = _T("text");
    constexpr wstring_view HeaderAttribute_sv           = _T("header");
    constexpr wstring_view NoHeaderAttribute_sv         = _T("noheader");
    constexpr wstring_view FontMonospaceAttribute_sv    = _T("monospace");
    constexpr wstring_view ImageNoChmAttribute_sv       = _T("nochm");
    constexpr wstring_view ImageWidthAttribute_sv       = _T("width");
    constexpr wstring_view ImageHeightAttribute_sv      = _T("height");
    constexpr wstring_view OrderedAttribute_sv          = _T("ordered");
    constexpr wstring_view NoWrapAttribute_sv           = _T("nowrap");
    constexpr wstring_view BorderAttribute_sv           = _T("border");

    constexpr wstring_view NewLineMarker_sv             = _T("~!~");
}


const CSDocCompilerWorker::SD& CSDocCompilerWorker::GetStaticData()
{
    static const SD sd = []()
    {
        // set up the tag definitions
        std::map<std::wstring, TagDefinition> tag_definitions =
        {
            { TitleTag_sv,         TagDefinition { true,   &TitleStartHandler, &TitleEndHandler, 0, 1 } },
            { ContextTag_sv,       TagDefinition { false,  &ContextStartHandler, { }, 1, SIZE_MAX } },
            { IndentTag_sv,        TagDefinition { true,   &IndentStartHandler, &EndTagWithContentsOfTextStack, 0, 1 } },
            { CenterTag_sv,        TagDefinition { true,   _T("<div align=\"center\">"), _T("</div>") } },
            { BoldTag_sv,          TagDefinition { true,   _T("<b>"), _T("</b>") } },
            { ItalicsTag_sv,       TagDefinition { true,   _T("<i>"), _T("</i>") } },
            { SuperscriptTag_sv,   TagDefinition { true,   _T("<sup>"), _T("</sup>") } },
            { FontTag_sv,          TagDefinition { true,   &FontStartHandler, _T("</span>"), 1, 3 } },
            { ListTag_sv,          TagDefinition { true,   &ListStartHandler, &EndTagWithContentsOfTextStack, 0, 1 } },
            { ListItemTag_sv,      TagDefinition { true,   _T("<li>"), _T("</li>") } },
            { SubheaderTag_sv,     TagDefinition { true,   _T("<div class=\"subheader_size subheader\">"), _T("</div>") } },
            { ImageTag_sv,         TagDefinition { false,  &ImageStartHandler, { }, 1, 6 } },
            { BarcodeTag_sv,       TagDefinition { false,  &BarcodeStartHandler, { }, 1, 8 } },
            { TopicTag_sv,         TagDefinition { false,  &TopicStartHandler, { }, 1, 1 } },
            { LinkTag_sv,          TagDefinition { true,   &LinkStartHandler, _T("</a>"), 1, 1 } },
            { SeeAlsoTag_sv,       TagDefinition { false,  &SeeAlsoStartHandler, { }, 1, SIZE_MAX } },
            { TableTag_sv,         TagDefinition { true,   &TableStartHandler, &TableEndHandler, 1, 4 } },
            { TableCellTag_sv,     TagDefinition { true,   &TableCellStartHandler, &TableCellEndHandler, 0, 2 } },
            { LogicTag_sv,         TagDefinition { true,   { }, &LogicEndHandler } },
            { LogicSyntaxTag_sv,   TagDefinition { true,   &LogicObjectStartHandler, &LogicSyntaxEndHandler, 0, 1 } },
            { LogicColorTag_sv,    TagDefinition { true,   &LogicObjectStartHandler, &LogicColorEndHandler, 0, 1 } },
            { LogicArgumentTag_sv, TagDefinition { true,   _T("<span class=\"code_colorization_argument\">"), _T("</span>") } },
            { LogicTableTag_sv,    TagDefinition { false,  &LogicTableStartHandler, { }, 1, 1 } },
            { ActionTag_sv,        TagDefinition { true,   { }, &ActionEndHandler } },
            { MessageTag_sv,       TagDefinition { true,   { }, &MessageEndHandler } },
            { ReportTag_sv,        TagDefinition { true,   &ReportStartHandler, &ReportEndHandler, 0, 1 } },
            { ColorTag_sv,         TagDefinition { true,   &ColorStartHandler, &ColorEndHandler, 1, 1 } },
            { ColorInlineTag_sv,   TagDefinition { true,   &ColorStartHandler, &ColorInlineEndHandler, 1, 1 } },
            { PffTag_sv,           TagDefinition { true,   { }, &PffEndHandler } },
            { PffColorTag_sv,      TagDefinition { true,   { }, &PffColorEndHandler} },
            { HtmlTag_sv,          TagDefinition { true,   { }, { } } },
            { NoteTag_sv,          TagDefinition { false,  &NoteStartHandler, { }, 1, 2 } },
            { CalloutTag_sv,       TagDefinition { true,   _T("<div style=\"background-color: lightgrey;border:1px solid black;margin:10px;padding:10px\">"), _T("</div>") } },
            { PageBreakTag_sv,     TagDefinition { false,  _T("<div class=\"new-page\" />") } },
            { BuildExtraTag_sv,    TagDefinition { false,  &BuildExtraStartHandler, { }, 1, SIZE_MAX } },
        };


        // set up the block tags
        std::set<std::wstring> block_tags;
        std::wstring block_tag_match_regex_text;

        for( std::wstring tag : { LogicTag_sv,
                                  LogicSyntaxTag_sv,
                                  MessageTag_sv,
                                  ReportTag_sv,
                                  ColorTag_sv,
                                  PffTag_sv,
                                  HtmlTag_sv } )
        {
            block_tag_match_regex_text.append(block_tag_match_regex_text.empty() ? _T(".*<(") : _T("|"));
            block_tag_match_regex_text.append(Encoders::ToRegex(tag));

            block_tags.insert(std::move(tag));
        }

        block_tag_match_regex_text.append(_T(")(?:\\s.*|)>.*"));

        return SD
        {
            std::move(tag_definitions),
            std::move(block_tags),
            std::wregex(block_tag_match_regex_text)
        };
    }();

    return sd;
}


CSDocCompilerWorker::CSDocCompilerWorker(CSDocCompilerSettings& settings, const wstring_view text_sv)
    :   m_sd(GetStaticData()),
        m_settings(settings),
        m_inBlockTag(false)
{
    const std::wstring preprocessed_text = PreprocessTextForDefinitionsAndIncludes(text_sv);

    CreateParagraphsFromPreprocessedText(preprocessed_text);
}


std::wstring CSDocCompilerWorker::CreateHtml()
{
    ASSERT(m_html.empty());

    std::optional<size_t> title_pos;

    if( m_settings.AddHtmlHeader() )
    {
        m_html.append(DEFAULT_HTML_HEADER);

        m_html.append(_T("<title>"));
        title_pos = m_html.size();
        m_html.append(_T("</title>\n"));

        m_html.append(m_settings.GetStylesheetsHtml());

        m_html.append(_T("</head>\n<body>\n"));
    }

    const auto [start_document_html, end_document_html] = m_settings.GetHtmlToWrapDocument();
    m_html.append(start_document_html);

    for( const std::wstring& paragraph : m_paragraphs )
        ProcessParagraph(paragraph);

    // by now the title has been processed
    if( !m_title.has_value() )
    {
        m_settings.ClearTitleForCompilationFilename();

        if( m_settings.TitleIsRequired() )
            throw CSProException("A title must be specified for the document.");
    }

    if( title_pos.has_value() && !m_settings.GetCompilationFilename().empty() )
    {
        // only insert the title when defined, suppressing errors getting a title if the title is not required
        try
        {
            m_html.insert(*title_pos, Encoders::ToHtml(m_settings.GetHtmlHeaderTitle(m_settings.GetCompilationFilename())));
        }

        catch(...)
        {
            if( m_settings.TitleIsRequired() )
                throw;
        }
    }

    m_html.append(end_document_html);

    if( m_settings.AddHtmlFooter() )
        m_html.append(_T("</body>\n</html>\n"));

    return m_html;
}


CSDocCompilerWorker::TagPosition CSDocCompilerWorker::GetTagPosition(const wstring_view text_sv, const size_t start_pos/* = 0*/)
{
    TagPosition tag_position { text_sv.find('<', start_pos), wstring_view::npos };

    while( true )
    {
        if( tag_position.start == wstring_view::npos )
            return tag_position;

        // tags must be followed by a letter or by the end tag character
        const size_t next_pos = tag_position.start + 1;

        if( next_pos == text_sv.length() )
            return tag_position;

        const TCHAR next_ch = text_sv[next_pos];

        if( next_ch == '/' || std::isalpha(next_ch) )
            break;

        tag_position.start = text_sv.find('<', next_pos);
    }

    ASSERT(tag_position.start != wstring_view::npos);

    for( size_t pos = tag_position.start + 1; pos < text_sv.length(); ++pos )
    {
        const TCHAR ch = text_sv[pos];

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


std::vector<std::wstring> CSDocCompilerWorker::GetTagComponents(const wstring_view text_sv, const TagPosition& tag_position) const
{
    ASSERT(text_sv[tag_position.start] == '<' && text_sv[tag_position.end] == '>');

    const wstring_view tag_sv = text_sv.substr(tag_position.start + 1, tag_position.end - tag_position.start - 1);

    // see if this is a version 8 tag
    const auto& first_whitespace_pos = std::find_if(tag_sv.cbegin(), tag_sv.cend(),
                                                    [](TCHAR ch) { return std::iswspace(ch); });

    if( first_whitespace_pos != tag_sv.cend() )
    {
        const size_t first_whitespace_index = std::distance(tag_sv.cbegin(), first_whitespace_pos);
        const wstring_view tag_name_sv = tag_sv.substr(0, first_whitespace_index);
        const auto& tag_definition_lookup = m_sd.tag_definitions.find(tag_name_sv);

        if( tag_definition_lookup != m_sd.tag_definitions.cend() &&
            std::holds_alternative<TagDefinition::StartHandlerFunctionV8>(tag_definition_lookup->second.start_handler) )
        {
            return GetTagComponentsV8(tag_name_sv, tag_sv.substr(first_whitespace_index));
        }
    }

    return GetTagComponentsV0(tag_sv);
}


std::vector<std::wstring> CSDocCompilerWorker::GetTagComponentsV0(const wstring_view tag_sv)
{
    const size_t quotemark_pos = tag_sv.find_first_of(_T("\"'"));

    // when the tag components do not use quotes, we do not have to process the tag in any special fashion
    if( quotemark_pos == wstring_view::npos )
        return SO::SplitString(tag_sv, ' ', true, false);

    std::vector<std::wstring> tag_components;
    bool add_new_tag_component_with_next_ch = true;

    for( size_t pos = 0; pos < tag_sv.length(); ++pos )
    {
        const TCHAR ch = tag_sv[pos];

        if( ch == '"' || ch == '\'' )
        {
            tag_components.emplace_back(ParseStringLiteral(tag_sv, pos));
            add_new_tag_component_with_next_ch = true;
        }

        else if( std::iswspace(ch) )
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


std::vector<std::wstring> CSDocCompilerWorker::GetTagComponentsV8(const wstring_view tag_name_sv, const wstring_view rest_of_tag_sv)
{
    std::vector<std::wstring> tag_components = { tag_name_sv };

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
        const TCHAR ch = rest_of_tag_sv[pos];

        if( expected_entity == ExpectedEntity::NothingAsTagIsEnded )
        {
            throw CSProException("No additional text can appear after the tag closing character '/'.");
        }

        else if( std::iswspace(ch) )
        {
            if( expected_entity == ExpectedEntity::NameContinue )
                expected_entity = ExpectedEntity::Equals;
        }

        else if( ch == '/' )
        {
            ensure_attribute_has_value();
            tag_components.emplace_back(_T("/"));
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
                throw CSProException(_T("A tag attribute cannot contain the character '%c'."), ch);

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
        throw CSProException(_T("A tag value must be specified after the '=' following the tag attribute '%s'."), tag_components.back().c_str());
    }

    ASSERT(tag_components.size() % 2 == 1 || tag_components.back() == _T("/"));

    // make sure each attribute name is unique
    for( size_t i = 3; i < tag_components.size(); i += 2 )
    {
        const std::wstring& this_tag_component = tag_components[i];

        for( size_t j = 1; j < i; j += 2 )
        {
            if( SO::EqualsNoCase(this_tag_component, tag_components[j]) )
                throw CSProException(_T("More than one tag attribute with the name '%s' cannot be specified."), this_tag_component.c_str());
        }
    }

    return tag_components;
}


std::map<wstring_view, wstring_view> CSDocCompilerWorker::ProcessTagComponentsV8(const cs::span<const std::wstring> tag_components)
{
    std::map<wstring_view, wstring_view> mapped_tag_components;

    ASSERT(tag_components.size() % 2 == 0);
    const auto& tag_components_end = tag_components.end();

    for( auto tag_components_itr = tag_components.begin(); tag_components_itr != tag_components_end; tag_components_itr += 2 )
    {
        ASSERT(!tag_components_itr->empty());
        mapped_tag_components.try_emplace(*tag_components_itr, *( tag_components_itr + 1 ));
    }

    return mapped_tag_components;
}


void CSDocCompilerWorker::ValidateTagComponentsV8(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components,
                                                  const cs::span<const wstring_view> required_attribute_names,
                                                  const cs::span<const wstring_view> optional_attribute_names/* = cs::span<const wstring_view>()*/,
                                                  const cs::span<const wstring_view> attribute_names_where_value_can_be_blank/* = cs::span<const wstring_view>()*/)
{
    auto index_in_span = [](const wstring_view& name_sv, const cs::span<const wstring_view>& attribute_names)
    {
        size_t index = 0;

        for( const wstring_view& attribute_name_sv : attribute_names )
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
            throw CSProException(_T("The '%s' tag contains an unrecognized attribute '%s'."),
                                 start_tag.c_str(), std::wstring(name_sv).c_str());
        }

        if( SO::IsWhitespace(value_sv) && index_in_span(name_sv, attribute_names_where_value_can_be_blank) == SIZE_MAX )
        {
            throw CSProException(_T("The '%s' tag contains an attribute '%s' with a blank value. The value must be non-blank."),
                                 start_tag.c_str(), std::wstring(name_sv).c_str());
        }
    }

    const auto& missing_required_attribute_lookup = std::find(required_attributes_present.cbegin(), required_attributes_present.cend(), false);

    if( missing_required_attribute_lookup != required_attributes_present.cend() )
    {
        const size_t index = std::distance(required_attributes_present.cbegin(), missing_required_attribute_lookup);

        throw CSProException(_T("The '%s' tag requires the specification of the attribute '%s'."),
                                start_tag.c_str(), std::wstring(required_attribute_names[index]).c_str());
    }
}


template<typename CF>
void CSDocCompilerWorker::ExecuteWithTagValue(const std::map<wstring_view, wstring_view>& tag_components,
                                              const wstring_view attribute_name_sv, const CF callback_function)
{
    const auto& lookup = tag_components.find(attribute_name_sv);

    if( lookup != tag_components.cend() )
        callback_function(lookup->second);
}


std::wstring CSDocCompilerWorker::PreprocessTextForDefinitionsAndIncludes(const wstring_view text_sv)
{
    std::optional<size_t> next_tag_offset = 0;

    while( next_tag_offset.has_value() )
    {
        const size_t start_tag_position = text_sv.find('<', *next_tag_offset);
        next_tag_offset.reset();

        if( start_tag_position == wstring_view::npos )
            return text_sv;

        // see if this is an actual preprocessor tag
        const wstring_view start_tag_sv = SO::TrimLeft(text_sv.substr(start_tag_position + 1));

        if( !SO::StartsWith(start_tag_sv, DefinitionTag_sv) &&
            !SO::StartsWith(start_tag_sv, IncludeTag_sv) )
        {
            // continue preprocessing text following the beginning of the start tag
            next_tag_offset = start_tag_position + 1;
            continue;
        }

        const TagPosition tag_position = GetTagPosition(text_sv, start_tag_position);

        if( tag_position.end == wstring_view::npos )
            return text_sv;

        const std::vector<std::wstring> tag_components = GetTagComponents(text_sv, tag_position);

        std::optional<std::wstring> preprocessed_text;

        if( tag_components.size() == 3 && tag_components.back() == _T("/") )
        {
            const std::wstring& tag_name = tag_components.front();
            const std::wstring& tag_value = tag_components[1];

            try
            {
                if( tag_name == DefinitionTag_sv )
                {
                    constexpr std::wstring_view SpecialDefinitionIndicator = _T("::");
                    const size_t double_colon_pos = tag_value.find(SpecialDefinitionIndicator);

                    if( double_colon_pos != std::wstring::npos )
                    {
                        preprocessed_text = m_settings.GetSpecialDefinition(tag_value.substr(0, double_colon_pos),
                                                                            tag_value.substr(double_colon_pos + SpecialDefinitionIndicator.length()));
                    }

                    else
                    {
                        preprocessed_text = m_settings.GetDefinition(tag_value);
                    }                    
                }

                else if( tag_name == IncludeTag_sv )
                {
                    const std::wstring include_path = m_settings.EvaluatePath(tag_value);
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
        return std::wstring(text_sv.substr(0, tag_position.start)) +
                *preprocessed_text +
                PreprocessTextForDefinitionsAndIncludes(text_sv.substr(tag_position.end + 1));
    }

    return ReturnProgrammingError(std::wstring());
}


void CSDocCompilerWorker::CreateParagraphsFromPreprocessedText(const std::wstring& preprocessed_text)
{
    ASSERT(m_paragraphs.empty());

    std::wstring paragraph;
    std::optional<std::wstring> in_block_end_tag;

    auto end_paragraph = [&]()
    {
        if( !paragraph.empty() )
        {
            m_paragraphs.emplace_back(paragraph);
            paragraph.clear();
        }
    };

    SO::ForeachLine(preprocessed_text, true,
        [&](std::wstring line)
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
                std::wsmatch matches;

                if( std::regex_match(line, matches, m_sd.block_tag_match_regex) )
                {
                    ASSERT(matches.size() == 2);
                    in_block_end_tag = _T("</") + matches.str(1) + _T(">");
                }
            }

            // if in a block, check if it has ended
            if( in_block_end_tag.has_value() && line.find(*in_block_end_tag) != wstring_view::npos )
                in_block_end_tag.reset();

            return true;
        });

    // end the last paragraph
    end_paragraph();

    if( in_block_end_tag.has_value() )
        throw CSProException(_T("The block ending tag '%s' was not found."), in_block_end_tag->c_str());
}


void CSDocCompilerWorker::ProcessParagraph(std::wstring paragraph)
{
    ASSERT(m_tagStack.empty());
    ASSERT(!m_inBlockTag);
    ASSERT(m_endTagTextStack.empty());
    ASSERT(m_tableStack.empty());

    std::wstring text;

    try
    {
        while( !paragraph.empty()  )
            text.append(ProcessText(paragraph));

        if( !m_tagStack.empty() )
            throw CSProException(_T("Missing end tag '%s' at the end of the paragraph."), m_tagStack.top().c_str());
    }

    catch( const CSProException& exception )
    {
        throw CSProException(_T("Error '%s' processing:\n\n%s"), exception.GetErrorMessage().c_str(), paragraph.c_str());
    }

    if( text.empty() )
        return;

    // some tags, like note, don't result in any output, but because of how CreateParagraphsFromPreprocessedText adds
    // ~!~ newline markers into the paragraph text, a paragraph could contain only newline markers and no actual output;
    // these should not be processed
    if( text.length() % NewLineMarker_sv.length() == 0 )
    {
        for( wstring_view text_check_sv = text; SO::StartsWith(text_check_sv, NewLineMarker_sv); )
        {
            text_check_sv = text_check_sv.substr(NewLineMarker_sv.length());

            if( text_check_sv.empty() )
                return;
        }
    }        

    // replace newlines with breaks and wrap the paragraph's HTML in a div
    m_html.append(_T("<div class=\"paragraph\">"));
    m_html.append(ReplaceNewlinesWithBreaks(std::move(text)));
    m_html.append(_T("</div>\n"));
}


std::wstring CSDocCompilerWorker::ReplaceNewlinesWithBreaks(std::wstring text)
{
    static_assert(std::wstring::npos + 1 == 0);
    size_t newline_pos = std::wstring::npos;

    // if a newline immediately follows or preceeds a tag, it won't be considered a break
    while( ( ( newline_pos + 1 ) < text.size() ) &&
           ( ( newline_pos = text.find(NewLineMarker_sv, newline_pos + 1) ) != std::wstring::npos ) )
    {
        if( ( newline_pos > 0 && text[newline_pos - 1] == '>' ) ||
            ( ( newline_pos + NewLineMarker_sv.length() ) < text.length() && text[newline_pos + NewLineMarker_sv.length()] == '<' ) )
        {
            text.erase(newline_pos, NewLineMarker_sv.length());
        }
    }

    SO::Replace(text, NewLineMarker_sv, _T("<br />\n"));

    return text;
}


std::wstring CSDocCompilerWorker::ProcessText(std::wstring& text)
{
    std::optional<size_t> next_tag_offset = 0;

    while( next_tag_offset.has_value() )
    {
        const TagPosition tag_position = GetTagPosition(text, *next_tag_offset);
        next_tag_offset.reset();

        // return if there are no more tags
        if( tag_position.start == wstring_view::npos )
            return std::exchange(text, std::wstring());

        if( tag_position.end == wstring_view::npos )
            throw CSProException(_T("Invalid tag construction around: ") + text.substr(tag_position.start));

        const std::vector<std::wstring> tag_components = GetTagComponents(text, tag_position);

        if( tag_components.empty() && !m_inBlockTag )
            throw CSProException(_T("Empty tag around: ") + text.substr(tag_position.start));

        std::wstring before_tag_text = text.substr(0, tag_position.start);

        // process end tags...
        if( !tag_components.empty() && SO::StartsWith(tag_components.front(), _T("/"))  )
        {
            if( m_tagStack.empty() )
                throw CSProException(_T("End tag without a start tag around: ") + text.substr(tag_position.start));

            wstring_view end_tag_sv = wstring_view(tag_components.front()).substr(1);

            // if in a block, ignore end tags unless they are ending the block tag
            if( m_inBlockTag && end_tag_sv != m_tagStack.top() )
            {
                next_tag_offset = tag_position.end + 1;
                continue;
            }

            if( end_tag_sv != m_tagStack.top() )
            {
                throw CSProException(_T("End tag </%s> does not match the last start tag <%s> around: %s"),
                                     std::wstring(end_tag_sv).c_str(), m_tagStack.top().c_str(), text.substr(tag_position.start).c_str());
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

        const std::wstring& start_tag = tag_components.front();
        const auto& tag_definition_lookup = m_sd.tag_definitions.find(start_tag);

        if( tag_definition_lookup == m_sd.tag_definitions.cend() )
            throw CSProException(_T("Invalid tag around: ") + text.substr(tag_position.start));

        // see if this is a block tag
        if( m_sd.block_tags.find(start_tag) != m_sd.block_tags.cend() )
            m_inBlockTag = true;

        const TagDefinition& tag_definition = tag_definition_lookup->second;

        // check that the tag ends correctly
        if( !tag_definition.paired )
        {
            if( tag_components.size() == 1 || tag_components.back() != _T("/") )
            {
                throw CSProException(_T("The tag '%s' is not paired and must end with / around: %s"),
                                     start_tag.c_str() ,text.substr(tag_position.start).c_str());
            }
        }

        // get only the real tag components
        const std::wstring* first_real_tag_component = tag_components.data() + 1;
        const std::wstring* last_real_tag_component = first_real_tag_component + tag_components.size() - 1 - ( tag_definition.paired ? 0 : 1 );

        const cs::span<const std::wstring> real_tag_components(first_real_tag_component, last_real_tag_component - first_real_tag_component);

        // a routine to confirm that the number of tag components is valid
        auto check_num_tag_components = [&](size_t num_tag_components)
        {
            if( num_tag_components < tag_definition.min_components )
            {
                throw CSProException(_T("The tag '%s' must have at least %d argument%s around: %s"),
                                     start_tag.c_str(), static_cast<int>(tag_definition.min_components),
                                     PluralizeWord(tag_definition.min_components), text.substr(tag_position.start).c_str());
            }

            if( num_tag_components > tag_definition.max_components )
            {
                throw CSProException(_T("The tag '%s' must have at most %d argument%s around: %s"),
                                     start_tag.c_str(), static_cast<int>(tag_definition.max_components),
                                     PluralizeWord(tag_definition.max_components), text.substr(tag_position.start).c_str());
            }
        };

        // process the start tag (using version 0 or 8 processing)
        std::wstring output;

        if( std::holds_alternative<TagDefinition::StartHandlerFunctionV8>(tag_definition.start_handler) )
        {
            const std::map<wstring_view, wstring_view> mapped_tag_components = ProcessTagComponentsV8(real_tag_components);
            check_num_tag_components(mapped_tag_components.size());

            output = (this->*std::get<TagDefinition::StartHandlerFunctionV8>(tag_definition.start_handler))(start_tag, mapped_tag_components);
        }

        else
        {
            check_num_tag_components(real_tag_components.size());
            output = StartTagV0(tag_definition, real_tag_components);
        }

        std::wstring after_tag_text = text.substr(tag_position.end + 1);

        if( tag_definition.paired )
        {
            m_tagStack.push(start_tag);

            const std::wstring inner_text = ProcessText(after_tag_text);
            output.append(EndTag(tag_definition, inner_text));
        }

        output = before_tag_text + output + ProcessText(after_tag_text);

        text = after_tag_text;

        return output;
    }

    return ReturnProgrammingError(std::wstring());
}


std::wstring CSDocCompilerWorker::StartTagV0(const TagDefinition& tag_definition, cs::span<const std::wstring> tag_components)
{
    if( std::holds_alternative<std::monostate>(tag_definition.start_handler) )
    {
        return std::wstring();
    }

    else if( std::holds_alternative<std::wstring>(tag_definition.start_handler) )
    {
        return std::get<std::wstring>(tag_definition.start_handler);
    }

    else
    {
        ASSERT(std::holds_alternative<TagDefinition::StartHandlerFunctionV0>(tag_definition.start_handler));
        return (this->*std::get<TagDefinition::StartHandlerFunctionV0>(tag_definition.start_handler))(tag_components);
    }
}


std::wstring CSDocCompilerWorker::EndTag(const TagDefinition& tag_definition, const std::wstring& inner_text)
{
    if( std::holds_alternative<std::monostate>(tag_definition.end_handler) )
    {
        return inner_text;
    }

    else if( std::holds_alternative<std::wstring>(tag_definition.end_handler) )
    {
        return inner_text + std::get<std::wstring>(tag_definition.end_handler);
    }

    else
    {
        ASSERT(std::holds_alternative<TagDefinition::EndHandlerFunction>(tag_definition.end_handler));
        return (this->*std::get<TagDefinition::EndHandlerFunction>(tag_definition.end_handler))(inner_text);
    }
}


std::wstring CSDocCompilerWorker::ParseStringLiteral(const wstring_view text_sv, size_t& pos)
{
    const TCHAR quotemark = text_sv[pos];
    ASSERT(quotemark == '"' || quotemark == '\'');

    std::wstring string_literal;
    bool last_character_was_an_escape = false;

    for( ++pos; pos < text_sv.length(); ++pos )
    {
        const TCHAR ch = text_sv[pos];

        if( last_character_was_an_escape )
        {
            const TCHAR escaped_representation = Encoders::GetEscapedRepresentation(ch);

            // invalid escape sequence
            if( escaped_representation == 0 )
                throw CSProException(_T("Invalid escape sequence, \\%c, in the string literal."), ch);

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

    throw CSProException(_T("Missing end quote (%c) in the string literal."), quotemark);
}


PortableColor CSDocCompilerWorker::ParsePortableColor(const wstring_view tag_or_attribute_name_sv, const bool first_argument_is_tag, const std::wstring& text)
{
    std::optional<PortableColor> color = PortableColor::FromString(text);

    if( !color.has_value() )
    {
        throw CSProException(_T("The '%s' %s contains an invalid color value: %s"),
                             std::wstring(tag_or_attribute_name_sv).c_str(),
                             first_argument_is_tag ? _T("tag") : _T("attribute"),
                             text.c_str());
    }

    return *color;
}


int CSDocCompilerWorker::ParseInt(const wstring_view attribute_name_sv, const wstring_view text_sv)
{
    if( !CIMSAString::IsInteger(text_sv) )
    {
        throw CSProException(_T("The '%s' attribute contains an invalid integer: %s"),
                             std::wstring(attribute_name_sv).c_str(),
                             std::wstring(text_sv).c_str());
    }

    return static_cast<int>(CIMSAString::Val(text_sv));
}


int CSDocCompilerWorker::ParseInt(const wstring_view attribute_name_sv, const wstring_view text_sv, const int min_value)
{
    const int value = ParseInt(attribute_name_sv, text_sv);

    if( value < min_value )
    {
        throw CSProException(_T("The '%s' attribute must contain an integer greater than or equal to %d, not: %s"),
                             std::wstring(attribute_name_sv).c_str(),
                             min_value,
                             std::wstring(text_sv).c_str());
    }

    return value;
}


bool CSDocCompilerWorker::TryParseInt(const wstring_view text_sv, int& value)
{
    if( CIMSAString::IsInteger(text_sv) )
    {
        value = static_cast<int>(CIMSAString::Val(text_sv));
        return true;
    }

    return false;
}


bool CSDocCompilerWorker::TryParseDouble(const wstring_view text_sv, double& value)
{
    if( CIMSAString::IsNumeric(text_sv) )
    {
        value = CIMSAString::fVal(text_sv);
        return true;
    }

    return false;
}


void CSDocCompilerWorker::AppendImageWidthHeight(std::wstring& html, const std::optional<int> width, const std::optional<int> height, const bool close_tag)
{
    if( width.has_value() )
        SO::AppendFormat(html, _T(" \" width=\"%d\""), *width);

    if( height.has_value() )
        SO::AppendFormat(html, _T("\" height=\"%d\""), *height);

    if( close_tag )
        html.append(_T(" />"));
}


std::wstring CSDocCompilerWorker::EndTagWithContentsOfTextStack(const std::wstring& inner_text)
{
    ASSERT(!m_endTagTextStack.empty());
    std::wstring result = inner_text + m_endTagTextStack.top();
    m_endTagTextStack.pop();
    return result;
}


std::wstring CSDocCompilerWorker::TitleStartHandler(cs::span<const std::wstring> tag_components)
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
        throw CSProException(_T("Unknown 'title' tag: ") + tag_components.front());
    }

    return std::wstring();
}


std::wstring CSDocCompilerWorker::ContextStartHandler(const cs::span<const std::wstring> tag_components)
{
    for( const std::wstring& tag_component : tag_components )
    {
        wstring_view context_sv = tag_component;
        const bool use_if_exists = ( context_sv.front() == '!' );

        if( use_if_exists )
            context_sv = context_sv.substr(1);

        m_settings.GetContextId(context_sv, use_if_exists);
    }

    return std::wstring();
}


std::wstring CSDocCompilerWorker::TitleEndHandler(const std::wstring& inner_text)
{
    ASSERT(m_title->empty() || *m_title == NoHeaderAttribute_sv);

    std::wstring header;

    if( *m_title != NoHeaderAttribute_sv )
    {
        header = _T("<h2><span class=\"header_size header\">") + Encoders::ToHtml(inner_text) + _T("</span></h2>");

        const std::wstring url = m_settings.CreateUrlForTitle(m_settings.GetCompilationFilename());

        if( !url.empty() )
        {
            const std::wstring a_tag_start = CreateHyperlinkStart(url, true, false);
            header = a_tag_start + _T(" style=\"text-decoration: none;\">") + header + _T("</a>");
        }
    }

    m_title = inner_text;

    // update the database of titles
    m_settings.SetTitleForCompilationFilename(*m_title);

    return header;
}


std::wstring CSDocCompilerWorker::IndentStartHandler(const cs::span<const std::wstring> tag_components)
{
    int indents = 1;

    if( !tag_components.empty() && ( !TryParseInt(tag_components.front(), indents) || indents < 1 ) )
        throw CSProException(_T("The 'indent' tag has an invalid attribute: ") + tag_components.front());

    std::wstring text;
    std::wstring end_text;

    while( indents-- != 0 )
    {
        text.append(_T("<div class=\"indent\">"));
        end_text.append(_T("</div>"));
    }

    m_endTagTextStack.push(end_text);

    return text;
}


std::wstring CSDocCompilerWorker::FontStartHandler(const cs::span<const std::wstring> tag_components)
{
    std::wstring classes;
    std::wstring styles;
    int types = 0;
    int sizes = 0;
    int colors = 0;

    for( const std::wstring& tag_component : tag_components )
    {
        double em;

        // check for the font type
        if( tag_component == FontMonospaceAttribute_sv )
        {
            classes.append(_T("monospace "));
            ++types;
        }

        // check for the font size
        else if( tag_component == HeaderAttribute_sv || tag_component == SubheaderTag_sv )
        {
            classes.append(tag_component + _T("_size "));
            ++sizes;
        }

        else if( TryParseDouble(tag_component, em) )
        {
            styles.append(_T("font-size: ") + DoubleToString(em) + _T("em; "));
            ++sizes;
        }

        // check for font color
        else
        {
            const PortableColor color = ParsePortableColor(_T("font"), true, tag_component);
            styles.append(_T("color: ") + color.ToString() + _T("; "));
            ++colors;
        }

        if( types > 1 || sizes > 1 || colors > 1 )
            throw CSProException("The 'font' tag cannot have more than one type, size, or color attribute.");
    }

    std::wstring result = _T("<span");

    if( !classes.empty() )
        result.append(_T(" class=\"") + classes + _T("\""));

    if( !styles.empty() )
        result.append(_T(" style=\"") + styles + _T("\""));

    result.push_back('>');

    return result;
}


std::wstring CSDocCompilerWorker::ListStartHandler(cs::span<const std::wstring> tag_components)
{
    bool unordered_list = true;

    if( !tag_components.empty() )
    {
        if( tag_components.front() != OrderedAttribute_sv )
            throw CSProException(_T("The 'list' tag has an invalid attribute: ") + tag_components.front());

        unordered_list = false;
    }

    m_endTagTextStack.push(unordered_list ? _T("</ul>") : _T("</ol>"));

    return unordered_list ? _T("<ul>") : _T("<ol>");
}


std::wstring CSDocCompilerWorker::ImageStartHandler(cs::span<const std::wstring> tag_components)
{
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int>* dimension_specifying = nullptr;
    std::wstring image_path;
    bool nochm = false;

    for( const std::wstring& tag_component : tag_components )
    {
        if( dimension_specifying != nullptr )
        {
            if( !TryParseInt(tag_component, dimension_specifying->emplace()) || *dimension_specifying <= 0 )
                throw CSProException(_T("The image width and height must be positive integers (not '%s')."), tag_component.c_str());

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
            throw CSProException(_T("The 'image' tag has an invalid or duplicated attribute: ") + tag_component);
        }
    }

    if( dimension_specifying != nullptr )
        throw CSProException("The image width or height were not specified.");

    if( image_path.empty() )
        throw CSProException("The image location must be specified.");

    if( nochm && m_settings.CompilingForCompiledHtmlHelp() )
        return std::wstring();

    image_path = m_settings.EvaluateImagePath(image_path);

    if( !PortableFunctions::FileIsRegular(image_path) )
        throw CSProException(_T("The image could not be located: ") + image_path);

    // for accessibility, set the title to the name of the image, replacing underscores with spaces
    std::wstring title = PortableFunctions::PathGetFilenameWithoutExtension(image_path);
    SO::Replace(title, '_', ' ');

    std::wstring html = _T("<img src=\"") + Encoders::ToHtmlTagValue(m_settings.CreateUrlForImageFile(image_path)) +
                        _T("\" title=\"") + Encoders::ToHtmlTagValue(title) + _T("\"");

    AppendImageWidthHeight(html, width, height, true);

    return html;
}


std::wstring CSDocCompilerWorker::BarcodeStartHandler(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components)
{
    constexpr wstring_view ErrorCorrectionAttribute_sv = _T("errorCorrection");
    constexpr wstring_view ScaleAttribute_sv           = _T("scale");
    constexpr wstring_view QuietZoneAttribute_sv       = _T("quietZone");
    constexpr wstring_view DarkColorAttribute_sv       = _T("darkColor");
    constexpr wstring_view LightColorAttribute_sv      = _T("lightColor");

    ValidateTagComponentsV8(start_tag, tag_components,
                            { TextAttribute_sv },
                            { ErrorCorrectionAttribute_sv, ScaleAttribute_sv, QuietZoneAttribute_sv, DarkColorAttribute_sv, LightColorAttribute_sv, ImageWidthAttribute_sv, ImageHeightAttribute_sv },
                            { TextAttribute_sv });

    Multimedia::QRCode qr_code;
    const wstring_view text_sv = tag_components.at(TextAttribute_sv);
    std::optional<int> width;
    std::optional<int> height;

    ExecuteWithTagValue(tag_components, ErrorCorrectionAttribute_sv,
                        [&](const wstring_view value_sv) { qr_code.SetErrorCorrectionLevel(value_sv); });

    ExecuteWithTagValue(tag_components, ScaleAttribute_sv,
                        [&](const wstring_view value_sv) { qr_code.SetScale(ParseInt(ScaleAttribute_sv, value_sv)); });

    ExecuteWithTagValue(tag_components, QuietZoneAttribute_sv,
                        [&](const wstring_view value_sv) { qr_code.SetQuietZone(ParseInt(QuietZoneAttribute_sv, value_sv)); });

    ExecuteWithTagValue(tag_components, DarkColorAttribute_sv,
                        [&](const wstring_view value_sv) { qr_code.SetDarkColor(ParsePortableColor(DarkColorAttribute_sv, false, value_sv)); });

    ExecuteWithTagValue(tag_components, LightColorAttribute_sv,
                        [&](const wstring_view value_sv) { qr_code.SetLightColor(ParsePortableColor(LightColorAttribute_sv, false, value_sv)); });

    ExecuteWithTagValue(tag_components, ImageWidthAttribute_sv,
                        [&](const wstring_view value_sv) { width = ParseInt(ImageWidthAttribute_sv, value_sv, 1); });

    ExecuteWithTagValue(tag_components, ImageHeightAttribute_sv,
                        [&](const wstring_view value_sv) { height = ParseInt(ImageHeightAttribute_sv, value_sv, 1); });

    qr_code.Create(UTF8Convert::WideToUTF8(text_sv));

    std::unique_ptr<Multimedia::Image> qr_code_bitmap = qr_code.GetImage();
    std::unique_ptr<std::vector<std::byte>> qr_code_png = qr_code_bitmap->ToBuffer(ImageType::Png);

    if( qr_code_png == nullptr )
        throw CSProException(_T("There was an error generating a QR code for text: ") + std::wstring(text_sv));

    const std::wstring data_url = Encoders::ToDataUrl(*qr_code_png, MimeType::Type::ImagePng);
    ASSERT(data_url == Encoders::ToHtmlTagValue(data_url));

    std::wstring html = _T("<img src=\"") + data_url +
                        _T("\" title=\"") + Encoders::ToHtmlTagValue(text_sv) + _T("\"");

    AppendImageWidthHeight(html, width, height, true);

    return html;
}


CSDocCompilerWorker::PathAndProject CSDocCompilerWorker::GetPathAndProjectForTopicComponent(CSDocCompilerSettings& settings, const std::wstring& topic_component)
{
    if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(topic_component), FileExtensions::CSDocument) )
        throw CSProException(_T("The file cannot be linked as it is not a CSPro Document: ") + topic_component);

    PathAndProject path_and_project;

    constexpr std::wstring_view ProjectIndicator = _T("::");
    const size_t double_colon_pos = topic_component.find(ProjectIndicator);

    if( double_colon_pos != std::wstring::npos )
    {
        path_and_project.project = topic_component.substr(0, double_colon_pos);
        path_and_project.path = settings.EvaluateTopicPath(path_and_project.project,
                                                           topic_component.substr(double_colon_pos + ProjectIndicator.length()));
    }

    else
    {
        path_and_project.path = settings.EvaluateTopicPath(topic_component);
    }

    if( !PortableFunctions::FileIsRegular(path_and_project.path) )
        throw CSProException(_T("The document could not be located: ") + topic_component);

    return path_and_project;
}


std::wstring CSDocCompilerWorker::EvaluateAndCreateUrlForTopicComponent(CSDocCompilerSettings& settings, const std::wstring& topic_component)
{
    const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(settings, topic_component);
    return settings.CreateUrlForTopic(path_and_project.project, path_and_project.path);;
}


std::wstring CSDocCompilerWorker::CreateHyperlinkStart(wstring_view CreateUrlForFile_result_sv,
                                                       const bool target_blank/* = false*/, const bool end_tag/* = true*/)
{
    constexpr TCHAR OnClickIndicator = '!';

    if( CreateUrlForFile_result_sv.empty() )
        return _T("<a>");

    std::wstring html = _T("<a href=\"");

    if( CreateUrlForFile_result_sv.front() == OnClickIndicator )
    {
        html.push_back('#');

        if( !CreateUrlForFile_result_sv.empty() )
        {
            html.append(_T("\" onclick=\""));
            CreateUrlForFile_result_sv = CreateUrlForFile_result_sv.substr(1);
        }
    }

    html.append(Encoders::ToHtmlTagValue(CreateUrlForFile_result_sv));

    if( target_blank )
        html.append(_T("\" target=\"_blank"));

    html.append(end_tag ? _T("\">") :
                          _T("\""));

    return html;
}


std::wstring CSDocCompilerWorker::TopicStartHandler(const cs::span<const std::wstring> tag_components)
{
    const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(tag_components.front());
    const std::wstring title = m_settings.GetTitle(path_and_project.path);

    std::wstring html = CreateHyperlinkStart(m_settings.CreateUrlForTopic(path_and_project.project, path_and_project.path));
    return html + Encoders::ToHtml(title) + _T("</a>");
}


std::wstring CSDocCompilerWorker::LinkStartHandler(const cs::span<const std::wstring> tag_components)
{
    std::wstring url = tag_components.front();
    bool target_blank = false;

    if( SO::StartsWith(url, _T("http")) || SO::StartsWith(url, _T("mailto")) )
    {
        target_blank = m_settings.OpenExternalLinksInSeparateWindow();
    }

    else
    {
        const PathAndProject path_and_project = GetPathAndProjectForTopicComponent(url);
        url = m_settings.CreateUrlForTopic(path_and_project.project, path_and_project.path);
    }

    return CreateHyperlinkStart(url, target_blank);
}


std::wstring CSDocCompilerWorker::SeeAlsoStartHandler(const cs::span<const std::wstring> tag_components)
{
    std::vector<std::tuple<std::wstring, std::wstring>> urls_and_titles;
    bool ordered_list = false;

    for( const std::wstring& tag_component : tag_components )
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

    std::wstring html;

    for( const auto& [url, title] : urls_and_titles )
    {
        html.append(html.empty() ? _T("<b>See also</b>: ") : _T(", "));
        html.append(CreateHyperlinkStart(url) + Encoders::ToHtml(title) + _T("</a>"));
    }

    return html;
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


std::wstring CSDocCompilerWorker::TableStartHandler(const cs::span<const std::wstring> tag_components)
{
    auto table_settings = std::make_shared<TableSettings>();

    for( const std::wstring& tag_component : tag_components )
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
            throw CSProException(_T("The 'table' tag has an invalid attribute: ") + tag_component);
        }
    }

    if( table_settings->columns == 0 )
        throw CSProException("The number of columns in a table must be specified.");

    m_tableStack.push(table_settings);

    return table_settings->border ? _T("<table class=\"bordered_table\">") : _T("<table>");
}


std::wstring CSDocCompilerWorker::TableEndHandler(const std::wstring& inner_text)
{
    const TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;

    if( cell_index != 0 )
        throw CSProException(_T("You cannot end the table without specifying an additional '%d' cells."), table_settings.columns - cell_index);

    m_tableStack.pop();

    return inner_text + _T("</table>");
}


std::wstring CSDocCompilerWorker::TableCellStartHandler(const cs::span<const std::wstring> tag_components)
{
    TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;
    const bool is_header_row = ( table_settings.header && table_settings.cells < table_settings.columns );
    const bool is_first_cell_in_row = ( cell_index == 0 );
    bool nowrap = ( table_settings.nowrap && is_first_cell_in_row );
    int columns = 1;

    for( const std::wstring& tag_component : tag_components )
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

    const std::wstring row_prefix = is_first_cell_in_row ? _T("<tr>") : std::wstring();
    const std::wstring cell_type = is_header_row ? _T("th") : _T("td");
    const std::wstring span = ( columns > 1 ) ? FormatTextCS2WS(_T(" colspan=\"%d\""), columns) : std::wstring();

    std::wstring style;

    if( nowrap )
        style.append(_T("white-space: nowrap; "));

    if( table_settings.center )
        style.append(_T("text-align: center;"));

    const std::wstring class_str = table_settings.border ? _T(" class=\"bordered_table_cell\"") : std::wstring();

    if( !style.empty() )
    {
        SO::MakeTrim(style);
        style = _T(" style=\"") + style + _T("\"");
    }

    return row_prefix + _T("<") + cell_type + style + span + class_str + _T(">");
}


std::wstring CSDocCompilerWorker::TableCellEndHandler(const std::wstring& inner_text)
{
    const TableSettings& table_settings = GetCurrentTable();
    const int cell_index = table_settings.cells % table_settings.columns;
    const bool is_header_row = ( table_settings.header && table_settings.cells <= table_settings.columns );
    const bool is_last_cell_in_row = ( cell_index == 0 );

    return inner_text + ( is_header_row       ? _T("</th>")   : _T("</td>") ) +
                        ( is_last_cell_in_row ? _T("</tr>\n") : _T("") );
}


std::wstring CSDocCompilerWorker::NoteStartHandler(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components)
{
    constexpr wstring_view TypeAttribute_sv    = _T("type");
    constexpr wstring_view TypeErrorValue_sv   = _T("error");
    constexpr wstring_view TypeWarningValue_sv = _T("warning");
    constexpr wstring_view TypeTodoValue_sv    = _T("todo");
    constexpr wstring_view TypeCommentValue_sv = _T("comment");

    ValidateTagComponentsV8(start_tag, tag_components,
                            { TextAttribute_sv },
                            { TypeAttribute_sv });

    std::wstring text;
    ExecuteWithTagValue(tag_components, TextAttribute_sv,
                        [&](const wstring_view value_sv) { text = value_sv; });

    enum class NoteType { Error, Warning, Todo, Comment };
    NoteType note_type = NoteType::Comment;
    ExecuteWithTagValue(tag_components, TypeAttribute_sv,
        [&](const wstring_view value_sv)
        {
            note_type = ( value_sv == TypeErrorValue_sv )   ? NoteType::Error :
                        ( value_sv == TypeWarningValue_sv ) ? NoteType::Warning :
                        ( value_sv == TypeTodoValue_sv )    ? NoteType::Todo:
                        ( value_sv == TypeCommentValue_sv ) ? NoteType::Comment :
                                                              throw CSProException(_T("Invalid note type: '%s'"), std::wstring(value_sv).c_str());
        });

    switch( note_type )
    {
        case NoteType::Error:
            throw CSProException(text);

        case NoteType::Warning:
            m_settings.AddCompilerMessage(CompilerMessageType::Warning, text);
            break;

        case NoteType::Todo:
            m_settings.AddCompilerMessage(CompilerMessageType::Info, _T("TODO: ") + text);
            break;

        default:
            ASSERT(note_type == NoteType::Comment);
            break;
    }

    return std::wstring();
}


std::wstring CSDocCompilerWorker::BuildExtraStartHandler(cs::span<const std::wstring> tag_components)
{
    for( const std::wstring& tag_component : tag_components )
        m_settings.EvaluateBuildExtra(tag_component);

    return std::wstring();
}
