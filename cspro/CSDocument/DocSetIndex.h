#pragma once

class DocSetSpec;
class DocSetCompiler;


class DocSetIndex
{
    class CompileWorker;

public:
    void SortByTitle(DocSetSpec& doc_set_spec);

    // compilation and serialization
    static std::optional<DocSetIndex> Compile(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles);

    class Writer;
    void WriteJson(JsonWriter& json_writer, DocSetSpec* doc_set_spec = nullptr,
                   bool write_documents_with_filename_only_when_possible = false,
                   bool write_evaluated_titles = false, bool detailed_format = false) const;

private:
    struct Entry
    {
        std::shared_ptr<DocSetComponent> document;
        std::wstring title_override;
        std::vector<Entry> subentries;
    };

private:
    static std::vector<Entry> CompileEntries(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, size_t entries_level);

private:
    std::vector<Entry> m_entries;
};



// --------------------------------------------------------------------------
// DocSetIndex::Writer
// --------------------------------------------------------------------------

class DocSetIndex::Writer
{
public:
    virtual ~Writer() { }

    void Write(const DocSetIndex& index);

protected:
    void WriteSubentries(const void* subentries_tag);

    virtual void StartWriting() { } 
    virtual void FinishWriting() { } 

    virtual void StartEntries() { }
    virtual void WriteEntry(const std::wstring& csdoc_filename, const std::wstring* title_override, const void* subentries_tag) { csdoc_filename; title_override; subentries_tag; }
    virtual void FinishEntries() { }

private:
    void WriteEntries(const std::vector<Entry>& entries);
};
