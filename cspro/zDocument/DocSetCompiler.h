#pragma once

#include <CSDocument/DocSetSpec.h>


class DocSetCompiler
{
    friend class DocBuildSettings;
    friend class DocSetBuilderBaseGenerateTask;
    friend class DocSetIndex;
    friend class DocSetIndexTableOfContentsBaseCompileWorker;
    friend class DocSetSettings;
    friend class DocSetTableOfContents;

public:
    struct SuppressErrors { };
    struct ThrowErrors { };
    using ErrorIssuerType = std::variant<SuppressErrors, ThrowErrors, BuildWnd*>;

    DocSetCompiler(const GlobalSettings& global_settings, ErrorIssuerType error_issuer);
    DocSetCompiler(ErrorIssuerType error_issuer);

    enum class SpecCompilationType { SettingsOnly, DocumentsNodeAndSettingsOnly, DataForTree, DataForCSDocCompilation, SpecOnly, SpecAndComponents };

    void CompileSpec(DocSetSpec& doc_set_spec, const JsonNode& json_node, SpecCompilationType spec_compilation_type);
    void CompileSpec(DocSetSpec& doc_set_spec, const std::string& text, SpecCompilationType spec_compilation_type);

    void CompileSpecIfNecessary(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type);

    void CompileDocumentsNode(DocSetSpec& doc_set_spec, const JsonNode& json_node);

    std::optional<DocSetTableOfContents> CompileTableOfContents(DocSetSpec& doc_set_spec, const JsonNode& json_node, bool validate_titles);

    std::optional<DocSetIndex> CompileIndex(DocSetSpec& doc_set_spec, const JsonNode& json_node, bool validate_titles);

    void CompileSettings(const JsonNode& json_node, DocSetSettings& doc_set_settings, bool reset_settings);

    void CompileDefinitions(const JsonNode& json_node, std::vector<std::tuple<std::string, std::string>>& definitions);

    void CompileContextIds(const std::string& resource_file_text, std::map<std::string, unsigned>& context_ids);

    static DocSetSettings GetSettingsFromSpecOrSettingsFile(const std::string& file_path, ErrorIssuerType error_issuer = ThrowErrors { });

private:
    void AddErrorOrWarning(bool error, const std::string& text);

    template<typename... Args>
    void AddError(const char* formatter, Args const&... args);

    template<typename... Args>
    void AddWarning(const char* formatter, Args const&... args);

    using GetFileTextOrModifiedIterationCallback = std::function<const std::tuple<SharableString, int64_t>*(const std::string&)>;
    static RAII::PushOnVectorAndPopOnDestruction<GetFileTextOrModifiedIterationCallback> OverrideGetFileTextOrModifiedIteration(GetFileTextOrModifiedIterationCallback callback);

    template<typename T>
    static T GetFileTextOrModifiedIteration(const std::string& file_path);

    struct ComponentText;
    ComponentText GetComponentText(const std::string& file_path);

    bool SpecRequiresCompilation(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type) const;

    static bool JsonNodeContainsAndIsNotNull(const JsonNode& json_node, std::string_view key_sv);

    bool EnsureJsonNodeIsObject(const JsonNode& json_node, const char* type);
    bool EnsureJsonNodeIsArray(const JsonNode& json_node, const char* type);

    std::string JsonNodeGetStringWithWhitespaceCheck(const JsonNode& json_node, const char* key);

    std::shared_ptr<DocSetComponent> JsonNodeGetDocumentWithSupportForFilenameOnly(const JsonNode& json_node, DocSetSpec& doc_set_spec,
                                                                                   bool create_component_if_not_found, const char* document_type_for_error);

    // null nodes are not processed
    template<typename CF>
    void JsonNodeForeachNode(const JsonNode& json_node, const char* type, bool nodes_cannot_be_arrays_or_objects, CF callback_function);

    bool CheckIfComponentsDoesNotContain(const DocSetSpec& doc_set_spec, DocSetComponent::Type doc_set_component_type);

    bool CheckFileValidity(const DocSetComponent& doc_set_component, bool error_if_file_does_not_exist = true);

    bool CheckIfDirectoryExists(const char* type, const std::string& path);

    bool CheckIfOverridesWithNewValue(const char* type, const std::string& new_value, const std::string& old_value);

    bool AddComponent(DocSetSpec& doc_set_spec, std::shared_ptr<DocSetComponent> doc_set_component, bool error_if_file_does_not_exist = true);

    static std::tuple<std::string, bool> GetPathWithRecursiveOption(const JsonNode& json_node);

    void ProcessContextIdsNode(DocSetSpec& doc_set_spec, const JsonNode& json_node);

private:
    const GlobalSettings* m_globalSettings;
    ErrorIssuerType m_errorIssuer;

    struct ComponentText
    {
        RAII::PushOnVectorAndPopOnDestruction<std::string> file_path_holder;
        SharableString text;
        JsonReaderInterface json_reader_interface;

        JsonNode GetJsonNode();
    };

    std::vector<std::string> m_componentTextFilePaths;

    static std::vector<GetFileTextOrModifiedIterationCallback> m_getFileTextOrModifiedIterationCallbacks;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void DocSetCompiler::AddError(const char* const formatter, Args const&... args)
{
    AddErrorOrWarning(true, FormatText(formatter, args...));
}


template<typename... Args>
void DocSetCompiler::AddWarning(const char* const formatter, Args const&... args)
{
    AddErrorOrWarning(false, FormatText(formatter, args...));
}
