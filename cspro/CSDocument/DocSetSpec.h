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
    explicit DocSetSpec(std::wstring filename);

    void Reset();

    const std::wstring& GetFilename() const { return m_filename; }

    // component routines
    const std::vector<std::shared_ptr<DocSetComponent>>& GetComponents() const { return m_components; }

    const std::map<StringNoCase, std::vector<std::shared_ptr<DocSetComponent>>>& GetFilenameDocumentMap() const { return m_filenameDocumentMap; }

    void AddComponent(std::shared_ptr<DocSetComponent> doc_set_component);

    std::shared_ptr<DocSetComponent> FindComponent(const std::wstring& filename, bool search_special_documents) const; // searches by full path
    const std::vector<std::shared_ptr<DocSetComponent>>* FindDocument(const std::wstring& filename) const; // searches by filename only

    // object access
    const std::optional<std::wstring>& GetTitle() const { return m_title; }
    std::wstring GetTitleOrFilenameWithoutExtension() const;

    std::shared_ptr<DocSetComponent> GetCoverPageDocument() const { return m_coverPageDocument; }

    std::shared_ptr<DocSetComponent> GetDefaultDocument() const { return m_defaultDocument; }

    const std::optional<DocSetTableOfContents>& GetTableOfContents() const { return m_tableOfContents; }

    const std::optional<DocSetIndex>& GetIndex() const { return m_index; }

    const DocSetSettings& GetSettings() const { return m_settings; }

    const std::vector<std::tuple<std::wstring, std::wstring>>& GetDefinitions() const { return m_definitions; }

    const std::map<std::wstring, unsigned>& GetContextIds() const { return m_contextIds; }

    // JSON writing routines
    void WriteJsonSpecOnly(JsonWriter& json_writer, const JsonNode<wchar_t>* json_node_to_copy_documents_node,
                           const GlobalSettings* global_settings, bool detailed_format);

    static void WriteJsonDefinitions(JsonWriter& json_writer, const std::vector<std::tuple<std::wstring, std::wstring>>& definitions, bool write_as_object);

    static void WriteJsonContextIds(JsonWriter& json_writer, const std::map<std::wstring, unsigned>& context_ids);

    void WriteDocumentPathWithFilenameOnlyWhenPossible(JsonWriter& json_writer, const std::wstring& path, bool documents_with_filename_must_come_from_documents_node);

    static void WriteNewDocumentSetShell(const std::wstring& filename);

private:
    const std::wstring m_filename;

    std::vector<std::shared_ptr<DocSetComponent>> m_components;
    std::map<StringNoCase, std::vector<std::shared_ptr<DocSetComponent>>> m_filenameDocumentMap;

    std::optional<std::wstring> m_title;
    std::shared_ptr<DocSetComponent> m_coverPageDocument;
    std::shared_ptr<DocSetComponent> m_defaultDocument;
    std::optional<DocSetTableOfContents> m_tableOfContents;
    std::optional<DocSetIndex> m_index;
    DocSetSettings m_settings;
    std::vector<std::tuple<std::wstring, std::wstring>> m_definitions;
    std::map<std::wstring, unsigned> m_contextIds;

    std::optional<std::tuple<size_t, int, int64_t>> m_lastCompilationDetails;
};
