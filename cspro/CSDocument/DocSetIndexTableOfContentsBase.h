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

    std::wstring GetTitle(const std::wstring& csdoc_filename)
    {
        return m_titleManager.GetTitle(csdoc_filename);
    }

    std::optional<std::wstring> GetTitleOrOptional(const std::wstring& csdoc_filename)
    {
        try
        {
            return GetTitle(csdoc_filename);
        }

        catch(...)
        {
            return std::nullopt;
        }        
    }

    std::wstring GetTitleOrFilenameWithoutExtension(const std::wstring& csdoc_filename)
    {
        try
        {
            return GetTitle(csdoc_filename);
        }

        catch(...)
        {
            return PortableFunctions::PathGetFilenameWithoutExtension(csdoc_filename);
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
    DocSetIndexTableOfContentsBaseCompileWorker(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, bool validate_titles)
        :   m_docSetCompiler(doc_set_compiler),
            m_docSetSpec(doc_set_spec),
            m_validateTitles(validate_titles),
            m_titleLookupWorker(doc_set_spec)
    {
    }

protected:
    void EnsureTitleExists(const std::wstring& csdoc_filename)
    {
        if( !m_validateTitles )
            return;

        try
        {
            m_titleLookupWorker.GetTitle(csdoc_filename);
        }

        catch( const CSProException& exception )
        {
            m_docSetCompiler.AddError(exception.GetErrorMessage());
        }
    }

protected:
    DocSetCompiler& m_docSetCompiler;
    DocSetSpec& m_docSetSpec;
    const bool m_validateTitles;
    DocSetIndexTableOfContentsBaseTitleLookupWorker m_titleLookupWorker;
};



// --------------------------------------------------------------------------
// DocSetIndexTableOfContentsBaseJsonWriterWorker
// --------------------------------------------------------------------------

class DocSetIndexTableOfContentsBaseJsonWriterWorker
{
protected:
    DocSetIndexTableOfContentsBaseJsonWriterWorker(JsonWriter& json_writer, DocSetSpec* doc_set_spec, DocSetComponent::Type doc_set_component_type,
                                                   bool write_documents_with_filename_only_when_possible, bool write_evaluated_titles, bool detailed_format)
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

    std::optional<std::wstring> GetTitleOrOptional(const std::wstring& csdoc_filename, const std::wstring* title_override) const
    {
        return ( title_override != nullptr )      ? std::make_optional(*title_override) :
               ( m_titleLookupWorker != nullptr ) ? m_titleLookupWorker->GetTitleOrOptional(csdoc_filename) :
                                                    std::nullopt;
    }

    void WritePath(const std::wstring& csdoc_filename) const
    {
        if( m_writeDocumentsWithFilenameOnlyWhenPossible )
        {
            ASSERT(m_docSetSpec != nullptr);
            m_docSetSpec->WriteDocumentPathWithFilenameOnlyWhenPossible(m_jsonWriter, csdoc_filename, m_writeDocumentsWithFilenameMustComeFromDocumentNodes);
        }

        else
        {
            m_jsonWriter.WriteRelativePath(csdoc_filename);
        }
    }

    void WriteTitle(const std::wstring* title_override, const std::wstring& title)
    {
        // when writing to format the component in detailed mode, include titles that are not overriden with a ~ before the key
        m_jsonWriter.Write(( m_detailedFormat && title_override == nullptr ) ? _T("~title") : JK::title, title);
    }

protected:
    JsonWriter& m_jsonWriter;
    const bool m_detailedFormat;
    std::optional<JsonWriter::FormattingHolder> m_jsonFormattingHolder;

private:
    DocSetSpec* m_docSetSpec;
    const bool m_writeDocumentsWithFilenameOnlyWhenPossible;
    const bool m_writeDocumentsWithFilenameMustComeFromDocumentNodes;
    std::unique_ptr<DocSetIndexTableOfContentsBaseTitleLookupWorker> m_titleLookupWorker;
};
