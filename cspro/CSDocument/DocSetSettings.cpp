#include "StdAfx.h"
#include "DocSetSettings.h"


void DocSetSettings::Reset()
{
    m_projectRootDirectory.clear();
    m_imageDirectories.clear();
    m_defaultBuildSettings.reset();
    m_namedBuildSettings.clear();
}


bool DocSetSettings::HasCustomSettings() const
{
    return ( !m_projectRootDirectory.empty() ||
             !m_imageDirectories.empty() ||
             m_defaultBuildSettings.has_value() ||
             !m_namedBuildSettings.empty() );
}


std::string DocSetSettings::FindProjectDocSetSpecFilePath(const std::string& project) const
{
    if( m_projectRootDirectory.empty() )
        throw CSProException("No project root directory has been set so the project '%s' cannot be found.", project.c_str());

    const std::string project_directory = Path::Combine(m_projectRootDirectory, project);

    return CacheableCalculator::FindProjectDocSetSpecFilePath(project_directory);
}


std::string DocSetSettings::GetProjectNameFromDocSetSpecFilePath(std::string_view project_doc_set_spec_file_path_sv) const
{
    if( !m_projectRootDirectory.empty() && SO::StartsWithNoCase(project_doc_set_spec_file_path_sv, m_projectRootDirectory) )
    {
        project_doc_set_spec_file_path_sv.remove_prefix(m_projectRootDirectory.length());

        if( !project_doc_set_spec_file_path_sv.empty() && Path::IsSlashChar(project_doc_set_spec_file_path_sv.front()) )
            project_doc_set_spec_file_path_sv.remove_prefix(1);

        // the project name is the name of the first directory
        const size_t slash_pos = project_doc_set_spec_file_path_sv.find_first_of(Path::SlashChars_sv);

        if( slash_pos != std::string_view::npos )
            return std::string(project_doc_set_spec_file_path_sv.substr(0, slash_pos));
    }

    return std::string();
}


const std::string* DocSetSettings::FindInImageDirectories(const std::string& image_name) const
{
    const std::string* image_path = nullptr;

    for( const ImageDirectory& image_directory : m_imageDirectories )
        image_path = CacheableCalculator::FindFileByNameInDirectory(image_directory.directory, image_directory.recursive, image_name, image_path);

    return image_path;
}


DocBuildSettings DocSetSettings::GetEvaluatedBuildSettings(const DocBuildSettings& build_settings) const
{
    // if there are default build settings, apply the other build settings on top of them
    if( m_defaultBuildSettings.has_value() )
        return DocBuildSettings::ApplySettings(*m_defaultBuildSettings, build_settings);

    return build_settings;
}


DocBuildSettings DocSetSettings::GetEvaluatedBuildSettings(const std::string& name) const
{
    for( const auto& [this_name, build_settings] : m_namedBuildSettings )
    {
        if( SO::EqualsNoCase(name, this_name) )
            return GetEvaluatedBuildSettings(build_settings);
    }

    throw CSProException("There are no build settings with the name: %s", name.c_str());
}


std::tuple<DocBuildSettings, std::string> DocSetSettings::GetEvaluatedBuildSettings(const DocBuildSettings::BuildType build_type, const std::string& name/* = SO::Empty_string*/) const
{
    // search by name
    if( !name.empty() )
    {
        try
        {
            return { GetEvaluatedBuildSettings(name), name };
        }
        catch(...) { }
    }

    // search by build type
    for( const auto& [this_name, build_settings] : m_namedBuildSettings )
    {
        if( build_settings.GetBuildType() == build_type )
            return { GetEvaluatedBuildSettings(build_settings), this_name };
    }

    if( m_defaultBuildSettings.has_value() && m_defaultBuildSettings->GetBuildType() == build_type )
        return { *m_defaultBuildSettings, std::string() };

    // return the default settings for the build type
    return { GetEvaluatedBuildSettings(DocBuildSettings::DefaultSettingsForBuildType(build_type)), std::string() };
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

void DocSetSettings::Compile(DocSetCompiler& doc_set_compiler, const JsonNode& json_node, const bool reset_settings)
{
    if( reset_settings )
        Reset();

    if( !doc_set_compiler.EnsureJsonNodeIsObject(json_node, ToString(DocSetComponent::Type::Settings)) )
        return;

    // project root
    if( json_node.Contains(JK::projectRootDirectory) )
    {
        std::string path = json_node.GetAbsolutePath(JK::projectRootDirectory);

        if( doc_set_compiler.CheckIfDirectoryExists("project root", path) &&
            doc_set_compiler.CheckIfOverridesWithNewValue("project root directory", path, m_projectRootDirectory) )
        {
            m_projectRootDirectory = std::move(path);
        }
    }

    // image directories
    if( json_node.Contains(JK::imageDirectories) )
    {
        for( const JsonNode& image_directory_node : json_node.GetArray(JK::imageDirectories) )
        {
            auto [path, recursive] = doc_set_compiler.GetPathWithRecursiveOption(image_directory_node);

            if( !doc_set_compiler.CheckIfDirectoryExists("image", path) )
                continue;

            auto lookup = std::find_if(m_imageDirectories.begin(), m_imageDirectories.end(),
                [&path_check = std::as_const(path)](const ImageDirectory& image_directory)
                {
                    return SO::EqualsNoCase(path_check, image_directory.directory);
                });

            if( lookup != m_imageDirectories.cend() )
            {
                // if the image directory was already added, make sure the recursive flag reflects the msot permissive setting
                lookup->recursive |= recursive;
            }

            else
            {
                m_imageDirectories.emplace_back(DocSetSettings::ImageDirectory { std::move(path), recursive } );
            }
        }
    }

    // default build settings
    if( m_defaultBuildSettings.has_value() )
    {
        m_defaultBuildSettings->Compile(doc_set_compiler, json_node);
    }

    else
    {
        // if not compiling on top a previously-compiled object, see if there are any default build settings
        DocBuildSettings build_settings;
        build_settings.Compile(doc_set_compiler, json_node);

        if( build_settings.HasCustomSettings() )
            m_defaultBuildSettings = std::move(build_settings);
    }

    // named build settings
    if( json_node.Contains(JK::builds) )
    {
        const JsonNode builds_node = json_node.Get(JK::builds);

        if( doc_set_compiler.EnsureJsonNodeIsArray(builds_node, JK::builds) )
        {
            for( const JsonNode& build_node : builds_node.GetArray() )
            {
                std::string name = doc_set_compiler.JsonNodeGetStringWithWhitespaceCheck(build_node, JK::name);

                DocBuildSettings* named_build_settings = nullptr;

                for( auto& [existing_name, existing_build_settings] : m_namedBuildSettings )
                {
                    if( SO::EqualsNoCase(name, existing_name) )
                    {
                        named_build_settings = &existing_build_settings;
                        break;
                    }
                }

                // add new named build settings if they do not already exist
                if( named_build_settings == nullptr )
                    named_build_settings = &std::get<1>(m_namedBuildSettings.emplace_back(std::move(name), DocBuildSettings()));

                named_build_settings->Compile(doc_set_compiler, build_node);
            }
        }
    }
}


void DocSetSettings::WriteJson(JsonWriter& json_writer, const bool write_evaluated_build_settings/* = true*/) const
{
    json_writer.BeginObject();

    if( !m_projectRootDirectory.empty() )
        json_writer.WriteRelativePathWithDirectorySupport(JK::projectRootDirectory, m_projectRootDirectory);

    json_writer.WriteArrayIfNotEmpty(JK::imageDirectories, m_imageDirectories,
        [&](const DocSetSettings::ImageDirectory& image_directory)
        {
            if( image_directory.recursive )
            {
                json_writer.BeginObject()
                           .WriteRelativePathWithDirectorySupport(JK::path, image_directory.directory)
                           .Write(JK::recursive, image_directory.recursive)
                           .EndObject();
            }

            else
            {
                json_writer.WriteRelativePathWithDirectorySupport(image_directory.directory);
            }
        });

    if( m_defaultBuildSettings.has_value() )
        m_defaultBuildSettings->WriteJson(json_writer, false);

    json_writer.WriteArrayIfNotEmpty(JK::builds, m_namedBuildSettings,
        [&](const std::tuple<std::string, DocBuildSettings>& name_and_build_settings)
        {
            json_writer.BeginObject()
                       .Write(JK::name, std::get<0>(name_and_build_settings));

            const DocBuildSettings& build_settings = std::get<1>(name_and_build_settings);

            if( write_evaluated_build_settings && m_defaultBuildSettings.has_value() )
            {
                GetEvaluatedBuildSettings(build_settings).WriteJson(json_writer, false);
            }

            else
            {
                build_settings.WriteJson(json_writer, false);
            }

            json_writer.EndObject();
        });

    json_writer.EndObject();
}
