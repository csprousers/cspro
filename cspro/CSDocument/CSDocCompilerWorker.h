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
    CSDocCompilerWorker(CSDocCompilerSettings& settings, std::string_view text_sv);

    std::string CreateHtml();

    static std::string CreateHyperlinkStart(std::string_view CreateUrlForFile_result_sv, bool target_blank = false, bool end_tag = true);

    static std::string EvaluateAndCreateUrlForTopicComponent(CSDocCompilerSettings& settings, const std::string& topic_component);

private:
    struct SD;
    struct TagDefinition;
    struct TableSettings;

    static const SD& GetStaticData();

    // Returns the positions of the first tag's start and end characters (< and >).
    // If no tag exists, end will be std::string_view::npos.
    struct TagPosition { size_t start; size_t end; };
    TagPosition GetTagPosition(std::string_view text_sv, size_t start_pos = 0);

    // Splits the components of the tag.
    std::vector<std::string> GetTagComponents(std::string_view text_sv, const TagPosition& tag_position) const;
    static std::vector<std::string> GetTagComponentsV0(std::string_view tag_sv);
    static std::vector<std::string> GetTagComponentsV8(std::string_view tag_name_sv, std::string_view rest_of_tag_sv);

    // Converts tag components into a map for version 8 processing.
    std::map<std::string_view, std::string_view> ProcessTagComponentsV8(cs::span<const std::string> tag_components);

    // Validates the attribute names of version 8 tags.
    void ValidateTagComponentsV8(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components,
                                 cs::span<const std::string_view> required_attribute_names,
                                 cs::span<const std::string_view> optional_attribute_names = cs::span<const std::string_view>(),
                                 cs::span<const std::string_view> attribute_names_where_value_can_be_blank = cs::span<const std::string_view>());

    // Runs the callback if the attribute is defined.
    template<typename CF>
    static void ExecuteWithTagValue(const std::map<std::string_view, std::string_view>& tag_components, std::string_view attribute_name_sv, CF callback_function);

    // Replaces definitions and inserts the text for any include tags.
    std::string PreprocessTextForDefinitionsAndIncludes(std::string_view text_sv);

    // Converts preprocessed text to paragraphs.
    void CreateParagraphsFromPreprocessedText(const std::string& preprocessed_text);

    // Processes a paragraph.
    void ProcessParagraph(std::string paragraph);

    // Replaces newline markers in the text with break tags.
    static std::string& ReplaceNewlinesWithBreaks(std::string& text);

    // Processes text, removing from the argument the part that has been processed.
    std::string ProcessText(std::string& text);

    // Starts and ends the processing of the components in a tag.
    std::string StartTagV0(const TagDefinition& tag_definition, cs::span<const std::string> tag_components);
    std::string EndTag(const TagDefinition& tag_definition, const std::string& inner_text);


    // --------------------------------------------------------------------------
    // parsers
    // --------------------------------------------------------------------------

    static std::string ParseStringLiteral(std::string_view text_sv, size_t& pos);
    static PortableColor ParsePortableColor(std::string_view tag_or_attribute_name_sv, bool first_argument_is_tag, const std::string& text);
    static int ParseInt(std::string_view attribute_name_sv, std::string_view text_sv);
    static int ParseInt(std::string_view attribute_name_sv, std::string_view text_sv, int min_value);
    static bool TryParseInt(std::string_view text_sv, int& value);
    static bool TryParseDouble(std::string_view text_sv, double& value);


    // --------------------------------------------------------------------------
    // HTML helpers
    // --------------------------------------------------------------------------

    static void AppendImageWidthHeight(std::string& html, const std::optional<int> width, const std::optional<int> height, bool close_tag);

    // Creates the HTML for the title and updates the database of titles.
    std::string CreateTitleHtml(std::string raw_title, const std::string& title_html);

    // Evaluates the image path and returns the 'img' tag HTML with the 'src' attribute set and the 'alt' attribute started.
    std::string CreateImageStartHtml(const std::string& image_path);

    // Evaluates the URL and returns the 'a' tag HTML with the 'href' attribute set.
    std::string CreateLinkStartHtml(std::string url, bool end_tag);


    // --------------------------------------------------------------------------
    // tag handlers
    // --------------------------------------------------------------------------

    std::string EndTagWithContentsOfTextStack(const std::string& inner_text);

    std::string TitleStartHandler(cs::span<const std::string> tag_components);
    std::string TitleEndHandler(const std::string& inner_text);

    std::string ContextStartHandler(cs::span<const std::string> tag_components);

    std::string IndentStartHandler(cs::span<const std::string> tag_components);

    std::string FontStartHandler(cs::span<const std::string> tag_components);

    std::string ListStartHandler(cs::span<const std::string> tag_components);

    std::string ImageStartHandler(cs::span<const std::string> tag_components);

    std::string BarcodeStartHandler(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components);

    struct PathAndProject { std::string path; std::string project; };
    static PathAndProject GetPathAndProjectForTopicComponent(CSDocCompilerSettings& settings, const std::string& topic_component);
    PathAndProject GetPathAndProjectForTopicComponent(const std::string& topic_component) { return GetPathAndProjectForTopicComponent(m_settings, topic_component); }

    std::string TopicStartHandler(cs::span<const std::string> tag_components);
    std::string LinkStartHandler(cs::span<const std::string> tag_components);
    std::string SeeAlsoStartHandler(cs::span<const std::string> tag_components);
    std::string ResourceStartHandler(cs::span<const std::string> tag_components);

    TableSettings& GetCurrentTable();
    std::string TableStartHandler(cs::span<const std::string> tag_components);
    std::string TableEndHandler(const std::string& inner_text);
    std::string TableCellStartHandler(cs::span<const std::string> tag_components);
    std::string TableCellEndHandler(const std::string& inner_text);

    std::string NoteStartHandler(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components);

    std::string MetadataStartHandler(const std::string& start_tag, const std::map<std::string_view, std::string_view>& tag_components);

    std::string BuildExtraStartHandler(cs::span<const std::string> tag_components);


    // --------------------------------------------------------------------------
    // colorizer tag handlers
    // --------------------------------------------------------------------------

    static std::string TrimOnlyOneNewlineFromBothEnds(const std::string& text);

    std::string LogicObjectStartHandler(cs::span<const std::string> tag_components);
    std::string LogicEndHandler(const std::string& inner_text);
    std::string LogicSyntaxEndHandler(const std::string& inner_text);
    std::string LogicColorEndHandler(const std::string& inner_text);
    std::string LogicEndHandlerWorker(std::string text, HelpsHtmlProcessorMode mode);
    std::string LogicTableStartHandler(cs::span<const std::string> tag_components);

    std::string ActionEndHandler(const std::string& inner_text);

    std::string MessageEndHandler(const std::string& inner_text);

    std::string ReportStartHandler(cs::span<const std::string> tag_components);
    std::string ReportEndHandler(const std::string& inner_text);

    std::string ColorStartHandler(cs::span<const std::string> tag_components);
    std::string ColorEndHandler(const std::string& inner_text);
    std::string ColorInlineEndHandler(const std::string& inner_text);
    std::string ColorTagEndHandler(const std::string& inner_text);
    std::string ColorEndHandlerWorker(const std::string& inner_text, HelpsHtmlProcessorMode mode);

    std::string PffEndHandler(const std::string& inner_text);
    std::string PffColorEndHandler(const std::string& inner_text);


    // --------------------------------------------------------------------------
    // other handlers
    // --------------------------------------------------------------------------

    class MarkdownCreator;
    std::string MarkdownEndHandler(const std::string& inner_text);


private:
    const SD& m_sd;
    CSDocCompilerSettings& m_settings;
    std::vector<std::string> m_paragraphs;
    std::string m_html;

    std::stack<std::string> m_tagStack;
    bool m_inBlockTag;
    std::stack<std::string> m_endTagTextStack;
    std::optional<std::string> m_title;
    std::stack<std::shared_ptr<TableSettings>> m_tableStack;
    std::optional<Logic::FunctionDomain> m_logicFunctionDomain;
    std::optional<int> m_lexerLanguage;

    struct TagDefinition
    {
        using StartHandlerFunctionV0 = std::string (CSDocCompilerWorker::*)(cs::span<const std::string>);
        using StartHandlerFunctionV8 = std::string (CSDocCompilerWorker::*)(const std::string&, const std::map<std::string_view, std::string_view>&);
        using StartHandler = std::variant<std::monostate, std::string, StartHandlerFunctionV0, StartHandlerFunctionV8>;
        using EndHandlerFunction = std::string (CSDocCompilerWorker::*)(const std::string&);
        using EndHandler = std::variant<std::monostate, std::string, EndHandlerFunction>;

        bool paired;
        StartHandler start_handler;
        EndHandler end_handler;
        size_t min_components = 0;
        size_t max_components = 0;
    };

    struct SD // static data
    {
        std::map<std::string_view, TagDefinition> tag_definitions;
        std::set<std::string> block_tags;
        std::regex block_tag_match_regex;
    };
};
