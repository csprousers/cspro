#pragma once

#include <CSDocument/DocSetComponent.h>
#include <CSDocument/DocSetIndex.h>
#include <CSDocument/DocSetSettings.h>
#include <CSDocument/DocSetTableOfContents.h>


class DocSetSpec
{
    friend class DocSetCompiler;

public:
    DocSetSpec();
    explicit DocSetSpec(std::string file_path);

    void Reset();

    const std::string& GetFilePath() const { return m_filePath; }

    // component routines
    const std::vector<std::shared_ptr<DocSetComponent>>& GetComponents() const { return m_components; }

    const std::map<std::string, std::vector<std::shared_ptr<DocSetComponent>>, cs::case_insensitive_less>& GetFilenameDocumentMap() const { return m_filenameDocumentMap; }

    void AddComponent(std::shared_ptr<DocSetComponent> doc_set_component);

    std::shared_ptr<DocSetComponent> FindComponent(const std::string& file_path, bool search_special_documents) const; // searches by full path
    const std::vector<std::shared_ptr<DocSetComponent>>* FindDocument(const std::string& filename) const; // searches by filename only

    // object access
    const std::optional<std::string>& GetTitle() const { return m_title; }
    std::string GetTitleOrFilenameWithoutExtension() const;

    std::shared_ptr<DocSetComponent> GetCoverPageDocument() const { return m_coverPageDocument; }

    std::shared_ptr<DocSetComponent> GetDefaultDocument() const { return m_defaultDocument; }

    const std::optional<DocSetTableOfContents>& GetTableOfContents() const { return m_tableOfContents; }

    const std::optional<DocSetIndex>& GetIndex() const { return m_index; }

    const DocSetSettings& GetSettings() const { return m_settings; }

    const std::vector<std::tuple<std::string, std::string>>& GetDefinitions() const { return m_definitions; }

    const std::map<std::string, unsigned>& GetContextIds() const { return m_contextIds; }

    // JSON writing routines
    void WriteJsonSpecOnly(JsonWriter& json_writer, const JsonNode* json_node_to_copy_documents_node,
                           const GlobalSettings* global_settings, bool detailed_format);

    static void WriteJsonDefinitions(JsonWriter& json_writer, const std::vector<std::tuple<std::string, std::string>>& definitions, bool write_as_object);

    static void WriteJsonContextIds(JsonWriter& json_writer, const std::map<std::string, unsigned>& context_ids);

    void WriteDocumentPathWithFilenameOnlyWhenPossible(JsonWriter& json_writer, const std::string& path, bool documents_with_filename_must_come_from_documents_node);

    static void WriteNewDocumentSetShell(const std::string& file_path);

private:
    std::string m_filePath;

    std::vector<std::shared_ptr<DocSetComponent>> m_components;
    std::map<std::string, std::vector<std::shared_ptr<DocSetComponent>>, cs::case_insensitive_less> m_filenameDocumentMap;

    std::optional<std::string> m_title;
    std::shared_ptr<DocSetComponent> m_coverPageDocument;
    std::shared_ptr<DocSetComponent> m_defaultDocument;
    std::optional<DocSetTableOfContents> m_tableOfContents;
    std::optional<DocSetIndex> m_index;
    DocSetSettings m_settings;
    std::vector<std::tuple<std::string, std::string>> m_definitions;
    std::map<std::string, unsigned> m_contextIds;

    std::optional<std::tuple<size_t, int, int64_t>> m_lastCompilationDetails;
};
