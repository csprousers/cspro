#pragma once

#include <CSDocument/TitleManager.h>


// --------------------------------------------------------------------------
// DocSetIndexTableOfContentsBaseTitleLookupWorker
// --------------------------------------------------------------------------

class DocSetIndexTableOfContentsBaseTitleLookupWorker
{
public:
    DocSetIndexTableOfContentsBaseTitleLookupWorker(DocSetSpec& doc_set_spec)
        :   m_titleManager(&doc_set_spec)
    {
    }

    std::string GetTitle(const std::string& csdoc_file_path)
    {
        return m_titleManager.GetTitle(csdoc_file_path);
    }

    std::optional<std::string> GetTitleOrOptional(const std::string& csdoc_file_path)
    {
        try
        {
            return GetTitle(csdoc_file_path);
        }

        catch(...)
        {
            return std::nullopt;
        }
    }

    std::string GetTitleOrFilenameWithoutExtension(const std::string& csdoc_file_path)
    {
        try
        {
            return GetTitle(csdoc_file_path);
        }

        catch(...)
        {
            return Path::GetFilenameWithoutExtension(csdoc_file_path);
        }
    }

private:
    TitleManager m_titleManager;
};



// --------------------------------------------------------------------------
// DocSetIndexTableOfContentsBaseCompileWorker
// --------------------------------------------------------------------------

class DocSetIndexTableOfContentsBaseCompileWorker
{
public:
    DocSetIndexTableOfContentsBaseCompileWorker(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const bool validate_titles)
        :   m_docSetCompiler(doc_set_compiler),
            m_docSetSpec(doc_set_spec),
            m_validateTitles(validate_titles),
            m_titleLookupWorker(doc_set_spec)
    {
    }

protected:
    void EnsureTitleExists(const std::string& csdoc_file_path)
    {
        if( !m_validateTitles )
            return;

        try
        {
            m_titleLookupWorker.GetTitle(csdoc_file_path);
        }

        catch( const CSProException& exception )
        {
            m_docSetCompiler.AddError(exception.what());
        }
    }

protected:
    DocSetCompiler& m_docSetCompiler;
    DocSetSpec& m_docSetSpec;
    bool m_validateTitles;
    DocSetIndexTableOfContentsBaseTitleLookupWorker m_titleLookupWorker;
};



// --------------------------------------------------------------------------
// DocSetIndexTableOfContentsBaseJsonWriterWorker
// --------------------------------------------------------------------------

class DocSetIndexTableOfContentsBaseJsonWriterWorker
{
protected:
    DocSetIndexTableOfContentsBaseJsonWriterWorker(JsonWriter& json_writer, DocSetSpec* const doc_set_spec, const DocSetComponent::Type doc_set_component_type,
                                                   const bool write_documents_with_filename_only_when_possible, const bool write_evaluated_titles, const bool detailed_format)
        :   m_jsonWriter(json_writer),
            m_detailedFormat(detailed_format),
            m_docSetSpec(doc_set_spec),
            m_writeDocumentsWithFilenameOnlyWhenPossible(write_documents_with_filename_only_when_possible),
            m_writeDocumentsWithFilenameMustComeFromDocumentNodes(doc_set_component_type == DocSetComponent::Type::TableOfContents)
    {
        ASSERT(( !write_documents_with_filename_only_when_possible && !write_evaluated_titles ) || ( doc_set_spec != nullptr ));

        if( write_evaluated_titles || m_detailedFormat )
        {
            ASSERT(m_docSetSpec != nullptr);
            m_titleLookupWorker = std::make_unique<DocSetIndexTableOfContentsBaseTitleLookupWorker>(*m_docSetSpec);
        }
    }

    std::optional<std::string> GetTitleOrOptional(const std::string& csdoc_file_path, const std::string* const title_override) const
    {
        return ( title_override != nullptr )      ? std::make_optional(*title_override) :
               ( m_titleLookupWorker != nullptr ) ? m_titleLookupWorker->GetTitleOrOptional(csdoc_file_path) :
                                                    std::nullopt;
    }

    void WritePath(const std::string& csdoc_file_path) const
    {
        if( m_writeDocumentsWithFilenameOnlyWhenPossible )
        {
            ASSERT(m_docSetSpec != nullptr);
            m_docSetSpec->WriteDocumentPathWithFilenameOnlyWhenPossible(m_jsonWriter, csdoc_file_path, m_writeDocumentsWithFilenameMustComeFromDocumentNodes);
        }

        else
        {
            m_jsonWriter.WriteRelativePath(csdoc_file_path);
        }
    }

    void WriteTitle(const std::string* const title_override, const std::string& title)
    {
        // when writing to format the component in detailed mode, include titles that are not overridden with a ~ before the key
        m_jsonWriter.Write(( m_detailedFormat && title_override == nullptr ) ? "~title" : JK::title, title);
    }

protected:
    JsonWriter& m_jsonWriter;
    bool m_detailedFormat;
    std::optional<JsonWriter::FormattingHolder> m_jsonFormattingHolder;

private:
    DocSetSpec* m_docSetSpec;
    bool m_writeDocumentsWithFilenameOnlyWhenPossible;
    bool m_writeDocumentsWithFilenameMustComeFromDocumentNodes;
    std::unique_ptr<DocSetIndexTableOfContentsBaseTitleLookupWorker> m_titleLookupWorker;
};
