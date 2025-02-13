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

    void CompileSpec(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, SpecCompilationType spec_compilation_type);
    void CompileSpec(DocSetSpec& doc_set_spec, const std::wstring& text, SpecCompilationType spec_compilation_type);

    void CompileSpecIfNecessary(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type);

    void CompileDocumentsNode(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node);

    std::optional<DocSetTableOfContents> CompileTableOfContents(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles);

    std::optional<DocSetIndex> CompileIndex(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles);

    void CompileSettings(const JsonNode<wchar_t>& json_node, DocSetSettings& doc_set_settings, bool reset_settings);

    void CompileDefinitions(const JsonNode<wchar_t>& json_node, std::vector<std::tuple<std::wstring, std::wstring>>& definitions);

    void CompileContextIds(const std::wstring& resource_file_text, std::map<std::wstring, unsigned>& context_ids);

    static DocSetSettings GetSettingsFromSpecOrSettingsFile(const std::wstring& filename, ErrorIssuerType error_issuer = ThrowErrors { });

private:
    void AddErrorOrWarning(bool error, const std::wstring& text);
    void AddError(const std::wstring& text)   { AddErrorOrWarning(true, text); }
    void AddWarning(const std::wstring& text) { AddErrorOrWarning(false, text); }

    using GetFileTextOrModifiedIterationCallback = std::function<const std::tuple<std::wstring, int64_t>*(const std::wstring&)>;
    static RAII::PushOnVectorAndPopOnDestruction<GetFileTextOrModifiedIterationCallback> OverrideGetFileTextOrModifiedIteration(GetFileTextOrModifiedIterationCallback callback);

    template<typename T>
    static T GetFileTextOrModifiedIteration(const std::wstring& filename);

    struct ComponentText;
    ComponentText GetComponentText(const std::wstring& filename);

    bool SpecRequiresCompilation(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type) const;

    static bool JsonNodeContainsAndIsNotNull(const JsonNode<wchar_t>& json_node, std::wstring_view key_sv);

    bool EnsureJsonNodeIsObject(const JsonNode<wchar_t>& json_node, const TCHAR* type);
    bool EnsureJsonNodeIsArray(const JsonNode<wchar_t>& json_node, const TCHAR* type);

    std::wstring JsonNodeGetStringWithWhitespaceCheck(const JsonNode<wchar_t>& json_node, const TCHAR* key);

    std::shared_ptr<DocSetComponent> JsonNodeGetDocumentWithSupportForFilenameOnly(const JsonNode<wchar_t>& json_node, DocSetSpec& doc_set_spec,
                                                                                   bool create_component_if_not_found, const TCHAR* document_type_for_error);

    // null nodes are not processed
    template<typename CF>
    void JsonNodeForeachNode(const JsonNode<wchar_t>& json_node, const TCHAR* type, bool nodes_cannot_be_arrays_or_objects, CF callback_function);

    bool CheckIfComponentsDoesNotContain(const DocSetSpec& doc_set_spec, DocSetComponent::Type doc_set_component_type);

    bool CheckFileValidity(const DocSetComponent& doc_set_component, bool error_if_file_does_not_exist = true);

    bool CheckIfDirectoryExists(const TCHAR* type, const std::wstring& path);

    bool CheckIfOverridesWithNewValue(const TCHAR* type, const std::wstring& new_value, const std::wstring& old_value);

    bool AddComponent(DocSetSpec& doc_set_spec, std::shared_ptr<DocSetComponent> doc_set_component, bool error_if_file_does_not_exist = true);

    static std::tuple<std::wstring, bool> GetPathWithRecursiveOption(const JsonNode<wchar_t>& json_node);

    void ProcessContextIdsNode(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node);

private:
    const GlobalSettings* m_globalSettings;
    ErrorIssuerType m_errorIssuer;

    struct ComponentText
    {
        RAII::PushOnVectorAndPopOnDestruction<std::wstring> filename_holder;
        std::wstring text;
        JsonReaderInterface json_reader_interface;

        JsonNode<wchar_t> GetJsonNode();
    };

    std::vector<std::wstring> m_componentTextFilenames;

    static std::vector<GetFileTextOrModifiedIterationCallback> m_getFileTextOrModifiedIterationCallbacks;
};
