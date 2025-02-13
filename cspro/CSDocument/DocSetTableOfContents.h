#pragma once

class DocSetSpec;
class DocSetCompiler;


// --------------------------------------------------------------------------
// DocSetTableOfContents
// --------------------------------------------------------------------------

class DocSetTableOfContents
{
    class CompileWorker;

public:
    bool operator==(const DocSetTableOfContents& rhs) const;
    bool operator!=(const DocSetTableOfContents& rhs) const { return !operator==(rhs); }

    size_t GetPosition(const DocSetComponent& doc_set_component) const; // returns SIZE_MAX if not found

    const DocSetComponent* GetDocumentByPosition(size_t position) const; // returns null if invalid

    // compilation and serialization
    static std::optional<DocSetTableOfContents> Compile(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles);
    
    class Writer;
    void WriteJson(JsonWriter& json_writer, DocSetSpec* doc_set_spec = nullptr,
                   bool write_documents_with_filename_only_when_possible = false,
                   bool write_evaluated_titles = false, bool detailed_format = false) const;

private:
    struct Chapter
    {
        std::wstring title;
        bool write_title_to_pdf;
        std::vector<std::shared_ptr<DocSetComponent>> documents;
        std::vector<Chapter> chapters;
    };

    struct ProjectLink
    {
        std::wstring project;
    };

private:
    const Chapter* GetRootChapter() const;

private:
    std::vector<std::variant<Chapter, ProjectLink>> m_nodes;
    std::map<std::wstring, std::wstring> m_documentTitleOverrides;
    std::vector<std::shared_ptr<DocSetComponent>> m_documentsInOrder;
};



// --------------------------------------------------------------------------
// DocSetTableOfContents::Writer
// --------------------------------------------------------------------------

class DocSetTableOfContents::Writer
{
public:
    virtual ~Writer() { }

    void Write(const DocSetTableOfContents& table_of_contents);

protected:
    virtual void StartWriting(size_t num_root_nodes) { num_root_nodes; } 
    virtual void FinishWriting() { } 

    virtual void WriteProject(const std::wstring& project) { project; }

    virtual void StartChapter(const std::wstring& title, bool write_title_to_pdf) { title; write_title_to_pdf; }
    virtual void FinishChapter() { }

    virtual void StartDocuments() { }
    virtual void WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override) { csdoc_filename; title_override; }
    virtual void FinishDocuments() { }

    virtual void StartSubchapters() { }
    virtual void FinishSubchapters() { }

private:
    void WriteChapter(const Chapter& chapter);

private:
    const std::map<std::wstring, std::wstring>* m_documentTitleOverrides = nullptr;
};
