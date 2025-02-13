#include "StdAfx.h"
#include "DocSetIndex.h"
#include "DocSetIndexTableOfContentsBase.h"


namespace
{
    inline bool ChmIndexTextLess(const std::wstring& text1, const std::wstring& text2)
    {
        // the index appears to be sorted in a case insensitive way, with special handling for some characters
        constexpr std::wstring_view SpecialChars_sv = _T("~-!$%&*./:[^|+<=>");

        const TCHAR* text2_itr = text2.c_str();

        for( const TCHAR text1_ch : text1 )
        {
            const TCHAR text2_ch = *text2_itr;

            // if text2 is complete, is it less than text1
            if( text2_ch == 0 )
                return false;

            // see if the text is a special character
            const size_t text1_special_ch_pos = SpecialChars_sv.find(text1_ch);
            const size_t text2_special_ch_pos = SpecialChars_sv.find(text2_ch);

            if( text1_special_ch_pos != text2_special_ch_pos )
                return ( text1_special_ch_pos < text2_special_ch_pos );

            // compare the non-special character text
            const auto ch_diff = std::towupper(text1_ch) - std::towupper(text2_ch);

            if( ch_diff != 0 )
                return ( ch_diff < 0 );

            ++text2_itr;
        }

        // if text2 is not complete, it is not less than text1
        return ( *text2_itr != 0 );
    }
}


void DocSetIndex::SortByTitle(DocSetSpec& doc_set_spec)
{
    DocSetIndexTableOfContentsBaseTitleLookupWorker title_lookup_worker(doc_set_spec);

    auto sort_entries = [&](std::vector<Entry>& entries)
    {
        std::sort(entries.begin(), entries.end(),
            [&](const Entry& entry1, const Entry& entry2)
            {
                return ChmIndexTextLess(!entry1.title_override.empty() ? entry1.title_override : title_lookup_worker.GetTitle(entry1.document->filename),
                                        !entry2.title_override.empty() ? entry2.title_override : title_lookup_worker.GetTitle(entry2.document->filename));
            });
    };

    // sort the root entries
    sort_entries(m_entries);

    // sort subindices
    for( Entry& entry : m_entries )
    {
        if( !entry.subentries.empty() )
        {
            ASSERT(std::count_if(entry.subentries.cbegin(), entry.subentries.cend(),
                                 [&](const Entry& entry) { return !entry.subentries.empty(); }) == 0);

            sort_entries(entry.subentries);
        }
    }
}


// --------------------------------------------------------------------------
// DocSetIndex::CompileWorker
// --------------------------------------------------------------------------

class DocSetIndex::CompileWorker : public DocSetIndexTableOfContentsBaseCompileWorker
{
public:
    using DocSetIndexTableOfContentsBaseCompileWorker::DocSetIndexTableOfContentsBaseCompileWorker;

    std::optional<DocSetIndex> Compile(const JsonNode<wchar_t>& json_node);

private:
    std::vector<DocSetIndex::Entry> CompileEntries(const JsonNode<wchar_t>& json_node, size_t entries_level);

private:
    std::set<const DocSetComponent*> m_documentsInIndex;
};


std::optional<DocSetIndex> DocSetIndex::CompileWorker::Compile(const JsonNode<wchar_t>& json_node)
{
    DocSetIndex doc_set_index;
    doc_set_index.m_entries = CompileEntries(json_node, 0);

    if( doc_set_index.m_entries.empty() )
        return std::nullopt;

    // make sure that the index contains all documents specified in the components
    for( const DocSetComponent& doc_set_component : VI_V(m_docSetSpec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
        {
            if( m_documentsInIndex.find(&doc_set_component) == m_documentsInIndex.cend() )
            {
                m_docSetCompiler.AddError(FormatTextCS2WS(_T("The '%s' must contain an entry for: %s"),
                                                          ToString(DocSetComponent::Type::Index), doc_set_component.filename.c_str()));
            }
        }
    }

    return doc_set_index;
}


std::vector<DocSetIndex::Entry> DocSetIndex::CompileWorker::CompileEntries(const JsonNode<wchar_t>& json_node, size_t entries_level)
{
    std::vector<Entry> entries;

    if( entries_level >= 2 )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("An '%s' only supports one level of subindices per index entry."),
                                                  ToString(DocSetComponent::Type::Index)));
        return entries;
    }

    if( !json_node.IsArray() )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("An '%s' must be specified using an array."),
                                                  ToString(DocSetComponent::Type::Index)));
        return entries;
    }

    for( const auto& entry_node : json_node.GetArray() )
    {
        Entry entry;

        entry.document = entry_node.IsObject() ? m_docSetCompiler.JsonNodeGetDocumentWithSupportForFilenameOnly(entry_node.Get(JK::path), m_docSetSpec, false, ToString(DocSetComponent::Type::Index)) :
                                                 m_docSetCompiler.JsonNodeGetDocumentWithSupportForFilenameOnly(entry_node, m_docSetSpec, false, ToString(DocSetComponent::Type::Index));

        if( entry.document == nullptr )
            continue;

        ASSERT(PortableFunctions::FileIsRegular(entry.document->filename) && entry.document->type == DocSetComponent::Type::Document);

        if( entry_node.IsObject() )
        {
            if( entry_node.Contains(JK::title) )
                entry.title_override = m_docSetCompiler.JsonNodeGetStringWithWhitespaceCheck(entry_node, JK::title);

            if( entry_node.Contains(JK::subindex) )
                entry.subentries = CompileEntries(entry_node.Get(JK::subindex), entries_level + 1);
        }

        // make sure the document has a title if the title was not overriden
        if( entry.title_override.empty() )
            EnsureTitleExists(entry.document->filename);

        m_documentsInIndex.emplace(entry.document.get());
        entries.emplace_back(std::move(entry));
    }

    return entries;
}


std::optional<DocSetIndex> DocSetIndex::Compile(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles)
{
    CompileWorker compiler_worker(doc_set_compiler, doc_set_spec, validate_titles);
    return compiler_worker.Compile(json_node);
}



// --------------------------------------------------------------------------
// DocSetIndex::Writer
// --------------------------------------------------------------------------

void DocSetIndex::Writer::Write(const DocSetIndex& index)
{
    StartWriting();
    WriteEntries(index.m_entries);
    FinishWriting();
}


void DocSetIndex::Writer::WriteEntries(const std::vector<Entry>& entries)
{
    StartEntries();

    for( const Entry& entry : entries )
    {
        const std::wstring* title_override = !entry.title_override.empty() ? &entry.title_override :
                                                                             nullptr;

        const void* subentries_tag = !entry.subentries.empty() ? &entry.subentries :
                                                                 nullptr;

        WriteEntry(entry.document->filename, title_override, subentries_tag);
    }

    FinishEntries();
}


void DocSetIndex::Writer::WriteSubentries(const void* subentries_tag)
{
    ASSERT(subentries_tag != nullptr);
    WriteEntries(*reinterpret_cast<const std::vector<Entry>*>(subentries_tag));
}



// --------------------------------------------------------------------------
// DocSetIndex_JsonWriterWorker
// --------------------------------------------------------------------------

class DocSetIndex_JsonWriterWorker : public DocSetIndexTableOfContentsBaseJsonWriterWorker, public DocSetIndex::Writer
{
public:
    DocSetIndex_JsonWriterWorker(JsonWriter& json_writer, DocSetSpec* doc_set_spec,
                                 bool write_documents_with_filename_only_when_possible, bool write_evaluated_titles, bool detailed_format)
        :   DocSetIndexTableOfContentsBaseJsonWriterWorker(json_writer, doc_set_spec, DocSetComponent::Type::Index, write_documents_with_filename_only_when_possible, write_evaluated_titles, detailed_format)
    {
    }
    
protected:
    void StartEntries() override
    {
        m_jsonWriter.BeginArray();
    }

    void WriteEntry(const std::wstring& csdoc_filename, const std::wstring* title_override, const void* subentries_tag) override
    {
        const std::optional<std::wstring> title = GetTitleOrOptional(csdoc_filename, title_override);
        bool write_as_object = ( title.has_value() || subentries_tag != nullptr );

        if( m_detailedFormat )
        {
            write_as_object = true;
            m_jsonFormattingHolder.emplace(m_jsonWriter.SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing));
        }

        if( write_as_object )
        {
            m_jsonWriter.BeginObject();

            if( m_detailedFormat )
                m_jsonWriter.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

            m_jsonWriter.Key(JK::path);
        }

        WritePath(csdoc_filename);

        if( write_as_object )
        {
            if( title.has_value() )
                WriteTitle(title_override, *title);

            if( subentries_tag != nullptr )
            {
                if( m_detailedFormat )
                    m_jsonWriter.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitMultiLine);

                m_jsonWriter.Key(JK::subindex);
                WriteSubentries(subentries_tag);
            }

            m_jsonWriter.EndObject();

            m_jsonFormattingHolder.reset();
        }

        ASSERT(!m_jsonFormattingHolder.has_value());
    }

    void FinishEntries() override
    {
        m_jsonWriter.EndArray();
    }
};


void DocSetIndex::WriteJson(JsonWriter& json_writer, DocSetSpec* doc_set_spec/* = nullptr*/,
                            bool write_documents_with_filename_only_when_possible/* = false*/,
                            bool write_evaluated_titles/* = false*/, bool detailed_format/* = false*/) const
{
    DocSetIndex_JsonWriterWorker json_writer_worker(json_writer, doc_set_spec, write_documents_with_filename_only_when_possible, write_evaluated_titles, detailed_format);
    json_writer_worker.Write(*this);
}
