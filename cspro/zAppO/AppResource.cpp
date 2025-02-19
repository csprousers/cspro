#include "stdafx.h"
#include "AppResource.h"
#include <zUtilO/SpecialDirectoryLister.h>


CREATE_JSON_KEY(includeInCompiledApplication)


AppResource::AppResource(std::string path, const bool include_in_compiled_application/* = true*/,
                         const bool recursive/* = true*/, std::string filter/* = std::string()*/)
    :   m_path(std::move(path)),
        m_includeInCompiledApplication(include_in_compiled_application),
        m_recursive(recursive),
        m_filenameFilter(std::move(filter))
{
    ASSERT(!m_path.empty() && !Path::IsSlashChar(m_path.back()));
}


AppResource::AppResource()
    :   m_includeInCompiledApplication(true),
        m_recursive(true)
{
    // this should never be called explicitly but allows serialization routines to work properly
}


bool AppResource::operator==(const AppResource& rhs) const
{
    return ( SO::EqualsNoCase(m_path, rhs.m_path) );
}


bool AppResource::IsDirectory() const
{
    return PortableFunctions::FileIsDirectory(m_path);
}


std::vector<std::string> AppResource::GetEvaluatedPaths(const bool include_directories) const
{
    if( IsDirectory() )
    {
        constexpr bool include_files = true;
        constexpr bool include_trailing_slash_on_directories = false;
        constexpr bool filter_directories = false;

        DirectoryLister directory_lister(m_recursive, include_files, include_directories,
                                         include_trailing_slash_on_directories, filter_directories);

        if( !SO::IsWhitespace(m_filenameFilter) )
            directory_lister.SetNameFilter(SpecialDirectoryLister::EvaluateFilter(m_filenameFilter));

        return directory_lister.GetPaths(m_path);
    }

    else
    {
        return { m_path };
    }
}


AppResource AppResource::CreateFromJson(const JsonNode& json_node)
{
    // prior to CSPro 8.1, only directories were allowed, which were written as strings, not objects
    if( json_node.IsString() )
        return AppResource(json_node.GetAbsolutePath());

    return AppResource(json_node.GetAbsolutePath(JK::path),
                       json_node.GetOrDefault(JK::includeInCompiledApplication, true),
                       json_node.GetOrDefault(JK::recursive, true),
                       json_node.GetOrConstruct<std::string>(JK::filter));
}


void AppResource::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .WriteRelativePath(JK::path, m_path)
               .Write(JK::includeInCompiledApplication, m_includeInCompiledApplication);

    if( IsDirectory() )
    {
        json_writer.Write(JK::recursive, m_recursive)
                   .WriteIfNotBlank(JK::filter, m_filenameFilter);
    }

    json_writer.EndObject();
}


void AppResource::serialize(Serializer& ar)
{
    ASSERT(ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1));

    ar & m_path;
}
