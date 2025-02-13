#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include <zToolsO/SharedPointerHelpers.h>
#include <zToolsO/VariantVisitOverload.h>
#include <zAction/ActionInvoker.h>
#include <zLogicO/ChildSymbolNames.h>
#include <zLogicO/ContextSensitiveHelp.h>
#include <zLogicO/FunctionTable.h>
#include <zLogicO/ReservedWords.h>
#include <zLogicO/Symbol.h>
#include <zEdit2O/ScintillaColorizer.h>


namespace
{
    constexpr const TCHAR* AllSymbolsDomainText = _T("Symbol");
    constexpr const TCHAR* CaseSymbolDomainText = _T("Case"); // ENGINECR_TODO remove because this should be part of Symbol::GetDeclarationTextMap
}


// --------------------------------------------------------------------------
// HelpsHtmlProcessor
// --------------------------------------------------------------------------

enum class HelpsHtmlProcessorMode { Normal, Inline, Syntax };


class HelpsHtmlProcessor : public ScintillaColorizer::HtmlProcessor
{
public:
    HelpsHtmlProcessor(HelpsHtmlProcessorMode mode);
    HelpsHtmlProcessor(HelpsHtmlProcessorMode mode, CSDocCompilerSettings& settings, std::optional<Logic::FunctionDomain> logic_function_domain);

    std::wstring PreprocessLogic(std::wstring text) const;
    static void CheckLogicCase(wstring_view text_sv, const Logic::FunctionDomain& logic_function_domain = SymbolType::None);

protected:
    std::vector<ScintillaColorizer::ExtendedEntity> GetExtendedEntities(const std::vector<ScintillaColorizer::Entity>& entities) const override;

private:
    void WriteHtmlHeader(std::wstringstream& output) const override;
    void WriteHtmlFooter(std::wstringstream& output) const override;

    void CreateLink(ScintillaColorizer::ExtendedEntity& extended_entity, const TCHAR* help_topic_filename) const;

    std::optional<size_t> FindWordIndexBeforeDotNotation(const std::vector<ScintillaColorizer::Entity>& entities, size_t start_index, bool skip_over_subscripts) const;

    static std::optional<std::tuple<SymbolType, char>> GetSymbolTypeAndStyleFromDeclarationText(const std::wstring& symbol_type_name);

    std::optional<SymbolType> FindSymbolTypeFromVariableDeclaration(const std::vector<ScintillaColorizer::Entity>& entities,
                                                                    const std::map<std::wstring, size_t>& first_identifier_location_map,
                                                                    const std::wstring& symbol_name) const;

    void PostprocessExtendedEntities(std::vector<ScintillaColorizer::ExtendedEntity>& extended_entities) const;

private:
    const HelpsHtmlProcessorMode m_mode;
    CSDocCompilerSettings* m_settings;
    const std::optional<Logic::FunctionDomain> m_logicFunctionDomain;

    static constexpr const TCHAR* ArgTagStart             = _T("<arg>");
    static constexpr const TCHAR* ArgTagEnd               = _T("</arg>");
    static constexpr TCHAR OptionalTagStart               = _T('『');
    static constexpr TCHAR OptionalTagEnd                 = _T('』');
    static constexpr TCHAR OptionalTagSeparator           = _T('‖');

    static constexpr wstring_view ArgReplacementTextStart = _T("arg_start_");
    static constexpr wstring_view ArgReplacementTextEnd   = _T("_end_arg");
    static constexpr TCHAR OptionalReplacementStart       = _T('ʃ');
    static constexpr TCHAR OptionalReplacementEnd         = _T('ʅ');
};


HelpsHtmlProcessor::HelpsHtmlProcessor(HelpsHtmlProcessorMode mode)
    :   m_mode(mode),
        m_settings(nullptr)
{
}


HelpsHtmlProcessor::HelpsHtmlProcessor(HelpsHtmlProcessorMode mode, CSDocCompilerSettings& settings, std::optional<Logic::FunctionDomain> logic_function_domain)
    :   m_mode(mode),
        m_settings(&settings),
        m_logicFunctionDomain(std::move(logic_function_domain))
{
}


void HelpsHtmlProcessor::WriteHtmlHeader(std::wstringstream& output) const
{
    output << ( ( m_mode == HelpsHtmlProcessorMode::Inline ) ? _T("<span class=\"code_colorization\">") :
                                                               _T("<div class=\"code_colorization indent\">") );

}

void HelpsHtmlProcessor::WriteHtmlFooter(std::wstringstream& output) const
{
    output << ( ( m_mode == HelpsHtmlProcessorMode::Inline ) ? _T("</span>") :
                                                               _T("</div>") );
}


namespace
{
    constexpr bool IsStateAnyCommentOrStringLiteralType(int state)
    {
        return ( state == SCE_CSPRO_COMMENT ||
                 state == SCE_CSPRO_COMMENTLINE ||
                 state == SCE_CSPRO_STRING ||
                 state == SCE_CSPRO_STRING_ESCAPE );
    }
}


void HelpsHtmlProcessor::CreateLink(ScintillaColorizer::ExtendedEntity& extended_entity, const TCHAR* help_topic_filename) const
{
    ASSERT(help_topic_filename != nullptr);

    const std::wstring url = m_settings->CreateUrlForLogicTopic(help_topic_filename);

    if( url.empty() )
    {
        extended_entity.entity_spanning_tags.emplace_back();
    }

    else
    {
        const std::wstring a_tag_start = CSDocCompilerWorker::CreateHyperlinkStart(url, false, false);

        extended_entity.entity_spanning_tags.emplace_back(a_tag_start + _T(" class=\"code_colorization_keyword_link\">"),
                                                          _T("</a>"));
    }
}


std::optional<size_t> HelpsHtmlProcessor::FindWordIndexBeforeDotNotation(const std::vector<ScintillaColorizer::Entity>& entities,
                                                                         const size_t start_index, const bool skip_over_subscripts) const
{
    bool next_entity_must_be_dot = true;
    size_t subscript_parentheses_counter = 0;

    for( size_t i = start_index; i < entities.size(); --i )
    {
        const ScintillaColorizer::Entity& entity = entities[i];

        // ignore comments and strings literals
        if( IsStateAnyCommentOrStringLiteralType(entity.style) )
            continue;

        if( next_entity_must_be_dot )
        {
            if( entity.text != _T(".") )
                return std::nullopt;

            next_entity_must_be_dot = false;
            continue;
        }

        if( skip_over_subscripts )
        {
            bool processed_parenthesis = false;

            for( const TCHAR ch : entity.text )
            {
                if( ch == ')' )
                {
                    ++subscript_parentheses_counter;
                    processed_parenthesis = true;
                }

                else if( ch == '(' )
                {
                    --subscript_parentheses_counter;
                    processed_parenthesis = true;
                }
            }

            if( processed_parenthesis )
                continue;
        }

        if( subscript_parentheses_counter == 0 )
            return i;
    }

    return std::nullopt;
}


std::optional<std::tuple<SymbolType, char>> HelpsHtmlProcessor::GetSymbolTypeAndStyleFromDeclarationText(const std::wstring& symbol_type_name)
{
    const std::map<std::wstring, SymbolType>& symbol_declaration_text_map = ::Symbol::GetDeclarationTextMap();
    const auto& symbol_type_lookup = std::find_if(symbol_declaration_text_map.cbegin(), symbol_declaration_text_map.cend(),
                                                  [&](const auto& name_and_symbol_type) { return SO::EqualsNoCase(symbol_type_name, name_and_symbol_type.first); });

    if( symbol_type_lookup != symbol_declaration_text_map.cend() )
        return std::make_tuple(symbol_type_lookup->second, static_cast<char>(SCE_CSPRO_KEYWORD));

    if( Logic::ReservedWords::GetSpecialFunctions().IsEntry(symbol_type_name, nullptr) )
        return std::make_tuple(SymbolType::UserFunction, static_cast<char>(SCE_CSPRO_IDENTIFIER));

    if( symbol_type_name == CaseSymbolDomainText )
        return std::make_tuple(SymbolType::Pre80Dictionary, static_cast<char>(SCE_CSPRO_KEYWORD));

    return std::nullopt;
}


std::optional<SymbolType> HelpsHtmlProcessor::FindSymbolTypeFromVariableDeclaration(const std::vector<ScintillaColorizer::Entity>& entities,
                                                                                    const std::map<std::wstring, size_t>& first_identifier_location_map,
                                                                                    const std::wstring& symbol_name) const
{
    // search for the first symbol declaration text appearing before the first location that the symbol was used,
    // which will, for example, properly locate 'Array' here: Array alpha (30) xxx;
    const auto& symbol_declaration_lookup = first_identifier_location_map.find(symbol_name);

    if( symbol_declaration_lookup == first_identifier_location_map.cend() )
        return std::nullopt;

    std::optional<SymbolType> matched_symbol_type;

    for( size_t j = symbol_declaration_lookup->second - 1; j < entities.size(); --j )
    {
        const ScintillaColorizer::Entity& previous_entity = entities[j];

        // ignore comments and strings literals
        if( IsStateAnyCommentOrStringLiteralType(previous_entity.style) )
            continue;

        // the symbol type should be the earliest keyword that appears after a semicolon, or after a comma (in function calls),
        // and after any other identifier
        if( previous_entity.style == SCE_CSPRO_KEYWORD )
        {
            const std::optional<std::tuple<SymbolType, char>> symbol_type_and_style = GetSymbolTypeAndStyleFromDeclarationText(previous_entity.text);

            if( symbol_type_and_style.has_value() )
            {
                matched_symbol_type = std::get<0>(*symbol_type_and_style);

                // keep processing (so that, in the example above, "alpha" is not the type but instead we keep processing until we get to "Array")
            }
        }

        else if( previous_entity.style == SCE_CSPRO_IDENTIFIER || _tcschr(_T(";,"), previous_entity.text.front()) != nullptr )
        {
            break;
        }
    }

    return matched_symbol_type;
}


std::vector<ScintillaColorizer::ExtendedEntity> HelpsHtmlProcessor::GetExtendedEntities(const std::vector<ScintillaColorizer::Entity>& entities) const
{
    if( m_settings == nullptr )
        return { };

    std::vector<ScintillaColorizer::ExtendedEntity> extended_entities;
    std::map<std::wstring, size_t> first_identifier_location_map;

    for( size_t i = 0; i < entities.size(); ++i )
    {
        const ScintillaColorizer::Entity& entity = entities[i];

        ScintillaColorizer::ExtendedEntity& extended_entity = extended_entities.emplace_back(ScintillaColorizer::ExtendedEntity
            {
                entity.text,
                entity.style
            });

        // if an identifier, store the location of the first reference
        if( entity.style == SCE_CSPRO_IDENTIFIER )
        {
            // make sure that entries like the "codes" in valueset_name.codes are not added
            if( i == 0 || entities[i - 1].text != _T(".") )
                first_identifier_location_map.try_emplace(entity.text, i);
        }

        // links can come from a few different kinds of entities:
        const TCHAR* help_topic_filename;
        const Logic::FunctionDetails* function_details;

        // 1. multiple word combinations...the word combination must be separated by whitespace (with optional comments in between);
        //    e.g.: skip case;
        auto check_multiple_word_combinations = [&]()
        {
            int matches = 0;

            for( size_t j = i - 1; j < entities.size(); --j )
            {
                const ScintillaColorizer::Entity& previous_entity = entities[j];

                // ignore comments and strings literals
                if( IsStateAnyCommentOrStringLiteralType(previous_entity.style) )
                    continue;

                if( matches <= 1 && SO::IsBlank(previous_entity.text) )
                {
                    matches = 1;
                }

                else if( matches == 1 &&
                            Logic::ContextSensitiveHelp::UpdateTopicFilenameForMultipleWordExpressions(previous_entity.text, entity.text, &help_topic_filename) )
                {
                    CheckLogicCase(previous_entity.text);
                    CheckLogicCase(entity.text);

                    // start the link at the first entity and end it at this entity
                    ASSERT(extended_entities[j].entity_spanning_tags.size() <= 1);
                    extended_entities[j].entity_spanning_tags.clear();
                    CreateLink(extended_entities[j], help_topic_filename);
                    std::get<1>(extended_entities[j].entity_spanning_tags.back()).clear();

                    ASSERT(extended_entities[i].entity_spanning_tags.empty());
                    CreateLink(extended_entities[i], help_topic_filename);
                    std::get<0>(extended_entities[i].entity_spanning_tags.back()).clear();

                    return true;
                }

                else
                {
                    break;
                }
            }

            return false;
        };

        if( check_multiple_word_combinations() )
            continue;


        // 2. symbol dot-notation functions specified without the dot-notation but by specifying the logic function domain
        //    e.g.: $.write("..."); (with m_logicFunctionDomain == SymbolType::Report)
        if( m_logicFunctionDomain.has_value() &&
            Logic::FunctionTable::IsFunctionExtended(entity.text, *m_logicFunctionDomain, &function_details) )
        {
            CheckLogicCase(entity.text, *m_logicFunctionDomain);
            CreateLink(extended_entity, function_details->help_filename);

            // modify the style to what this would be if the logic function domain were specified
            ASSERT(!Logic::FunctionTable::IsFunctionNamespace(entity.text, SymbolType::None));
            extended_entities[i].style = SCE_CSPRO_DOT_NOTATION_FUNCTION;

            continue;
        }


        // 3. keywords
        //    e.g.,: reenter;
        if( entity.style == SCE_CSPRO_KEYWORD )
        {
            CheckLogicCase(entity.text);

            help_topic_filename = Logic::ContextSensitiveHelp::GetTopicFilename(entity.text);

            if( help_topic_filename != nullptr )
                CreateLink(extended_entity, help_topic_filename);

            continue;
        }


        // 4. function namespaces
        //    e.g.: CS.Localhost.createMapping(...);
        if( entity.style == SCE_CSPRO_FUNCTION_NAMESPACE_PARENT || entity.style == SCE_CSPRO_FUNCTION_NAMESPACE_CHILD )
        {
            const Logic::FunctionNamespaceDetails* function_namespace_details = nullptr;
            const std::optional<size_t> parent_namespace_index = FindWordIndexBeforeDotNotation(entities, i - 1, false);

            // handle child namespaces, assuming that a function namespace will only have one parent
            if( parent_namespace_index.has_value() &&
                std::holds_alternative<const Logic::FunctionNamespaceDetails*>(extended_entities[*parent_namespace_index].details) &&
                Logic::FunctionTable::IsFunctionNamespace(entity.text, std::get<const Logic::FunctionNamespaceDetails*>(extended_entities[*parent_namespace_index].details)->function_namespace, &function_namespace_details) )
            {
                CheckLogicCase(entity.text, std::get<const Logic::FunctionNamespaceDetails*>(extended_entities[*parent_namespace_index].details)->function_namespace);
            }

            // handle parent namespaces
            else if( Logic::FunctionTable::IsFunctionNamespace(entity.text, SymbolType::None, &function_namespace_details) )
            {
                CheckLogicCase(entity.text, function_namespace_details->function_namespace);
            }

            if( function_namespace_details != nullptr )
            {
                CreateLink(extended_entity, function_namespace_details->help_filename);

                // store that this was a function namespace
                extended_entity.details = function_namespace_details;

                continue;
            }
        }


        // 5. a symbol name that is part of a dot-notation function (not valid in logic but used in the helps when referring to functions)
        //    e.g.:: List.add(...);
        if( entity.style == SCE_CSPRO_IDENTIFIER )
        {
            const std::optional<std::tuple<SymbolType, char>> symbol_type_and_style = GetSymbolTypeAndStyleFromDeclarationText(entity.text);

            if( symbol_type_and_style.has_value() )
            {
                CheckLogicCase(entity.text);
                CreateLink(extended_entity, Logic::ContextSensitiveHelp::GetTopicFilename(entity.text));

                // modify the symbol name to appear as a keyword (or to stay an identifier for special functions)
                extended_entity.style = std::get<1>(*symbol_type_and_style);

                // store that this was a symbol name
                extended_entity.details = std::get<0>(*symbol_type_and_style);

                continue;
            }
        }


        // 6. dot-notation functions
        //    e.g.,: my_list.add(...);
        if( entity.style == SCE_CSPRO_DOT_NOTATION_FUNCTION )
        {
            const std::optional<size_t> previous_word_index = FindWordIndexBeforeDotNotation(entities, i - 1, true);
            Logic::FunctionDomain logic_function_domain = SymbolType::None;

            if( previous_word_index.has_value() )
            {
                // if the previous word was a symbol type (from #5 above), lookup the function in that symbol domain...
                if( std::holds_alternative<SymbolType>(extended_entities[*previous_word_index].details) )
                {
                    logic_function_domain = std::get<SymbolType>(extended_entities[*previous_word_index].details);
                }

                // ...or look it up in a function namespace (from #4 above)
                else if( std::holds_alternative<const Logic::FunctionNamespaceDetails*>(extended_entities[*previous_word_index].details) )
                {
                    logic_function_domain = std::get<const Logic::FunctionNamespaceDetails*>(extended_entities[*previous_word_index].details)->function_namespace;
                }

                // otherwise search for the first symbol declaration text appearing before the first location that the symbol was used,
                // which will, for example, properly locate 'Array' here: Array alpha (30) xxx;
                else
                {
                    const std::optional<SymbolType> symbol_type = FindSymbolTypeFromVariableDeclaration(entities, first_identifier_location_map, entities[*previous_word_index].text);

                    if( symbol_type.has_value() )
                    {
                        logic_function_domain = *symbol_type;

                        // store the symbol type in case it is used again
                        extended_entities[*previous_word_index].details = *symbol_type;
                    }

                    else if( entities[*previous_word_index].text == AllSymbolsDomainText )
                    {
                        logic_function_domain = Logic::AllSymbolsDomain();
                    }
                }
            }

            // if linked with a namespace or symbol, lookup the function
            if( logic_function_domain != SymbolType::None )
            {
                if( Logic::FunctionTable::IsFunctionExtended(entity.text, logic_function_domain, &function_details) )
                {
                    CreateLink(extended_entity, function_details->help_filename);
                }

                else
                {
                    const TCHAR* const child_symbol_name = std::visit(
                        overload
                        {
                            [](Logic::AllSymbolsDomain)        { return static_cast<const TCHAR*>(nullptr); },
                            [](const std::vector<SymbolType>&) { return ReturnProgrammingError(static_cast<const TCHAR*>(nullptr)); },
                            [&](const auto& object)            { return Logic::LookupChildSymbolName(entity.text, object); }

                        }, logic_function_domain);

                    if( child_symbol_name == nullptr )
                    {
                        if( std::holds_alternative<SymbolType>(logic_function_domain) )
                        {
                            throw CSProException(_T("The logic symbol '%s' does not have a function '%s'"),
                                                 ToString(std::get<SymbolType>(logic_function_domain)),
                                                 entity.text.c_str());
                        }

                        else if( std::holds_alternative<Logic::FunctionNamespace>(logic_function_domain) )
                        {
                            throw CSProException(_T("The function namespace '%s' does not have a function '%s'"),
                                                 Logic::FunctionTable::GetFunctionNamespaceName(std::get<Logic::FunctionNamespace>(logic_function_domain)),
                                                 entity.text.c_str());
                        }

                        else
                        {
                            throw CSProException(_T("The logic function domain does not have a function '%s'"),
                                                 entity.text.c_str());
                        }
                    }
                }

                CheckLogicCase(entity.text, logic_function_domain);
                continue;
            }
        }


        // if here, with no link created, check if this word is part of dot-notation, and then issue an error about the invalid entry
        ASSERT(entity.style != SCE_CSPRO_FUNCTION_NAMESPACE_PARENT && entity.style != SCE_CSPRO_FUNCTION_NAMESPACE_CHILD);

        if( entity.style == SCE_CSPRO_IDENTIFIER || entity.style == SCE_CSPRO_DOT_NOTATION_FUNCTION )
        {
            const std::optional<size_t> previous_word_index = FindWordIndexBeforeDotNotation(entities, i - 1, true);

            if( previous_word_index.has_value() )
            {
                const ScintillaColorizer::ExtendedEntity& previous_extended_entity = extended_entities[*previous_word_index];

                if( std::holds_alternative<const Logic::FunctionNamespaceDetails*>(previous_extended_entity.details) )
                {
                    throw CSProException(_T("The function namespace '%s' does not have a function '%s'"),
                                            Logic::FunctionTable::GetFunctionNamespaceName(std::get<const Logic::FunctionNamespaceDetails*>(previous_extended_entity.details)->function_namespace),
                                            entity.text.c_str());
                }

                const std::optional<SymbolType> symbol_type = FindSymbolTypeFromVariableDeclaration(entities, first_identifier_location_map, previous_extended_entity.text);

                if( symbol_type.has_value() )
                {
                    // an entry here may be like the "codes" in valueset_name.codes
                    if( entity.style == SCE_CSPRO_IDENTIFIER && Logic::LookupChildSymbolName(entity.text, *symbol_type) != nullptr )
                    {
                        CheckLogicCase(entity.text, *symbol_type);
                    }

                    else
                    {
                        throw CSProException(_T("The logic symbol '%s' does not have a function '%s'"), ToString(*symbol_type), entity.text.c_str());
                    }
                }

                // strict checking is disabled by default because it leads to too many false positives
                // (e.g.: Media.Images, or dot-notation functions where the symbol is not defined in the logic)
#ifdef STRICT_CHECKING_FOR_DOT_NOTATION
                if( m_mode != HelpsHtmlProcessorMode::Syntax )
                {
                    throw CSProException(_T("The construction '%s.%s' is not valid"),
                                         entities[*previous_word_index].text.c_str(), entity.text.c_str());
                }
#endif
            }
        }
    }

    PostprocessExtendedEntities(extended_entities);

    return extended_entities;
}


void HelpsHtmlProcessor::PostprocessExtendedEntities(std::vector<ScintillaColorizer::ExtendedEntity>& extended_entities) const
{
    if( m_mode != HelpsHtmlProcessorMode::Syntax )
        return;

    auto split_entity = [&](size_t index, size_t before_split_length, size_t entity_pos, size_t entity_length, size_t after_split_pos)
    {
        ASSERT(entity_length > 0);

        std::wstring original_entity_text = extended_entities[index].text;

        // add a new entity before the current entity
        if( before_split_length > 0 )
        {
            extended_entities.insert(extended_entities.begin() + index, ScintillaColorizer::ExtendedEntity
                {
                    original_entity_text.substr(0, before_split_length),
                    extended_entities[index].style,
                    { },
                    extended_entities[index].entity_specific_tags,
                });

            // and move the entity-spanning end tags
            ++index;

            for( auto& [start_tag, end_tag] : extended_entities[index].entity_spanning_tags )
            {
                extended_entities[index - 1].entity_spanning_tags.emplace_back(start_tag, end_tag);
                start_tag.clear();
            }
        }

        // modify the current entity's text
        extended_entities[index].text = original_entity_text.substr(entity_pos, entity_length);

        // add a new entity after the current entity
        if( after_split_pos < original_entity_text.length() )
        {
            extended_entities.insert(extended_entities.begin() + index + 1, ScintillaColorizer::ExtendedEntity
                {
                    original_entity_text.substr(after_split_pos),
                    extended_entities[index].style,
                    { },
                    extended_entities[index].entity_specific_tags,
                });

            // and move or delete the entity-spanning start tags
            for( auto tag_itr = extended_entities[index].entity_spanning_tags.begin();
                 tag_itr != extended_entities[index].entity_spanning_tags.end(); )
            {
                extended_entities[index + 1].entity_spanning_tags.emplace_back(std::wstring(), std::get<1>(*tag_itr));

                if( std::get<0>(*tag_itr).empty() )
                {
                    tag_itr = extended_entities[index].entity_spanning_tags.erase(tag_itr);
                }

                else
                {
                    std::get<1>(*tag_itr).clear();
                    ++tag_itr;
                }
            }
        }

        return index;
    };


    // process the arguments
    for( size_t i = 0; i < extended_entities.size(); ++i )
    {
        size_t arg_start_pos = extended_entities[i].text.find(ArgReplacementTextStart);

        if( arg_start_pos == std::wstring::npos )
        {
            ASSERT(extended_entities[i].text.find(ArgReplacementTextEnd) == std::wstring::npos);
            continue;
        }

        size_t arg_end_pos = extended_entities[i].text.find(ArgReplacementTextEnd);

        if( arg_end_pos == std::wstring::npos )
            throw CSProException(_T("Malformed argument tag: ") + extended_entities[i].text);

        size_t entity_start_pos = arg_start_pos + ArgReplacementTextStart.length();

        i = split_entity(i, arg_start_pos,
                            entity_start_pos, arg_end_pos - entity_start_pos,
                            arg_end_pos + ArgReplacementTextEnd.length());

        extended_entities[i].entity_specific_tags.emplace_back(_T("<span class=\"code_colorization_argument\">"), _T("</span>"));
    }


    // process the optional tags
    for( size_t i = 0; i < extended_entities.size(); ++i )
    {
        constexpr TCHAR OptionalChars[] { OptionalTagStart, OptionalTagEnd, OptionalTagSeparator, '\0' };

        size_t optional_tag_pos = extended_entities[i].text.find_first_of(OptionalChars);

        if( optional_tag_pos == std::wstring::npos )
            continue;

        TCHAR tag = extended_entities[i].text[optional_tag_pos];

        i = split_entity(i, optional_tag_pos,
                            optional_tag_pos, 1,
                            optional_tag_pos + 1);

        extended_entities[i].entity_specific_tags.emplace_back(_T("<span class=\"code_colorization_bracket\">"), _T("</span>"));

        if( tag == OptionalTagStart )
        {
            extended_entities[i].text = OptionalReplacementStart;
            extended_entities[i].entity_spanning_tags.emplace_back(_T("<span class=\"code_colorization_optional_text\">"), std::wstring());
        }

        else if( tag == OptionalTagEnd )
        {
            extended_entities[i].text = OptionalReplacementEnd;
            extended_entities[i].entity_spanning_tags.emplace_back(std::wstring(), _T("</span>"));
        }
    }
}


std::wstring HelpsHtmlProcessor::PreprocessLogic(std::wstring text) const
{
    if( m_mode != HelpsHtmlProcessorMode::Syntax )
        return text;

    // preprocess the logic syntax tags
    SO::Replace(text, ArgTagStart, ArgReplacementTextStart);
    SO::Replace(text, ArgTagEnd, ArgReplacementTextEnd);

    return text;
}


void HelpsHtmlProcessor::CheckLogicCase(const wstring_view text_sv, const Logic::FunctionDomain& logic_function_domain/* = SymbolType::None*/)
{
    // first check if this is a word that can be written in multiple cases
    static const std::vector<const TCHAR*> MultipleCaseWords =
    {
        _T("array"),
        _T("case"),
        _T("ErrMsg"),
        _T("image"),
        _T("report"),
        _T("setProperty"),
        _T("valueset")
    };

    if( std::find_if(MultipleCaseWords.cbegin(), MultipleCaseWords.cend(),
                     [&](const std::wstring& word) { return ( word == text_sv ); }) != MultipleCaseWords.cend() )
    {
        return;
    }

    // otherwise check the case
    const TCHAR* const defined_case = Logic::ReservedWords::GetDefinedCase(text_sv, logic_function_domain);

    if( defined_case != nullptr && !SO::Equals(text_sv, defined_case) )
    {
        throw CSProException(_T("The reserved word '%s' must be used with the defined case: '%s'"),
                             std::wstring(text_sv).c_str(), defined_case);
    }
}



// --------------------------------------------------------------------------
// CSDocCompilerWorker
// --------------------------------------------------------------------------

std::wstring CSDocCompilerWorker::TrimOnlyOneNewlineFromBothEnds(const std::wstring& text)
{
    ASSERT(text.find('\r') == std::wstring::npos);

    if( !text.empty() )
    {
        const size_t start_pos = ( text.front() == '\n' ) ? 1 : 0;
        const bool trim_end = ( text.back() == '\n' && text.length() > 1 );

        if( start_pos != 0 || trim_end )
            return text.substr(start_pos, text.length() - start_pos - ( trim_end ? 1 : 0 ));
    }

    return text;
}


std::wstring CSDocCompilerWorker::LogicObjectStartHandler(cs::span<const std::wstring> tag_components)
{
    ASSERT(!m_logicFunctionDomain.has_value());

    if( !tag_components.empty() )
    {
        const std::wstring& logic_function_domain_text = tag_components.front();

        for( const auto& [declaration_text, symbol_type] : Symbol::GetDeclarationTextMap() )
        {
            if( logic_function_domain_text == declaration_text )
            {
                m_logicFunctionDomain = symbol_type;
                break;
            }
        }

        if( !m_logicFunctionDomain.has_value() )
        {
            if( logic_function_domain_text == AllSymbolsDomainText )
            {
                m_logicFunctionDomain = Logic::AllSymbolsDomain();
            }

            else if( logic_function_domain_text == CaseSymbolDomainText )
            {
                m_logicFunctionDomain = SymbolType::Pre80Dictionary;
            }

            else
            {
                throw CSProException(_T("The logic function domain '%s' is not a valid symbol type."), logic_function_domain_text.c_str());
            }
        }
    }

    return std::wstring();
}


std::wstring CSDocCompilerWorker::LogicEndHandler(const std::wstring& inner_text)
{
    return LogicEndHandlerWorker(TrimOnlyOneNewlineFromBothEnds(inner_text), HelpsHtmlProcessorMode::Normal);
}


std::wstring CSDocCompilerWorker::LogicSyntaxEndHandler(const std::wstring& inner_text)
{
    return LogicEndHandlerWorker(TrimOnlyOneNewlineFromBothEnds(inner_text), HelpsHtmlProcessorMode::Syntax);
}


std::wstring CSDocCompilerWorker::LogicColorEndHandler(const std::wstring& inner_text)
{
    return LogicEndHandlerWorker(Encoders::FromHtmlAmpersandEscapes(inner_text), HelpsHtmlProcessorMode::Inline);
}


std::wstring CSDocCompilerWorker::LogicEndHandlerWorker(std::wstring text, HelpsHtmlProcessorMode mode)
{
    HelpsHtmlProcessor html_processor(mode, m_settings, m_logicFunctionDomain);
    m_logicFunctionDomain.reset();

    ScintillaColorizer colorizer(SCLEX_CSPRO_LOGIC_V8_0, html_processor.PreprocessLogic(std::move(text)));
    return colorizer.GetHtml(&html_processor);
}


std::wstring CSDocCompilerWorker::LogicTableStartHandler(cs::span<const std::wstring> tag_components)
{
    int columns = 0;

    if( !TryParseInt(tag_components.front(), columns) || columns < 1 )
        throw CSProException("The number of columns in a table must be a positive integer.");

    // sort the words
    std::vector<std::wstring> reserved_words = Logic::ReservedWords::GetAllReservedWords();
    std::sort(reserved_words.begin(), reserved_words.end(),
              [&](const std::wstring& rw1, const std::wstring& rw2) { return ( SO::CompareNoCase(rw1, rw2) < 0 ); });

    const int rows = static_cast<int>(std::ceil(reserved_words.size() / static_cast<double>(columns)));

    std::wstring text = _T("<div align=\"center\"><table class=\"bordered_table\">");

    for( int r = 0; r < rows; ++r )
    {
        text.append(_T("<tr>"));

        for( int c = 0; c < columns; ++c )
        {
            text.append(_T("<td class=\"bordered_table_cell\">"));

            const size_t index = ( c * rows ) + r;

            if( index < reserved_words.size() )
                text.append(LogicEndHandlerWorker(reserved_words[index], HelpsHtmlProcessorMode::Inline));

            text.append(_T("</td>"));
        }

        text.append(_T("</tr>"));
    }

    text.append(_T("</table></div>"));

    return text;
}


std::wstring CSDocCompilerWorker::ActionEndHandler(const std::wstring& inner_text)
{
    // this subclass will remove the CS. from the output, and add the Async (as necessary)
    class ActionInvokerHelpsHtmlProcessor : public HelpsHtmlProcessor
    {
    public:
        ActionInvokerHelpsHtmlProcessor(const bool cs_was_specified, const bool async_was_specified, CSDocCompilerSettings& settings)
            :   HelpsHtmlProcessor(HelpsHtmlProcessorMode::Inline, settings, std::nullopt),
                m_csWasSpecified(cs_was_specified),
                m_asyncWasSpecified(async_was_specified)
        {
        }

    private:
        std::vector<ScintillaColorizer::ExtendedEntity> GetExtendedEntities(const std::vector<ScintillaColorizer::Entity>& entities) const override
        {
            std::vector<ScintillaColorizer::ExtendedEntity> extended_entities = __super::GetExtendedEntities(entities);

            if( extended_entities.size() < 2 ||
                extended_entities[0].text != _T("CS") ||
                extended_entities[1].text != _T(".") )
            {
                throw CSProException("Invalid 'action' tag");
            }

            if( !m_csWasSpecified )
                extended_entities.erase(extended_entities.begin(), extended_entities.begin() + 2);

            if( m_asyncWasSpecified )
                extended_entities.back().text.append(ActionInvoker::AsyncActionSuffix_sv);

            return extended_entities;
        }

    private:
        const bool m_csWasSpecified;
        const bool m_asyncWasSpecified;
    };

    std::wstring action_text = inner_text;

    const bool cs_was_specified = SO::StartsWith(inner_text, _T("CS."));

    if( !cs_was_specified )
        action_text.insert(0, _T("CS."));

    const bool async_was_specified = ( ActionInvoker::AsyncActionSuffix_sv.length() < action_text.length() &&
                                       std::wstring_view(action_text).substr(action_text.length() - ActionInvoker::AsyncActionSuffix_sv.length()) == ActionInvoker::AsyncActionSuffix_sv );

    if( async_was_specified )
        action_text.resize(action_text.length() - ActionInvoker::AsyncActionSuffix_sv.length());

    ActionInvokerHelpsHtmlProcessor html_processor(cs_was_specified, async_was_specified, m_settings);
    ScintillaColorizer colorizer(SCLEX_CSPRO_LOGIC_V8_0, action_text);
    return colorizer.GetHtml(&html_processor);
}


std::wstring CSDocCompilerWorker::MessageEndHandler(const std::wstring& inner_text)
{
    HelpsHtmlProcessor html_processor(HelpsHtmlProcessorMode::Normal);
    ScintillaColorizer colorizer(SCLEX_CSPRO_MESSAGE_V8_0, TrimOnlyOneNewlineFromBothEnds(inner_text));
    return colorizer.GetHtml(&html_processor);
}


std::wstring CSDocCompilerWorker::ReportStartHandler(cs::span<const std::wstring> tag_components)
{
    ASSERT(!m_lexerLanguage.has_value());

    if( tag_components.empty() )
    {
        m_lexerLanguage = SCLEX_CSPRO_REPORT_V8_0;
    }

    else if( tag_components.front() == _T("HTML") )
    {
        m_lexerLanguage = SCLEX_CSPRO_REPORT_HTML_V8_0;
    }

    else
    {
        throw CSProException(_T("Invalid 'report' tag: ") + tag_components.front());
    }

    return std::wstring();
}


std::wstring CSDocCompilerWorker::ReportEndHandler(const std::wstring& inner_text)
{
    ASSERT(m_lexerLanguage.has_value());

    const std::optional<SymbolType> domain_symbol_type = ( *m_lexerLanguage == SCLEX_CSPRO_REPORT_HTML_V8_0 ) ? std::make_optional(SymbolType::Report) :
                                                                                                                std::nullopt;

    HelpsHtmlProcessor html_processor(HelpsHtmlProcessorMode::Normal, m_settings, domain_symbol_type);
    ScintillaColorizer colorizer(*m_lexerLanguage, TrimOnlyOneNewlineFromBothEnds(inner_text));

    m_lexerLanguage.reset();

    return colorizer.GetHtml(&html_processor);
}


std::wstring CSDocCompilerWorker::ColorStartHandler(cs::span<const std::wstring> tag_components)
{
    ASSERT(!m_lexerLanguage.has_value());

    const std::wstring& language_name = tag_components.front();

    m_lexerLanguage = ( language_name == _T("C++") )        ? SCLEX_CPP : 
                      ( language_name == _T("cspro_v0") )   ? SCLEX_CSPRO_LOGIC_V0 :
                      ( language_name == _T("HTML") )       ? SCLEX_HTML :
                      ( language_name == _T("JSON") )       ? SCLEX_JSON :
                      ( language_name == _T("JavaScript") ) ? SCLEX_JAVASCRIPT :
                      ( language_name == _T("message") )    ? SCLEX_CSPRO_MESSAGE_V8_0 :
                      ( language_name == _T("SQL") )        ? SCLEX_SQL :
                                                              throw CSProException(_T("Coloring the language '%s' is not supported."), language_name.c_str());

    return std::wstring();
}


std::wstring CSDocCompilerWorker::ColorEndHandler(const std::wstring& inner_text)
{
    return ColorEndHandlerWorker(inner_text, HelpsHtmlProcessorMode::Normal);
}


std::wstring CSDocCompilerWorker::ColorInlineEndHandler(const std::wstring& inner_text)
{
    return ColorEndHandlerWorker(inner_text, HelpsHtmlProcessorMode::Inline);
}


std::wstring CSDocCompilerWorker::ColorEndHandlerWorker(const std::wstring& inner_text, HelpsHtmlProcessorMode mode)
{
    ASSERT(m_lexerLanguage.has_value());

    // validate JSON
    if( *m_lexerLanguage == SCLEX_JSON )
        Json::Parse(inner_text);

    HelpsHtmlProcessor html_processor(mode);
    ScintillaColorizer colorizer(*m_lexerLanguage, TrimOnlyOneNewlineFromBothEnds(inner_text));

    m_lexerLanguage.reset();

    return colorizer.GetHtml(&html_processor);
}
