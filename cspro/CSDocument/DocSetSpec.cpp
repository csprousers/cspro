#include "StdAfx.h"
#include "DocSetSpec.h"


DocSetSpec::DocSetSpec()
{
    // the default constructor should only be called when compiling CSPro Documents without an associated Document Set
}


DocSetSpec::DocSetSpec(std::wstring filename)
    :   m_filename(std::move(filename))
{
    AddComponent(std::make_shared<DocSetComponent>(DocSetComponent::Type::Spec, m_filename));
}


void DocSetSpec::Reset()
{
    m_components.clear();
    AddComponent(std::make_shared<DocSetComponent>(DocSetComponent::Type::Spec, m_filename));

    m_filenameDocumentMap.clear();

    m_title.reset();
    m_coverPageDocument.reset();
    m_defaultDocument.reset();
    m_tableOfContents.reset();
    m_index.reset();
    m_settings.Reset();
    m_definitions.clear();
    m_contextIds.clear();

    m_lastCompilationDetails.reset();
}


void DocSetSpec::AddComponent(std::shared_ptr<DocSetComponent> doc_set_component)
{
    ASSERT(doc_set_component != nullptr && FindComponent(doc_set_component->filename, false) == nullptr);

    // add documents to the filename map
    if( doc_set_component->type == DocSetComponent::Type::Document )
        m_filenameDocumentMap[PortableFunctions::PathGetFilename(doc_set_component->filename)].emplace_back(doc_set_component);

    m_components.emplace_back(std::move(doc_set_component));
}


std::shared_ptr<DocSetComponent> DocSetSpec::FindComponent(const std::wstring& filename, bool search_special_documents) const
{
    const auto& lookup = std::find_if(m_components.cbegin(), m_components.cend(),
                                      [&](const std::shared_ptr<DocSetComponent>& doc_set_component) { return SO::EqualsNoCase(filename, doc_set_component->filename); });

    if( lookup != m_components.cend() )
        return *lookup;

    if( search_special_documents )
    {
        if( m_coverPageDocument != nullptr && SO::EqualsNoCase(filename, m_coverPageDocument->filename) )
            return m_coverPageDocument;
    }

    return nullptr;
}


const std::vector<std::shared_ptr<DocSetComponent>>* DocSetSpec::FindDocument(const std::wstring& filename) const
{
    const auto& lookup = m_filenameDocumentMap.find(PortableFunctions::PathGetFilename(filename));

    if( lookup != m_filenameDocumentMap.cend() )
    {
        ASSERT(!lookup->second.empty());
        return &lookup->second;
    }

    ASSERT(FindComponent(filename, false) == nullptr);
    return nullptr;
}


std::wstring DocSetSpec::GetTitleOrFilenameWithoutExtension() const
{
    return m_title.has_value() ? *m_title :
                                 PortableFunctions::PathGetFilenameWithoutExtension(m_filename);
}


void DocSetSpec::WriteJsonSpecOnly(JsonWriter& json_writer, const JsonNode<wchar_t>* json_node_to_copy_documents_node,
                                   const GlobalSettings* global_settings, bool detailed_format)
{
    json_writer.BeginObject();

    // organize the components by type
    std::map<DocSetComponent::Type, std::vector<const DocSetComponent*>> doc_set_components_by_type;

    for( const std::shared_ptr<DocSetComponent>& doc_set_component : m_components )
        doc_set_components_by_type[doc_set_component->type].emplace_back(doc_set_component.get());

    auto get_components_of_type = [&](DocSetComponent::Type doc_set_component_type)
    {
        const auto& lookup = doc_set_components_by_type.find(doc_set_component_type);
        return ( lookup != doc_set_components_by_type.cend() ) ? &lookup->second :
                                                                 nullptr;
    };

    // title
    if( m_title.has_value() )
        json_writer.Write(JK::title, *m_title);

    // cover page document
    if( m_coverPageDocument != nullptr )
    {
        json_writer.Key(JK::coverPageDocument);
        WriteDocumentPathWithFilenameOnlyWhenPossible(json_writer, m_coverPageDocument->filename, false);
    }

    // default document
    if( m_defaultDocument != nullptr )
    {
        json_writer.Key(JK::defaultDocument);
        WriteDocumentPathWithFilenameOnlyWhenPossible(json_writer, m_defaultDocument->filename, false);
    }

    // imported components
    {
        std::optional<RAII::RunOnDestruction> import_object_run_on_destruction;
        const std::vector<const DocSetComponent*>* doc_set_components;

        auto get_components_of_type_and_start_import_node = [&](DocSetComponent::Type doc_set_component_type)
        {
            doc_set_components = get_components_of_type(doc_set_component_type);

            if( doc_set_components == nullptr )
                return false;

            if( !import_object_run_on_destruction.has_value() )
            {
                json_writer.BeginObject(JK::import);
                import_object_run_on_destruction.emplace([&]() { json_writer.EndObject(); });
            }

            return true;
        };

        auto write_single_component = [&](const TCHAR* key, DocSetComponent::Type doc_set_component_type)
        {
            if( !get_components_of_type_and_start_import_node(doc_set_component_type) )
                return;

            ASSERT(doc_set_components->size() == 1);
            json_writer.WriteRelativePath(key, doc_set_components->front()->filename);
        };

        auto write_multiple_components = [&](const TCHAR* key, DocSetComponent::Type doc_set_component_type)
        {
            if( !get_components_of_type_and_start_import_node(doc_set_component_type) )
                return;

            ASSERT(!doc_set_components->empty());

            json_writer.WriteArray(key, *doc_set_components,
                [&](const DocSetComponent* doc_set_component)
                {
                    json_writer.WriteRelativePath(doc_set_component->filename);
                });
        };

        write_single_component(JK::tableOfContents, DocSetComponent::Type::TableOfContents);
        write_single_component(JK::index, DocSetComponent::Type::Index);
        write_multiple_components(JK::settings, DocSetComponent::Type::Settings);
        write_multiple_components(JK::definitions, DocSetComponent::Type::Definitions);

        // context IDs may be part of the CSPro code directory so we cannot use the write_multiple_components routine
        if( get_components_of_type_and_start_import_node(DocSetComponent::Type::ContextIds) )
        {
            std::wstring cspro_code_path;

            if( global_settings != nullptr && !global_settings->cspro_code_path.empty() )
                cspro_code_path = PortableFunctions::PathEnsureTrailingSlash(PortableFunctions::PathToNativeSlash(global_settings->cspro_code_path));

            json_writer.WriteArray(JK::contextIds, *doc_set_components,
                [&](const DocSetComponent* doc_set_component)
                {
                    if( !cspro_code_path.empty() )
                    {
                        const std::wstring component_path = PortableFunctions::PathToNativeSlash(doc_set_component->filename);

                        if( SO::StartsWithNoCase(component_path, cspro_code_path) )
                        {
                            json_writer.WritePath(component_path.substr(cspro_code_path.length()));
                            return;
                        }
                    }

                    json_writer.WriteRelativePath(doc_set_component->filename);
                });
        }
    }

    // settings
    if( m_settings.HasCustomSettings() )
    {
        json_writer.Key(JK::settings);
        m_settings.WriteJson(json_writer, false);
    }

    // definitions
    if( !m_definitions.empty() )
    {
        json_writer.Key(JK::definitions);
        WriteJsonDefinitions(json_writer, m_definitions, true);
    }

    // documents
    const std::vector<const DocSetComponent*>* document_doc_set_components = get_components_of_type(DocSetComponent::Type::Document);

    if( document_doc_set_components != nullptr &&
        std::find_if(document_doc_set_components->cbegin(), document_doc_set_components->cend(),
                    [&](const DocSetComponent* doc_set_component) { return doc_set_component->component_comes_from_documents_node; }) != document_doc_set_components->cend() )
    {
        json_writer.Key(JK::documents);

        if( json_node_to_copy_documents_node != nullptr && json_node_to_copy_documents_node->Contains(JK::documents) )
        {
            json_writer.Write(json_node_to_copy_documents_node->Get(JK::documents));
        }

        else
        {
            json_writer.BeginArray();

            for( const DocSetComponent* doc_set_component : *document_doc_set_components )
            {
                if( doc_set_component->component_comes_from_documents_node )
                    json_writer.WriteRelativePath(doc_set_component->filename);
            }

            json_writer.EndArray();
        }
    }

    // inline table of contents
    if( m_tableOfContents.has_value() &&
        std::find_if(m_components.cbegin(), m_components.cend(),
                     [&](const std::shared_ptr<DocSetComponent>& doc_set_component) { return ( doc_set_component->type == DocSetComponent::Type::TableOfContents); }) == m_components.cend() )
    {
        json_writer.Key(JK::tableOfContents);
        m_tableOfContents->WriteJson(json_writer, this, true, false, detailed_format);
    }

    // inline index
    if( m_index.has_value() &&
        std::find_if(m_components.cbegin(), m_components.cend(),
                     [&](const std::shared_ptr<DocSetComponent>& doc_set_component) { return ( doc_set_component->type == DocSetComponent::Type::Index); }) == m_components.cend() )
    {
        m_index->SortByTitle(*this);

        json_writer.Key(JK::index);
        m_index->WriteJson(json_writer, this, true, false, detailed_format);
    }

    json_writer.EndObject();
}


void DocSetSpec::WriteJsonDefinitions(JsonWriter& json_writer, const std::vector<std::tuple<std::wstring, std::wstring>>& definitions, bool write_as_object)
{
    if( write_as_object )
    {
        json_writer.BeginObject();

        for( const auto& [key, value] : definitions )
            json_writer.Write(key, value);

        json_writer.EndObject();
    }

    else
    {
        json_writer.BeginArray();

        for( const auto& [key, value] : definitions )
        {
            json_writer.BeginObject()
                       .Write(JK::key, key)
                       .Write(JK::value, value)
                       .EndObject();
        }

        json_writer.EndArray();
    }
}


void DocSetSpec::WriteJsonContextIds(JsonWriter& json_writer, const std::map<std::wstring, unsigned>& context_ids)
{
    json_writer.BeginArray();

    for( const auto& [name, id] : context_ids )
    {
        json_writer.BeginObject()
                   .Write(JK::name, name)
                   .Write(JK::id, id)
                   .EndObject();
    }

    json_writer.EndArray();
}


void DocSetSpec::WriteDocumentPathWithFilenameOnlyWhenPossible(JsonWriter& json_writer, const std::wstring& path, bool documents_with_filename_must_come_from_documents_node)
{
    const std::vector<std::shared_ptr<DocSetComponent>>* doc_set_components_with_name = FindDocument(path);

    if( doc_set_components_with_name != nullptr &&
        doc_set_components_with_name->size() == 1 &&
        ( !documents_with_filename_must_come_from_documents_node || doc_set_components_with_name->front()->component_comes_from_documents_node ) )
    {
        // the name is unique
        json_writer.Write(PortableFunctions::PathGetFilename(path));
    }

    else
    {
        json_writer.WriteRelativePath(path);
    }
}


void DocSetSpec::WriteNewDocumentSetShell(const std::wstring& filename)
{
    // create the default text for a new Document Set and write it
    auto json_writer = Json::CreateFileWriter(filename);

    json_writer->BeginObject();

    json_writer->Write(JK::title, PortableFunctions::PathGetFilenameWithoutExtension(filename));

    json_writer->BeginObject(JK::import)
                .WriteNull(JK::tableOfContents)
                .WriteNull(JK::index)
                .BeginArray(JK::settings).EndArray()
                .BeginArray(JK::definitions).EndArray()
                .EndObject();

    json_writer->WriteNull(JK::tableOfContents);
    json_writer->WriteNull(JK::index);
    json_writer->BeginObject(JK::settings).EndObject();
    json_writer->BeginObject(JK::definitions).EndObject();
    json_writer->BeginArray(JK::documents).EndArray();

    json_writer->EndObject();
}
