#include "StdAfx.h"
#include "DocSetCompiler.h"


std::vector<DocSetCompiler::GetFileTextOrModifiedIterationCallback> DocSetCompiler::m_getFileTextOrModifiedIterationCallbacks;


DocSetCompiler::DocSetCompiler(const GlobalSettings& global_settings, ErrorIssuerType error_issuer)
    :   m_globalSettings(&global_settings),
        m_errorIssuer(std::move(error_issuer))
{
}


DocSetCompiler::DocSetCompiler(ErrorIssuerType error_issuer)
    :   m_globalSettings(nullptr),
        m_errorIssuer(std::move(error_issuer))
{
}


void DocSetCompiler::AddErrorOrWarning(bool error, const std::wstring& text)
{
    if( ( std::holds_alternative<SuppressErrors>(m_errorIssuer) ) ||
        ( std::holds_alternative<ThrowErrors>(m_errorIssuer) && !error ) )
    {
        return;
    }

    // when errors occur in a file other than the one being compiled, report the error with the filename
    const std::wstring& filename_with_error = !m_componentTextFilenames.empty() ? m_componentTextFilenames.back() :
                                                                                  SO::EmptyString;

    if( std::holds_alternative<ThrowErrors>(m_errorIssuer) )
    {
        throw CSProExceptionWithFilename(filename_with_error, text);
    }

    else if( error )
    {
        std::get<BuildWnd*>(m_errorIssuer)->AddError(filename_with_error, text);
    }

    else
    {
        std::get<BuildWnd*>(m_errorIssuer)->AddWarning(filename_with_error, text);
    }
}


JsonNode<wchar_t> DocSetCompiler::ComponentText::GetJsonNode()
{
    try
    {
        return Json::Parse(text, &json_reader_interface);
    }

    catch( const JsonParseException& json_parse_exception )
    {
        // rethrow the error as a CSProExceptionWithFilename so that it does not get handled by DocSetBaseFrame
        // as a JsonParseException with line numbers that apply to the current document
        throw CSProExceptionWithFilename(filename_holder.GetValue(), json_parse_exception.GetErrorMessage());
    }
}


RAII::PushOnVectorAndPopOnDestruction<DocSetCompiler::GetFileTextOrModifiedIterationCallback>
DocSetCompiler::OverrideGetFileTextOrModifiedIteration(GetFileTextOrModifiedIterationCallback callback)
{
    return RAII::PushOnVectorAndPopOnDestruction<GetFileTextOrModifiedIterationCallback>(m_getFileTextOrModifiedIterationCallbacks, std::move(callback));
}


template<typename T>
static T DocSetCompiler::GetFileTextOrModifiedIteration(const std::wstring& filename)
{
    // try to use an open document
    if( !m_getFileTextOrModifiedIterationCallbacks.empty() )
    {
        const std::tuple<std::wstring, int64_t>* text_and_modified_iteration = m_getFileTextOrModifiedIterationCallbacks.back()(filename);

        if( text_and_modified_iteration != nullptr )
            return std::get<T>(*text_and_modified_iteration);
    }

    else
    {
        std::shared_ptr<TextSourceEditable> text_source;

        if( WindowsDesktopMessage::Send(UWM::Designer::FindOpenTextSourceEditable, &filename, &text_source) == 1 )
        {
            ASSERT(text_source != nullptr);

            if constexpr(std::is_same_v<T, int64_t>)
            {
                return text_source->GetModifiedIteration();
            }

            else
            {
                return text_source->GetText();
            }
        }
    }

    // if not available, use the values from the disk...
    if constexpr(std::is_same_v<T, int64_t>)
    {
        return PortableFunctions::FileModifiedTime(filename);
    }

    else
    {
        // ... allowing FileIO to throw exceptions instead of using AddError because this is a fatal error
        return FileIO::ReadText(filename);
    }
}


DocSetCompiler::ComponentText DocSetCompiler::GetComponentText(const std::wstring& filename)
{
    ComponentText component_text
    {
        RAII::PushOnVectorAndPopOnDestruction<std::wstring>(m_componentTextFilenames, filename),
        std::wstring(),
        JsonReaderInterface(PortableFunctions::PathGetDirectory(filename))
    };

    component_text.text = GetFileTextOrModifiedIteration<std::wstring>(filename);

    return component_text;
}


bool DocSetCompiler::JsonNodeContainsAndIsNotNull(const JsonNode<wchar_t>& json_node, std::wstring_view key_sv)
{
    return ( json_node.Contains(key_sv) &&
             !json_node.Get(key_sv).IsNull() );
}


bool DocSetCompiler::EnsureJsonNodeIsObject(const JsonNode<wchar_t>& json_node, const TCHAR* type)
{
    if( json_node.IsObject() )
        return true;

    AddError(FormatTextCS2WS(_T("'%s' must be specified as a JSON object."), type));

    return false;
}


bool DocSetCompiler::EnsureJsonNodeIsArray(const JsonNode<wchar_t>& json_node, const TCHAR* type)
{
    if( json_node.IsArray() )
        return true;

    AddError(FormatTextCS2WS(_T("'%s' must be specified as a JSON array."), type));

    return false;
}


std::wstring DocSetCompiler::JsonNodeGetStringWithWhitespaceCheck(const JsonNode<wchar_t>& json_node, const TCHAR* key)
{
    std::wstring text = json_node.Get<std::wstring>(key);

    if( SO::IsWhitespace(text) )
        AddError(FormatTextCS2WS(_T("'%s' cannot be %s."), key, text.empty() ? _T("blank") : _T("whitespace")));

    return text;
}


std::shared_ptr<DocSetComponent> DocSetCompiler::JsonNodeGetDocumentWithSupportForFilenameOnly(const JsonNode<wchar_t>& json_node, DocSetSpec& doc_set_spec,
                                                                                               bool create_component_if_not_found, const TCHAR* document_type_for_error)
{
    // first search by absolute path
    std::wstring absolute_path = json_node.GetAbsolutePath();

    std::shared_ptr<DocSetComponent> doc_set_component = doc_set_spec.FindComponent(absolute_path, false);

    if( doc_set_component != nullptr )
        return doc_set_component;

    // if not found, search by the filename only
    const std::wstring filename_only = json_node.Get<std::wstring>();
    const std::vector<std::shared_ptr<DocSetComponent>>* doc_set_components_with_name = doc_set_spec.FindDocument(filename_only);

    if( doc_set_components_with_name != nullptr )
    {
        if( doc_set_components_with_name->size() != 1 )
        {
            AddError(FormatTextCS2WS(_T("The path '%s' is ambiguous. It could refer to '%s' or '%s'."),
                                     filename_only.c_str(),
                                     doc_set_components_with_name->front()->filename.c_str(),
                                     doc_set_components_with_name->at(1)->filename.c_str()));
            return nullptr;
        }

        return doc_set_components_with_name->front();
    }

    // if still not found, create a component (when possible)
    if( create_component_if_not_found && PortableFunctions::FileIsRegular(absolute_path) )
        return std::make_shared<DocSetComponent>(DocSetComponent::Type::Document, std::move(absolute_path));

    if( document_type_for_error != nullptr )
        AddError(FormatTextCS2WS(_T("The %s path is not part of the Document Set: %s"), document_type_for_error, absolute_path.c_str()));

    return nullptr;
}


template<typename CF>
void DocSetCompiler::JsonNodeForeachNode(const JsonNode<wchar_t>& json_node, const TCHAR* type, bool nodes_cannot_be_arrays_or_objects, CF callback_function)
{
    if( !EnsureJsonNodeIsObject(json_node, type) )
        return;

    json_node.ForeachNode(
        [&](std::wstring_view key_sv, const JsonNode<wchar_t>& attribute_value_node)
        {
            std::wstring key(key_sv);

            if( nodes_cannot_be_arrays_or_objects && ( attribute_value_node.IsArray() || attribute_value_node.IsObject() ) )
            {
                AddError(FormatTextCS2WS(_T("'%s' (part of '%s') cannot be specified as an array or object."), key.c_str(), type));
                return;
            }

            if( !attribute_value_node.IsNull() )
                callback_function(key, attribute_value_node);
        });
}


bool DocSetCompiler::CheckIfComponentsDoesNotContain(const DocSetSpec& doc_set_spec, DocSetComponent::Type doc_set_component_type)
{
    const auto& lookup = std::find_if(doc_set_spec.m_components.cbegin(), doc_set_spec.m_components.cend(),
                                      [&](const std::shared_ptr<DocSetComponent>& doc_set_component) { return ( doc_set_component_type == doc_set_component->type ); });

    if( lookup != doc_set_spec.m_components.cend() )
    {
        AddError(FormatTextCS2WS(_T("You cannot specify a '%s' when one has already been imported."), ToString(doc_set_component_type)));
        return false;
    }

    return true;
}


bool DocSetCompiler::CheckFileValidity(const DocSetComponent& doc_set_component, bool error_if_file_does_not_exist/* = true*/)
{
    if( !PortableFunctions::FileIsRegular(doc_set_component.filename) )
    {
        AddErrorOrWarning(error_if_file_does_not_exist, FormatTextCS2WS(_T("The '%s' file does not exist: %s"),
                                                                        ToString(doc_set_component.type), doc_set_component.filename.c_str()));
        return false;
    }

    const std::wstring extension = PortableFunctions::PathGetFileExtension(doc_set_component.filename);
    const bool extension_is_csdoc = SO::EqualsNoCase(extension, FileExtensions::CSDocument);

    if( doc_set_component.type == DocSetComponent::Type::Document )
    {
        if( !extension_is_csdoc )
        {
            AddError(FormatTextCS2WS(_T("The '%s' file must have the extension '%s': %s"),
                                     ToString(doc_set_component.type), FileExtensions::CSDocument, doc_set_component.filename.c_str()));
            return false;
        }
    }

    else if( extension_is_csdoc || SO::EqualsNoCase(extension, FileExtensions::CSDocumentSet) )
    {
        AddError(FormatTextCS2WS(_T("The '%s' file cannot have the extension '%s': %s"),
                                 ToString(doc_set_component.type), extension.c_str(), doc_set_component.filename.c_str()));
        return false;
    }

    return true;
}


bool DocSetCompiler::CheckIfDirectoryExists(const TCHAR* type, const std::wstring& path)
{
    if( PortableFunctions::FileIsDirectory(path) )
        return true;

    AddError(FormatTextCS2WS(_T("The %s directory does not exist: %s"), type, path.c_str()));

    return false;
}


bool DocSetCompiler::CheckIfOverridesWithNewValue(const TCHAR* type, const std::wstring& new_value, const std::wstring& old_value)
{
    if( old_value == new_value )
        return false;

    if( !old_value.empty() )
    {
        AddWarning(FormatTextCS2WS(_T("The %s has already been specified but this value ('%s') will override the previous value ('%s')."),
                                   type, new_value.c_str(), old_value.c_str()));
    }

    return true;
}


bool DocSetCompiler::AddComponent(DocSetSpec& doc_set_spec, std::shared_ptr<DocSetComponent> doc_set_component, bool error_if_file_does_not_exist/* = true*/)
{
    ASSERT(doc_set_component != nullptr);

    if( !CheckFileValidity(*doc_set_component, error_if_file_does_not_exist) )
        return false;

    std::shared_ptr<const DocSetComponent> already_added_doc_set_component = doc_set_spec.FindComponent(doc_set_component->filename, false);

    if( already_added_doc_set_component == nullptr )
    {
        doc_set_spec.AddComponent(std::move(doc_set_component));
        return true;
    }

    else if( already_added_doc_set_component->type == doc_set_component->type )
    {
        // ignore multiple entries of the same document
        if( doc_set_component->type != DocSetComponent::Type::Document )
        {
            AddWarning(FormatTextCS2WS(_T("The '%s' file has already been added and will be ignored: %s"),
                                       ToString(doc_set_component->type), doc_set_component->filename.c_str()));
        }
    }

    else
    {
        AddError(FormatTextCS2WS(_T("The '%s' file has already been added as a '%s' file: %s"),
                                 ToString(doc_set_component->type), ToString(already_added_doc_set_component->type), doc_set_component->filename.c_str()));
    }

    return false;
}


void DocSetCompiler::CompileSpec(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, SpecCompilationType spec_compilation_type)
{
    const bool compile_title                = true;

    const bool compiling_component_listing  = ( spec_compilation_type == SpecCompilationType::DataForTree ||
                                                spec_compilation_type == SpecCompilationType::SpecOnly ||
                                                spec_compilation_type == SpecCompilationType::SpecAndComponents );

    const bool compile_external_toc         = ( spec_compilation_type == SpecCompilationType::DataForTree ||
                                                spec_compilation_type == SpecCompilationType::DataForCSDocCompilation ||
                                                ( spec_compilation_type == SpecCompilationType::SpecOnly && JsonNodeContainsAndIsNotNull(json_node, JK::index) ) ||
                                                spec_compilation_type == SpecCompilationType::SpecAndComponents );
    const bool compile_inline_toc           = ( compile_external_toc ||
                                                spec_compilation_type == SpecCompilationType::SpecOnly );

    const bool compile_external_index       = ( spec_compilation_type == SpecCompilationType::SpecAndComponents );
    const bool compile_inline_index         = ( compile_external_index ||
                                                spec_compilation_type == SpecCompilationType::SpecOnly );
            
    const bool validate_toc_index_titles    = ( spec_compilation_type == SpecCompilationType::SpecOnly ||
                                                spec_compilation_type == SpecCompilationType::SpecAndComponents );

    const bool compile_external_settings    = ( spec_compilation_type != SpecCompilationType::SpecOnly );
    const bool compile_inline_settings      = true;

    const bool compile_external_definitions = ( spec_compilation_type == SpecCompilationType::DataForCSDocCompilation ||
                                                spec_compilation_type == SpecCompilationType::SpecAndComponents );
    const bool compile_inline_definitions   = ( compile_external_definitions ||
                                                spec_compilation_type == SpecCompilationType::SpecOnly );

    const bool compile_context_ids          = ( spec_compilation_type == SpecCompilationType::DataForCSDocCompilation ||
                                                spec_compilation_type == SpecCompilationType::SpecAndComponents );

    const bool compile_documents_node       = ( spec_compilation_type != SpecCompilationType::SettingsOnly );

    // reset any previously compiled values
    doc_set_spec.Reset();

    if( !EnsureJsonNodeIsObject(json_node, ToString(DocSetComponent::Type::Spec)) )
        return;

    // title
    if( compile_title && json_node.Contains(JK::title) )
        doc_set_spec.m_title = JsonNodeGetStringWithWhitespaceCheck(json_node, JK::title);

    // imported files
    if( json_node.Contains(JK::import) )
    {
        const auto& import_node = json_node.Get(JK::import);

        if( EnsureJsonNodeIsObject(import_node, JK::import) )
        {
            // imported table of contents
            if( ( compiling_component_listing || compile_external_toc ) && JsonNodeContainsAndIsNotNull(import_node, JK::tableOfContents) )
            {
                if( CheckIfComponentsDoesNotContain(doc_set_spec, DocSetComponent::Type::TableOfContents) )
                    AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::TableOfContents, import_node.GetAbsolutePath(JK::tableOfContents)));
            }

            // imported index
            if( ( compiling_component_listing || compile_external_index ) && JsonNodeContainsAndIsNotNull(import_node, JK::index) )
            {
                if( CheckIfComponentsDoesNotContain(doc_set_spec, DocSetComponent::Type::Index) )
                    AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::Index, import_node.GetAbsolutePath(JK::index)));
            }

            // imported settings
            if( ( compiling_component_listing || compile_external_settings ) && import_node.Contains(JK::settings) )
            {
                for( const auto& settings_node : import_node.GetArray(JK::settings) )
                    AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::Settings, settings_node.GetAbsolutePath()));
            }

            // imported definitions
            if( ( compiling_component_listing || compile_external_definitions ) && import_node.Contains(JK::definitions) )
            {
                for( const auto& definitions_node : import_node.GetArray(JK::definitions) )
                    AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::Definitions, definitions_node.GetAbsolutePath()));
            }

            // imported context IDs
            if( ( compiling_component_listing || compile_context_ids ) && import_node.Contains(JK::contextIds) )
            {
                for( const auto& context_ids_node : import_node.GetArray(JK::contextIds) )
                    ProcessContextIdsNode(doc_set_spec, context_ids_node);
            }
        }
    }

    // compile imported settings, definitions, and context IDs
    for( DocSetComponent& doc_set_component : VI_V(doc_set_spec.m_components) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Settings )
        {
            if( compile_external_settings )
                CompileSettings(GetComponentText(doc_set_component.filename).GetJsonNode(), doc_set_spec.m_settings, false);
        }

        else if( doc_set_component.type == DocSetComponent::Type::Definitions )
        {
            if( compile_external_definitions )
                CompileDefinitions(GetComponentText(doc_set_component.filename).GetJsonNode(), doc_set_spec.m_definitions);
        }

        else if( doc_set_component.type == DocSetComponent::Type::ContextIds )
        {
            if( compile_context_ids )
                CompileContextIds(GetComponentText(doc_set_component.filename).text, doc_set_spec.m_contextIds);
        }
    }

    // inline settings
    if( compile_inline_settings && json_node.Contains(JK::settings) )
    {
        CompileSettings(json_node.Get(JK::settings), doc_set_spec.m_settings, false);
    }

    // inline definitions
    if( compile_inline_definitions && json_node.Contains(JK::definitions) )
    {
        CompileDefinitions(json_node.Get(JK::definitions), doc_set_spec.m_definitions);
    }

    // documents
    if( compile_documents_node && json_node.Contains(JK::documents) )
    {
        for( const auto& documents_node : json_node.GetArray(JK::documents) )
            CompileDocumentsNode(doc_set_spec, documents_node);
    }

    // now that the full list of documents is available, as well as the settings, compile the imported table of contents and index
    // (the compilation of the table of contents can introduce more components, so a range-based for loop is not used)
    for( size_t i = 0; i < doc_set_spec.m_components.size(); ++i )
    {
        DocSetComponent& doc_set_component = *doc_set_spec.m_components[i];

        if( doc_set_component.type == DocSetComponent::Type::TableOfContents )
        {
            if( compile_external_toc )
            {
                ASSERT(!doc_set_spec.m_tableOfContents.has_value());
                doc_set_spec.m_tableOfContents = CompileTableOfContents(doc_set_spec, GetComponentText(doc_set_component.filename).GetJsonNode(), validate_toc_index_titles);
            }
        }

        else if( doc_set_component.type == DocSetComponent::Type::Index )
        {
            if( compile_external_index )
            {
                ASSERT(!doc_set_spec.m_index.has_value());
                doc_set_spec.m_index = CompileIndex(doc_set_spec, GetComponentText(doc_set_component.filename).GetJsonNode(), validate_toc_index_titles);
            }
        }
    }

    // inline table of contents
    if( compile_inline_toc && JsonNodeContainsAndIsNotNull(json_node, JK::tableOfContents) &&
        CheckIfComponentsDoesNotContain(doc_set_spec, DocSetComponent::Type::TableOfContents) )
    {
        ASSERT(!doc_set_spec.m_tableOfContents.has_value());
        doc_set_spec.m_tableOfContents = CompileTableOfContents(doc_set_spec, json_node.Get(JK::tableOfContents), validate_toc_index_titles);
    }

    // cover page document
    if( compile_documents_node && json_node.Contains(JK::coverPageDocument) )
    {
        doc_set_spec.m_coverPageDocument = JsonNodeGetDocumentWithSupportForFilenameOnly(json_node.Get(JK::coverPageDocument), doc_set_spec, true, _T("cover page document"));

        if( doc_set_spec.m_coverPageDocument != nullptr && !CheckFileValidity(*doc_set_spec.m_coverPageDocument) )
            doc_set_spec.m_coverPageDocument.reset();
    }

    // default document
    if( compile_documents_node && json_node.Contains(JK::defaultDocument) )
        doc_set_spec.m_defaultDocument = JsonNodeGetDocumentWithSupportForFilenameOnly(json_node.Get(JK::defaultDocument), doc_set_spec, false, _T("default document"));

    // inline index
    if( compile_inline_index && JsonNodeContainsAndIsNotNull(json_node, JK::index) &&
        CheckIfComponentsDoesNotContain(doc_set_spec, DocSetComponent::Type::Index) )
    {
        ASSERT(!doc_set_spec.m_index.has_value());
        doc_set_spec.m_index = CompileIndex(doc_set_spec, json_node.Get(JK::index), validate_toc_index_titles);
    }
}


void DocSetCompiler::CompileSpec(DocSetSpec& doc_set_spec, const std::wstring& text, SpecCompilationType spec_compilation_type)
{
    JsonReaderInterface json_reader_interface(PortableFunctions::PathGetDirectory(doc_set_spec.GetFilename()));
    CompileSpec(doc_set_spec, Json::Parse(text, &json_reader_interface), spec_compilation_type);
}


void DocSetCompiler::CompileSpecIfNecessary(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type)
{
    if( !SpecRequiresCompilation(doc_set_spec, spec_compilation_type) )
        return;

    const std::wstring text = GetFileTextOrModifiedIteration<std::wstring>(doc_set_spec.GetFilename());

    CompileSpec(doc_set_spec, text, spec_compilation_type);

    doc_set_spec.m_lastCompilationDetails.emplace(m_errorIssuer.index(), static_cast<int>(spec_compilation_type), GetTimestamp<int64_t>());
}


bool DocSetCompiler::SpecRequiresCompilation(DocSetSpec& doc_set_spec, SpecCompilationType spec_compilation_type) const
{
    // this is only called by a couple routines
    ASSERT(spec_compilation_type == DocSetCompiler::SpecCompilationType::DocumentsNodeAndSettingsOnly ||
           spec_compilation_type == DocSetCompiler::SpecCompilationType::DataForTree ||
           spec_compilation_type == DocSetCompiler::SpecCompilationType::DataForCSDocCompilation);

    // always compile if this is the first compilation or if the error issuer or compilation type is different
    if( !doc_set_spec.m_lastCompilationDetails.has_value() ||
        std::get<0>(*doc_set_spec.m_lastCompilationDetails) != m_errorIssuer.index() ||
        std::get<1>(*doc_set_spec.m_lastCompilationDetails) != static_cast<int>(spec_compilation_type) )
    {
        return true;
    }

    // conditionally compile based on whether components have been modified
    for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.m_components) )
    {
        bool compile_if_modified;

        switch( doc_set_component.type )
        {
            case DocSetComponent::Type::Spec:
            case DocSetComponent::Type::Settings:
                compile_if_modified = true;
                break;

            case DocSetComponent::Type::TableOfContents:
                compile_if_modified = ( spec_compilation_type == DocSetCompiler::SpecCompilationType::DataForTree ||
                                        spec_compilation_type == DocSetCompiler::SpecCompilationType::DataForCSDocCompilation );
                break;

            case DocSetComponent::Type::Definitions:
            case DocSetComponent::Type::ContextIds:
                compile_if_modified = ( spec_compilation_type == DocSetCompiler::SpecCompilationType::DataForCSDocCompilation );
                break;

            default:
                ASSERT(doc_set_component.type == DocSetComponent::Type::Index ||
                       doc_set_component.type == DocSetComponent::Type::Document);
                compile_if_modified = false;
                break;
        }

        if( compile_if_modified && std::get<2>(*doc_set_spec.m_lastCompilationDetails) < DocSetCompiler::GetFileTextOrModifiedIteration<int64_t>(doc_set_component.filename) )
            return true;
    }

    return false;
}


std::tuple<std::wstring, bool> DocSetCompiler::GetPathWithRecursiveOption(const JsonNode<wchar_t>& json_node)
{
    std::wstring path;
    bool recursive = false;

    if( json_node.IsObject() )
    {
        path = json_node.GetAbsolutePath(JK::path);
        recursive = json_node.GetOrDefault(JK::recursive, recursive);
    }

    else
    {
        path = json_node.GetAbsolutePath();
    }

    return { path, recursive };
}


void DocSetCompiler::CompileDocumentsNode(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node)
{
    // documents can be specified as files or directories
    auto [path, recursive] = GetPathWithRecursiveOption(json_node);

    if( PortableFunctions::FileIsDirectory(path) )
    {
        DirectoryLister directory_lister(recursive, true, false, true, true);
        directory_lister.SetNameFilter(FileExtensions::Wildcard::CSDocument);

        for( std::wstring& filename : directory_lister.GetPaths(path) )
            AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::Document, std::move(filename), true));
    }

    else
    {
        AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::Document, std::move(path), true));
    }
}


void DocSetCompiler::ProcessContextIdsNode(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node)
{
    std::wstring filename = json_node.GetAbsolutePath();

    // if a context ID file is not found...
    if( !PortableFunctions::FileIsRegular(filename) )
    {
        // ...see if it is part of the CSPro code directory
        if( m_globalSettings != nullptr && !m_globalSettings->cspro_code_path.empty() )
        {
            std::wstring test_filename = PortableFunctions::PathAppendToPath(m_globalSettings->cspro_code_path, PortableFunctions::PathToNativeSlash(json_node.Get<std::wstring>()));

            if( PortableFunctions::FileIsRegular(test_filename) )
                filename = std::move(test_filename);
        }
    }

    AddComponent(doc_set_spec, std::make_shared<DocSetComponent>(DocSetComponent::Type::ContextIds, std::move(filename)), false);
}


std::optional<DocSetTableOfContents> DocSetCompiler::CompileTableOfContents(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles)
{
    return DocSetTableOfContents::Compile(*this, doc_set_spec, json_node, validate_titles);
}


std::optional<DocSetIndex> DocSetCompiler::CompileIndex(DocSetSpec& doc_set_spec, const JsonNode<wchar_t>& json_node, bool validate_titles)
{
    return DocSetIndex::Compile(*this, doc_set_spec, json_node, validate_titles);
}


void DocSetCompiler::CompileSettings(const JsonNode<wchar_t>& json_node, DocSetSettings& doc_set_settings, bool reset_settings)
{
    doc_set_settings.Compile(*this, json_node, reset_settings);
}


void DocSetCompiler::CompileDefinitions(const JsonNode<wchar_t>& json_node, std::vector<std::tuple<std::wstring, std::wstring>>& definitions)
{
    JsonNodeForeachNode(json_node, ToString(DocSetComponent::Type::Definitions), true,
        [&](std::wstring key, const JsonNode<wchar_t>& attribute_value_node)
        {
            auto key_lookup = std::find_if(definitions.begin(), definitions.end(),
                [&](const std::tuple<std::wstring, std::wstring>& key_and_value) { return ( std::get<0>(key_and_value) == key ); });

            std::wstring value = attribute_value_node.Get<std::wstring>();

            if( key_lookup == definitions.cend() )
            {
                definitions.emplace_back(std::move(key), std::move(value));
            }

            else if( std::get<1>(*key_lookup) != value )
            {
                AddWarning(FormatTextCS2WS(_T("The definition '%s' has already been specified but this value ('%s') will override the previous value ('%s')."),
                                           key.c_str(), value.c_str(), std::get<1>(*key_lookup).c_str()));

                std::get<1>(*key_lookup) = std::move(value);
            }
        });
}


void DocSetCompiler::CompileContextIds(const std::wstring& resource_file_text, std::map<std::wstring, unsigned>& context_ids)
{
    std::wregex define_line_match_regex(LR"(^\s*#define\s+([a-zA-Z]\S+)\s+(\d+|0[xX][0-9a-fA-F]+)\s*$)");

    SO::ForeachLine(resource_file_text, false,
        [&](const std::wstring& line)
        {
            std::wsmatch matches;

            if( std::regex_match(line, matches, define_line_match_regex) )
            {
                ASSERT(matches.size() == 3);

                std::wstring name = matches.str(1);
                const std::wstring id_text = matches.str(2);

                try
                {
                    const int base = SO::StartsWithNoCase(id_text, _T("0x")) ? 16 : 10;
                    const unsigned id = std::stoul(id_text, nullptr, base);

                    auto name_lookup = context_ids.find(name);

                    if( name_lookup == context_ids.cend() )
                    {
                        context_ids.try_emplace(std::move(name), id);
                    }

                    else if( name_lookup->second != id )
                    {
                        AddWarning(FormatTextCS2WS(_T("The context ID '%s' has already been specified but this value ('%d') will override the previous value ('%d')."),
                                                   name.c_str(), static_cast<int>(id), static_cast<int>(name_lookup->second)));

                        name_lookup->second = id;
                    }
                }

                catch(...)
                {
                    AddError(FormatTextCS2WS(_T("The context ID '%s' is not valid: %s"), name.c_str(), id_text.c_str()));
                }
            }

            return true;
        });    
}


DocSetSettings DocSetCompiler::GetSettingsFromSpecOrSettingsFile(const std::wstring& filename, ErrorIssuerType error_issuer/* = ThrowErrors { }*/)
{
    const std::wstring file_text = FileIO::ReadText(filename);

    DocSetCompiler doc_set_compiler(error_issuer);

    // parse the file as if it were a Document Set...
    if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(filename), FileExtensions::CSDocumentSet) )
    {
        DocSetSpec doc_set_spec(filename);

        doc_set_compiler.CompileSpec(doc_set_spec, file_text, DocSetCompiler::SpecCompilationType::SettingsOnly);

        return doc_set_spec.m_settings;
    }

    // ...or a settings file
    else
    {
        JsonReaderInterface json_reader_interface(PortableFunctions::PathGetDirectory(filename));
        const auto json_node = Json::Parse(file_text, &json_reader_interface);

        DocSetSettings doc_set_settings;

        doc_set_settings.Compile(doc_set_compiler, json_node, false);

        return doc_set_settings;
    }
}
