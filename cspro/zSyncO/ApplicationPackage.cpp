#include "stdafx.h"
#include "ApplicationPackage.h"
#include <zJson/JsonSpecFile.h>


CREATE_JSON_KEY(buildTime)
CREATE_JSON_KEY(cswebUrl)
CREATE_JSON_KEY(deployment)
CREATE_JSON_KEY(ftpUrl)
CREATE_JSON_KEY(onlyOnFirstInstall)
CREATE_JSON_KEY(uploadForSync)

CREATE_JSON_VALUE(deployment)

CREATE_ENUM_JSON_SERIALIZER(ApplicationPackage::DeploymentType,
    { ApplicationPackage::DeploymentType::None,        "None" },
    { ApplicationPackage::DeploymentType::CSWeb,       "CSWeb" },
    { ApplicationPackage::DeploymentType::Dropbox,     "Dropbox" },
    { ApplicationPackage::DeploymentType::FTP,         "FTP"},
    { ApplicationPackage::DeploymentType::LocalFile,   "LocalFile" },
    { ApplicationPackage::DeploymentType::LocalFolder, "LocalFolder" })


// --------------------------------------------------------------------------
// ApplicationPackage
// --------------------------------------------------------------------------

ApplicationPackage::ApplicationPackage(std::string name, std::string description, const int64_t build_time,
                                       const DeploymentType deployment_type, std::string server_url,
                                       std::vector<File> files, std::vector<Dictionary> dictionaries)
    :   m_name(std::move(name)),
        m_description(std::move(description)),
        m_buildTime(build_time),
        m_installedVersionBuildTime(0),
        m_deploymentType(deployment_type),
        m_serverUrl(std::move(server_url)),
        m_files(std::move(files)),
        m_dictionaries(std::move(dictionaries))
{
}


void ApplicationPackage::PrepareBuild()
{
    SetBuildTime(GetTimestamp());

    for( File& file : m_files )
    {
        file.signature = PortableFunctions::FileMd5(file.path);

        if( file.signature.empty() )
            throw FileIO::Exception::FileNotFound(file.path);
    }
}


ApplicationPackage ApplicationPackage::CreateFromJson(const JsonNode& json_node)
{
    std::string name = json_node.Get<std::string>(JK::name);
    std::string description = json_node.Get<std::string>(JK::description);

    const int64_t build_time = json_node.Contains(JK::buildTime) ? json_node.GetDate(JK::buildTime) : 0;

    const JsonNode deployment_node = json_node.GetOrEmpty(JK::deployment);
    DeploymentType deployment_type = DeploymentType::None;
    std::string server_url;

    if( !deployment_node.IsEmpty() && deployment_node.Contains(JK::type) )
    {
        deployment_type = deployment_node.Get<DeploymentType>(JK::type);

        if( deployment_type == DeploymentType::CSWeb )
        {
            server_url = deployment_node.GetOrConstruct<std::string>(JK::cswebUrl);
        }

        else if( deployment_type == DeploymentType::FTP )
        {
            server_url = deployment_node.GetOrConstruct<std::string>(JK::ftpUrl);
        }
    }

    const JsonNode files_node = json_node.GetOrEmpty(JK::files);
    const JsonNode dictionaries_node = json_node.GetOrEmpty(JK::dictionaries);

    return ApplicationPackage(std::move(name),
                              std::move(description),
                              build_time, deployment_type,
                              std::move(server_url),
                              !files_node.IsEmpty() ? files_node.GetArray().GetVector<File>() : std::vector<File>(),
                              !dictionaries_node.IsEmpty() ? dictionaries_node.GetArray().GetVector<Dictionary>() : std::vector<Dictionary>());
}


void ApplicationPackage::WriteJson(JsonWriter& json_writer, const bool spec_file_format/* = false*/) const
{
    json_writer.BeginObject();

    if( spec_file_format )
        JsonSpecFile::WriteHeading(json_writer, JV::deployment);

    json_writer.Write(JK::name, m_name)
               .Write(JK::description, m_description);

    if( m_buildTime != 0 )
        json_writer.WriteDate(JK::buildTime, m_buildTime);

    json_writer.Write(JK::files, m_files);
    json_writer.Write(JK::dictionaries, m_dictionaries);

    // deployment
    {
        json_writer.BeginObject(JK::deployment)
                   .Write(JK::type, m_deploymentType);

        if( m_deploymentType == DeploymentType::CSWeb )
        {
            json_writer.WriteIfNotBlank(JK::cswebUrl, m_serverUrl);
        }

        else if( m_deploymentType == DeploymentType::FTP )
        {
            json_writer.WriteIfNotBlank(JK::ftpUrl, m_serverUrl);
        }

        json_writer.EndObject();
    }

    json_writer.EndObject();
}


std::string ApplicationPackage::GetJson(const std::string& root_directory, const bool spec_file_format/* = false*/) const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriterWithRelativePaths(Path::Combine(root_directory, "g"));
    WriteJson(*json_writer, spec_file_format);
    return json_writer->ReleaseString();
}



// --------------------------------------------------------------------------
// ApplicationPackage::File
// --------------------------------------------------------------------------

ApplicationPackage::File ApplicationPackage::File::CreateFromJson(const JsonNode& json_node)
{
    return File
    {
        json_node.GetAbsolutePath(JK::path),
        json_node.GetOrConstruct<std::string>(JK::signature),
        json_node.GetOrDefault(JK::onlyOnFirstInstall, false)
    };
}


void ApplicationPackage::File::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .WriteRelativePath(JK::path, path)
               .WriteIfNotBlank(JK::signature, signature)
               .WriteIfNot(JK::onlyOnFirstInstall, only_on_first_install, false)
               .EndObject();
}


// --------------------------------------------------------------------------
// ApplicationPackage::Dictionary
// --------------------------------------------------------------------------

ApplicationPackage::Dictionary ApplicationPackage::Dictionary::CreateFromJson(const JsonNode& json_node)
{
    return Dictionary
    {
        json_node.GetAbsolutePath(JK::path),
        json_node.GetOrDefault(JK::uploadForSync, false)
    };
}


void ApplicationPackage::Dictionary::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .WriteRelativePath(JK::path, path)
               .WriteIfNot(JK::uploadForSync, upload_for_sync, false)
               .EndObject();
}
