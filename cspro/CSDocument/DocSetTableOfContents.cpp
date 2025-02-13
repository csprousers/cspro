#include "StdAfx.h"
#include "DocSetTableOfContents.h"
#include "DocSetIndexTableOfContentsBase.h"


// --------------------------------------------------------------------------
// DocSetTableOfContents
// --------------------------------------------------------------------------

bool DocSetTableOfContents::operator==(const DocSetTableOfContents& rhs) const
{
    if( m_nodes.size() != rhs.m_nodes.size() ||
        m_documentTitleOverrides != rhs.m_documentTitleOverrides )
    {
        return false;
    }

    auto lhs_nodes_itr = m_nodes.cbegin();
    auto lhs_nodes_end = m_nodes.cend();
    auto rhs_nodes_itr = rhs.m_nodes.cbegin();

    for( ; lhs_nodes_itr != lhs_nodes_end; ++lhs_nodes_itr, ++rhs_nodes_itr )
    {
        if( lhs_nodes_itr->index() != rhs_nodes_itr->index() )
            return false;

        if( std::holds_alternative<Chapter>(*lhs_nodes_itr) )
        {
            std::function<bool(const Chapter&, const Chapter&)> chapters_are_equal;

            chapters_are_equal = [&](const Chapter& lhs_chapter, const Chapter& rhs_chapter)
            {
                if( lhs_chapter.title != rhs_chapter.title ||
                    !VectorHelpers::ValueOfSharedPointersIsEqual(lhs_chapter.documents, rhs_chapter.documents) ||
                    lhs_chapter.chapters.size() != rhs_chapter.chapters.size() )
                {
                    return false;
                }

                auto lhs_chapters_itr = lhs_chapter.chapters.cbegin();
                auto lhs_chapters_end = lhs_chapter.chapters.cend();
                auto rhs_chapters_itr = rhs_chapter.chapters.cbegin();

                for( ; lhs_chapters_itr != lhs_chapters_end; ++lhs_chapters_itr, ++rhs_chapters_itr )
                {
                    if( !chapters_are_equal(*lhs_chapters_itr, *rhs_chapters_itr) )
                        return false;
                }

                return true;
            };

            if( !chapters_are_equal(std::get<Chapter>(*lhs_nodes_itr), std::get<Chapter>(*rhs_nodes_itr)) )
                return false;
        }

        else
        {
            if( std::get<ProjectLink>(*lhs_nodes_itr).project != std::get<ProjectLink>(*rhs_nodes_itr).project )
                return false;
        }
    }

    return true;
}


size_t DocSetTableOfContents::GetPosition(const DocSetComponent& doc_set_component) const
{
    const auto& lookup = std::find_if(m_documentsInOrder.cbegin(), m_documentsInOrder.cend(),
        [&](const std::shared_ptr<DocSetComponent>& other_doc_set_component)
        {
            return ( &doc_set_component == other_doc_set_component.get() );
        });

    return ( lookup != m_documentsInOrder.cend() ) ? std::distance(m_documentsInOrder.cbegin(), lookup) :
                                                     SIZE_MAX;
}


const DocSetComponent* DocSetTableOfContents::GetDocumentByPosition(size_t position) const
{
    return ( position < m_documentsInOrder.size() ) ? m_documentsInOrder[position].get() :
                                                      nullptr;
}


const DocSetTableOfContents::Chapter* DocSetTableOfContents::GetRootChapter() const
{
    const auto& lookup = std::find_if(m_nodes.cbegin(), m_nodes.cend(),
        [&](const std::variant<Chapter, ProjectLink>& chapter_or_project_link)
        {
            return std::holds_alternative<Chapter>(chapter_or_project_link);
        });

    return ( lookup != m_nodes.cend() ) ? &std::get<Chapter>(*lookup) :
                                          nullptr;
}



// --------------------------------------------------------------------------
// DocSetTableOfContents::CompileWorker
// --------------------------------------------------------------------------

CREATE_JSON_KEY(writeTitleToPdf)

class DocSetTableOfContents::CompileWorker : public DocSetIndexTableOfContentsBaseCompileWorker
{
public:
    using DocSetIndexTableOfContentsBaseCompileWorker::DocSetIndexTableOfContentsBaseCompileWorker;

    std::optional<DocSetTableOfContents> Compile(const JsonNode<wchar_t>& json_node);

private:
    void CompileProjectLink(std::wstring project);

    void CompileTableOfContentsNode(const JsonNode<wchar_t>& json_node);

    std::optional<Chapter> CompileChapter(const JsonNode<wchar_t>& json_node);

    void AddDocumentsInOrder(const Chapter& chapter);

private:
    DocSetTableOfContents m_tableOfContents;
};


std::optional<DocSetTableOfContents> DocSetTableOfContents::CompileWorker::Compile(const JsonNode<wchar_t>& json_node)
{
    // the table of contents can be a single object, or an array of objects (which can be used to create cross-project links)
    if( json_node.IsArray() )
    {
        for( const auto& toc_node : json_node.GetArray() )
        {
            if( toc_node.Contains(JK::project) ) 
            {
                CompileProjectLink(m_docSetCompiler.JsonNodeGetStringWithWhitespaceCheck(toc_node, JK::project));
            }

            else
            {
                CompileTableOfContentsNode(toc_node);
            }
        }
    }

    else
    {
        CompileTableOfContentsNode(json_node);
    }

    const Chapter* root_chapter = m_tableOfContents.GetRootChapter();

    if( root_chapter == nullptr )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("A '%s' must define one node of titles, documents, and chapters."),
                                                  ToString(DocSetComponent::Type::TableOfContents)));
        return std::nullopt;
    }

    // get a list of all documents added via the table of contents
    AddDocumentsInOrder(*root_chapter);

    if( m_tableOfContents.m_documentsInOrder.empty() )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("A '%s' must contain at least one document."),
                                                  ToString(DocSetComponent::Type::TableOfContents)));
        return std::nullopt;
    }

    const auto& documents_in_order_begin = m_tableOfContents.m_documentsInOrder.cbegin();
    const auto& documents_in_order_end = m_tableOfContents.m_documentsInOrder.cend();

    // this routine will add any documents not already added to the components via the documents node;
    // this will be called even if exceptions are thrown so that, for example, a table of contents error doesn't prevent the document
    // compiler from being able to resolve links to documents that are in the table of contents (but not from the documents node)
    auto add_documents_not_in_components = [&]()
    {
        for( auto documents_in_order_itr = documents_in_order_begin; documents_in_order_itr != documents_in_order_end; ++documents_in_order_itr )
        {
            const std::shared_ptr<DocSetComponent>& doc_set_component = *documents_in_order_itr;

            if( m_docSetSpec.FindComponent(doc_set_component->filename, false) == nullptr )
                m_docSetCompiler.AddComponent(m_docSetSpec, doc_set_component);
        }
    };

    try
    {
        // make sure that the table of contents contains all documents specified in the components (via the documents node)
        for( const DocSetComponent& doc_set_component : VI_V(m_docSetSpec.GetComponents()) )
        {
            if( doc_set_component.type == DocSetComponent::Type::Document )
            {
                const auto& lookup = std::find_if(documents_in_order_begin, documents_in_order_end,
                    [&](const std::shared_ptr<DocSetComponent>& other_doc_set_component) { return SO::EqualsNoCase(doc_set_component.filename, other_doc_set_component->filename); });

                if( lookup == documents_in_order_end )
                {
                    m_docSetCompiler.AddError(FormatTextCS2WS(_T("The '%s' must contain the document specified in the 'documents' node: %s"),
                                                              ToString(DocSetComponent::Type::TableOfContents), doc_set_component.filename.c_str()));
                }
            }
        }

        // make sure that no documents added to the table of contents are duplicated
        for( auto documents_in_order_itr = documents_in_order_begin; documents_in_order_itr != documents_in_order_end; ++documents_in_order_itr )
        {
            const std::shared_ptr<DocSetComponent>& doc_set_component = *documents_in_order_itr;

            const auto& lookup = std::find_if(documents_in_order_begin, documents_in_order_itr,
                [&](const std::shared_ptr<DocSetComponent>& other_doc_set_component) { return SO::EqualsNoCase(doc_set_component->filename, other_doc_set_component->filename); });

            if( lookup < documents_in_order_itr )
            {
                m_docSetCompiler.AddError(FormatTextCS2WS(_T("The '%s' cannot contain multiple instances of: %s"),
                                                          ToString(DocSetComponent::Type::TableOfContents), doc_set_component->filename.c_str()));
            }
        }

        add_documents_not_in_components();
    }

    catch(...)
    {
        add_documents_not_in_components();
        throw;
    }

    return std::move(m_tableOfContents);
}


void DocSetTableOfContents::CompileWorker::CompileProjectLink(std::wstring project)
{
    try
    {
        // calling FindProjectDocSetSpecFilename will throw an exception if the project name is not valid
        project = PortableFunctions::PathRemoveTrailingSlash(project);
        m_docSetSpec.GetSettings().FindProjectDocSetSpecFilename(project);
        ASSERT(project.find_first_of(PortableFunctions::PathSlashChars) == std::wstring_view::npos);

        const auto& lookup = std::find_if(m_tableOfContents.m_nodes.cbegin(), m_tableOfContents.m_nodes.cend(),
            [&](const std::variant<Chapter, ProjectLink>& chapter_or_project_link)
            {
                return ( std::holds_alternative<ProjectLink>(chapter_or_project_link) &&
                         SO::EqualsNoCase(project, std::get<ProjectLink>(chapter_or_project_link).project) );
            });

        if( lookup != m_tableOfContents.m_nodes.cend() )
        {
            throw CSProException(_T("The '%s' cannot contain multiple links to the project: %s"),
                                 ToString(DocSetComponent::Type::TableOfContents), project.c_str());
        }

        m_tableOfContents.m_nodes.emplace_back(ProjectLink { std::move(project) });
    }

    catch( const CSProException& exception )
    {
        m_docSetCompiler.AddError(exception.GetErrorMessage());
    }
}


void DocSetTableOfContents::CompileWorker::CompileTableOfContentsNode(const JsonNode<wchar_t>& json_node)
{
    m_docSetCompiler.EnsureJsonNodeIsObject(json_node, ToString(DocSetComponent::Type::TableOfContents));

    if( m_tableOfContents.GetRootChapter() != nullptr )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("A '%s' can only have one node of titles, documents, and chapters."),
                                                  ToString(DocSetComponent::Type::TableOfContents)));
        return;
    }

    std::optional<Chapter> chapter = CompileChapter(json_node);

    if( chapter.has_value() )
        m_tableOfContents.m_nodes.emplace_back(std::move(*chapter));
}


std::optional<DocSetTableOfContents::Chapter> DocSetTableOfContents::CompileWorker::CompileChapter(const JsonNode<wchar_t>& json_node)
{
    Chapter chapter;

    // title
    chapter.title = json_node.GetOrDefault(JK::title, SO::EmptyString);

    if( chapter.title.empty() )
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("A '%s' chapter must contain a title."), ToString(DocSetComponent::Type::TableOfContents)));

    chapter.write_title_to_pdf = json_node.GetOrDefault(JK::writeTitleToPdf, true);

    // documents
    for( const auto& documents_node : json_node.GetArrayOrEmpty(JK::documents) )
    {
        std::shared_ptr<DocSetComponent> doc_set_component =
            documents_node.IsObject() ? m_docSetCompiler.JsonNodeGetDocumentWithSupportForFilenameOnly(documents_node.Get(JK::path), m_docSetSpec, true, ToString(DocSetComponent::Type::TableOfContents)) :
                                        m_docSetCompiler.JsonNodeGetDocumentWithSupportForFilenameOnly(documents_node, m_docSetSpec, true, ToString(DocSetComponent::Type::TableOfContents));

        if( doc_set_component == nullptr || !m_docSetCompiler.CheckFileValidity(*doc_set_component) )
            continue;

        if( documents_node.IsObject() && documents_node.Contains(JK::title) )
        {
            m_tableOfContents.m_documentTitleOverrides.try_emplace(doc_set_component->filename,
                                                                   m_docSetCompiler.JsonNodeGetStringWithWhitespaceCheck(documents_node, JK::title));
        }

        // make sure the document has a title if the title was not overriden
        else
        {
            EnsureTitleExists(doc_set_component->filename);
        }

        chapter.documents.emplace_back(std::move(doc_set_component));
    }

    // chapters
    for( const auto& chapters_node : json_node.GetArrayOrEmpty(JK::chapters) )
    {
        std::optional<Chapter> subchapter = CompileChapter(chapters_node);

        if( subchapter.has_value() )
            chapter.chapters.emplace_back(std::move(*subchapter));
    }

    if( chapter.documents.empty() && chapter.chapters.empty() )
    {
        m_docSetCompiler.AddError(FormatTextCS2WS(_T("A '%s' chapter must contain at least one document or chapter."),
                                                  ToString(DocSetComponent::Type::TableOfContents)));
        return std::nullopt;
    }

    return chapter;
}


void DocSetTableOfContents::CompileWorker::AddDocumentsInOrder(const Chapter& chapter)
{
    for( const std::shared_ptr<DocSetComponent>& doc_set_component : chapter.documents )
        m_tableOfContents.m_documentsInOrder.emplace_back(doc_set_component);

    for( const Chapter& subchapter : chapter.chapters )
        AddDocumentsInOrder(subchapter);
}


std::optional<DocSetTableOfContents> DocSetTableOfContents::Compile(DocSetCompiler& doc_set_compiler, DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles)
{
    CompileWorker compiler_worker(doc_set_compiler, doc_set_spec, validate_titles);
    return compiler_worker.Compile(json_node);
}



// --------------------------------------------------------------------------
// DocSetTableOfContents::Writer
// --------------------------------------------------------------------------

void DocSetTableOfContents::Writer::Write(const DocSetTableOfContents& table_of_contents)
{
    ASSERT(table_of_contents.GetRootChapter() != nullptr);

    m_documentTitleOverrides = &table_of_contents.m_documentTitleOverrides;

    StartWriting(table_of_contents.m_nodes.size());

    for( const std::variant<Chapter, ProjectLink>& node : table_of_contents.m_nodes )
    {
        if( std::holds_alternative<Chapter>(node) )
        {
            WriteChapter(std::get<Chapter>(node));
        }

        else
        {
            WriteProject(std::get<ProjectLink>(node).project);
        }
    }

    FinishWriting();
}


void DocSetTableOfContents::Writer::WriteChapter(const Chapter& chapter)
{
    StartChapter(chapter.title, chapter.write_title_to_pdf);

    if( !chapter.documents.empty() )
    {
        StartDocuments();

        for( const DocSetComponent& doc_set_component : VI_V(chapter.documents) )
        {
            const auto& title_override_lookup = m_documentTitleOverrides->find(doc_set_component.filename);
            const std::wstring* title_override = ( title_override_lookup != m_documentTitleOverrides->cend() ) ? &title_override_lookup->second : 
                                                                                                                 nullptr;
            WriteDocument(doc_set_component.filename, title_override);
        }

        FinishDocuments();
    }

    if( !chapter.chapters.empty() )
    {
        StartSubchapters();

        for( const Chapter& subchapter : chapter.chapters )
            WriteChapter(subchapter);

        FinishSubchapters();
    }

    FinishChapter();
}



// --------------------------------------------------------------------------
// DocSetTableOfContents_JsonWriterWorker
// --------------------------------------------------------------------------

class DocSetTableOfContents_JsonWriterWorker : public DocSetIndexTableOfContentsBaseJsonWriterWorker, public DocSetTableOfContents::Writer
{
public:
    DocSetTableOfContents_JsonWriterWorker(JsonWriter& json_writer, DocSetSpec* doc_set_spec, bool write_documents_with_filename_only_when_possible, bool write_evaluated_titles, bool detailed_format)
        :   DocSetIndexTableOfContentsBaseJsonWriterWorker(json_writer, doc_set_spec, DocSetComponent::Type::TableOfContents, write_documents_with_filename_only_when_possible, write_evaluated_titles, detailed_format),
            m_writingAsArray(false)
    {
    }

protected:
    void StartWriting(size_t num_root_nodes) override
    {
        if( num_root_nodes > 1 )
        {
            m_writingAsArray = true;
            m_jsonWriter.BeginArray();
        }
    }

    void FinishWriting() override
    {
        if( m_writingAsArray )
            m_jsonWriter.EndArray();
    }

    void WriteProject(const std::wstring& project) override
    {
        m_jsonWriter.BeginObject()
                    .Write(JK::project, project)
                    .EndObject();
    }

    void StartChapter(const std::wstring& title, bool write_title_to_pdf) override
    {
        m_jsonWriter.BeginObject()
                    .Write(JK::title, title)
                    .WriteIfNot(JK::writeTitleToPdf, write_title_to_pdf, true);
    }

    void FinishChapter() override
    {
        m_jsonWriter.EndObject();
    }

    void StartDocuments() override
    {
        m_jsonWriter.BeginArray(JK::documents);
    }

    void WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override) override
    {
        const std::optional<std::wstring> title = GetTitleOrOptional(csdoc_filename, title_override);

        if( title.has_value() )
        {
            if( m_detailedFormat )
                m_jsonFormattingHolder.emplace(m_jsonWriter.SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing));

            m_jsonWriter.BeginObject();

            if( m_detailedFormat )
                m_jsonWriter.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

            m_jsonWriter.Key(JK::path);
        }

        WritePath(csdoc_filename);

        if( title.has_value() )
        {
            WriteTitle(title_override, *title);

            m_jsonWriter.EndObject();

            m_jsonFormattingHolder.reset();
        }

        ASSERT(!m_jsonFormattingHolder.has_value());
    }

    void FinishDocuments() override
    {
        m_jsonWriter.EndArray();
    }

    void StartSubchapters() override
    {
        m_jsonWriter.BeginArray(JK::chapters);
    }

    void FinishSubchapters() override
    {
        m_jsonWriter.EndArray();
    }

private:
    bool m_writingAsArray;
};


void DocSetTableOfContents::WriteJson(JsonWriter& json_writer, DocSetSpec* doc_set_spec/* = nullptr*/,
                            bool write_documents_with_filename_only_when_possible/* = false*/,
                            bool write_evaluated_titles/* = false*/, bool detailed_format/* = false*/) const
{
    DocSetTableOfContents_JsonWriterWorker json_writer_worker(json_writer, doc_set_spec, write_documents_with_filename_only_when_possible, write_evaluated_titles, detailed_format);
    json_writer_worker.Write(*this);
}
