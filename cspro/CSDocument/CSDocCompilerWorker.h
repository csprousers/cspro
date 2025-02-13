#pragma once

#include <CSDocument/CSDocCompilerSettings.h>
#include <zToolsO/span.h>
#include <zLogicO/FunctionTable.h>
#include <regex>

enum class HelpsHtmlProcessorMode;
class PortableColor;
enum class SymbolType;


class CSDocCompilerWorker
{
public:
    CSDocCompilerWorker(CSDocCompilerSettings& settings, wstring_view text_sv);

    std::wstring CreateHtml();

    static std::wstring CreateHyperlinkStart(wstring_view CreateUrlForFile_result_sv, bool target_blank = false, bool end_tag = true);

    static std::wstring EvaluateAndCreateUrlForTopicComponent(CSDocCompilerSettings& settings, const std::wstring& topic_component);

private:
    struct SD;
    struct TagDefinition;
    struct TableSettings;

    static const SD& GetStaticData();

    // returns the positions of the first tag's start and end characters (< and >);
    // if no tag exists, end will be wstring_view::npos
    struct TagPosition { size_t start; size_t end; };
    TagPosition GetTagPosition(wstring_view text_sv, size_t start_pos = 0);

    // splits the components of the tag
    std::vector<std::wstring> GetTagComponents(wstring_view text_sv, const TagPosition& tag_position) const;
    static std::vector<std::wstring> GetTagComponentsV0(wstring_view tag_sv);
    static std::vector<std::wstring> GetTagComponentsV8(wstring_view tag_name_sv, wstring_view rest_of_tag_sv);

    // converts tag components into a map for version 8 processing
    std::map<wstring_view, wstring_view> ProcessTagComponentsV8(cs::span<const std::wstring> tag_components);

    // validates the attribute names of version 8 tags
    void ValidateTagComponentsV8(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components,
                                 cs::span<const wstring_view> required_attribute_names,
                                 cs::span<const wstring_view> optional_attribute_names = cs::span<const wstring_view>(),
                                 cs::span<const wstring_view> attribute_names_where_value_can_be_blank = cs::span<const wstring_view>());

    // runs the callback if the attribute is defined
    template<typename CF>
    static void ExecuteWithTagValue(const std::map<wstring_view, wstring_view>& tag_components, wstring_view attribute_name_sv, CF callback_function);

    // replaces definitions and inserts the text for any include tags
    std::wstring PreprocessTextForDefinitionsAndIncludes(wstring_view text_sv);

    // converts preprocessed text to paragraphs
    void CreateParagraphsFromPreprocessedText(const std::wstring& preprocessed_text);

    // processes a paragraph
    void ProcessParagraph(std::wstring paragraph);

    // replaces newline markers in the text with break tags
    static std::wstring ReplaceNewlinesWithBreaks(std::wstring text);

    // processes text, removing from the argument the part that has been processed
    std::wstring ProcessText(std::wstring& text);

    // starts and ends the processing of the components in a tag
    std::wstring StartTagV0(const TagDefinition& tag_definition, cs::span<const std::wstring> tag_components);
    std::wstring StartTagV8(const TagDefinition& tag_definition, cs::span<const std::wstring> tag_components);
    std::wstring EndTag(const TagDefinition& tag_definition, const std::wstring& inner_text);

    // parsers
    static std::wstring ParseStringLiteral(wstring_view text_sv, size_t& pos);
    static PortableColor ParsePortableColor(wstring_view tag_or_attribute_name_sv, bool first_argument_is_tag, const std::wstring& text);
    static int ParseInt(wstring_view attribute_name_sv, wstring_view text_sv);
    static int ParseInt(wstring_view attribute_name_sv, wstring_view text_sv, int min_value);
    static bool TryParseInt(wstring_view text_sv, int& value);
    static bool TryParseDouble(wstring_view text_sv, double& value);

    // HTML helpers
    static void AppendImageWidthHeight(std::wstring& html, const std::optional<int> width, const std::optional<int> height, bool close_tag);

    // tag handlers
    std::wstring EndTagWithContentsOfTextStack(const std::wstring& inner_text);

    std::wstring TitleStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring TitleEndHandler(const std::wstring& inner_text);

    std::wstring ContextStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring IndentStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring FontStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring ListStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring ImageStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring BarcodeStartHandler(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components);

    struct PathAndProject { std::wstring path; std::wstring project; };
    static PathAndProject GetPathAndProjectForTopicComponent(CSDocCompilerSettings& settings, const std::wstring& topic_component);
    PathAndProject GetPathAndProjectForTopicComponent(const std::wstring& topic_component) { return GetPathAndProjectForTopicComponent(m_settings, topic_component); }    

    std::wstring TopicStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring LinkStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring SeeAlsoStartHandler(cs::span<const std::wstring> tag_components);

    TableSettings& GetCurrentTable();
    std::wstring TableStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring TableEndHandler(const std::wstring& inner_text);
    std::wstring TableCellStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring TableCellEndHandler(const std::wstring& inner_text);

    std::wstring NoteStartHandler(const std::wstring& start_tag, const std::map<wstring_view, wstring_view>& tag_components);

    std::wstring BuildExtraStartHandler(cs::span<const std::wstring> tag_components);

    // colorizer tag handlers
    static std::wstring TrimOnlyOneNewlineFromBothEnds(const std::wstring& text);

    std::wstring LogicObjectStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring LogicEndHandler(const std::wstring& inner_text);
    std::wstring LogicSyntaxEndHandler(const std::wstring& inner_text);
    std::wstring LogicColorEndHandler(const std::wstring& inner_text);
    std::wstring LogicEndHandlerWorker(std::wstring text, HelpsHtmlProcessorMode mode);
    std::wstring LogicTableStartHandler(cs::span<const std::wstring> tag_components);

    std::wstring ActionEndHandler(const std::wstring& inner_text);

    std::wstring MessageEndHandler(const std::wstring& inner_text);

    std::wstring ReportStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring ReportEndHandler(const std::wstring& inner_text);

    std::wstring ColorStartHandler(cs::span<const std::wstring> tag_components);
    std::wstring ColorEndHandler(const std::wstring& inner_text);
    std::wstring ColorInlineEndHandler(const std::wstring& inner_text);
    std::wstring ColorEndHandlerWorker(const std::wstring& inner_text, HelpsHtmlProcessorMode mode);

    std::wstring PffEndHandler(const std::wstring& inner_text);
    std::wstring PffColorEndHandler(const std::wstring& inner_text);

private:
    const SD& m_sd;
    CSDocCompilerSettings& m_settings;
    std::vector<std::wstring> m_paragraphs;
    std::wstring m_html;

    std::stack<std::wstring> m_tagStack;
    bool m_inBlockTag;
    std::stack<std::wstring> m_endTagTextStack;
    std::optional<std::wstring> m_title;
    std::stack<std::shared_ptr<TableSettings>> m_tableStack;
    std::optional<Logic::FunctionDomain> m_logicFunctionDomain;
    std::optional<int> m_lexerLanguage;

    struct TagDefinition
    {
        using StartHandlerFunctionV0 = std::wstring (CSDocCompilerWorker::*)(cs::span<const std::wstring>);
        using StartHandlerFunctionV8 = std::wstring (CSDocCompilerWorker::*)(const std::wstring&, const std::map<wstring_view, wstring_view>&);
        using StartHandler = std::variant<std::monostate, std::wstring, StartHandlerFunctionV0, StartHandlerFunctionV8>;
        using EndHandlerFunction = std::wstring (CSDocCompilerWorker::*)(const std::wstring&);
        using EndHandler = std::variant<std::monostate, std::wstring, EndHandlerFunction>;

        bool paired;
        StartHandler start_handler;
        EndHandler end_handler;
        size_t min_components = 0;
        size_t max_components = 0;
    };

    struct SD // static data
    {
        std::map<std::wstring, TagDefinition> tag_definitions;
        std::set<std::wstring> block_tags;
        std::wregex block_tag_match_regex;
    };
};
