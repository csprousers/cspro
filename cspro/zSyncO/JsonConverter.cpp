#include "stdafx.h"
#include "JsonConverter.h"


std::string JsonConverter::ToJson(const ApplicationPackage& package)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

    json_writer->BeginObject()
                .Write(JK::name, package.GetName())
                .Write(JK::description, package.GetDescription())
                .WriteDate("buildTime", package.GetBuildTime());

    json_writer->Key(JK::files);
    WriteFileSpecArray(*json_writer, package.GetFiles());

    json_writer->EndObject();

    return json_writer->ReleaseString();
}


std::string JsonConverter::ToJson(const std::vector<ApplicationPackage::File>& files)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

    WriteFileSpecArray(*json_writer, files);

    return json_writer->ReleaseString();
}


void JsonConverter::WriteFileSpecArray(JsonWriter& json_writer, const std::vector<ApplicationPackage::File>& files)
{
    json_writer.BeginArray();

    for( const ApplicationPackage::File& file : files )
    {
        json_writer.BeginObject()
                   .Write(JK::path, file.path)
                   .WriteIfNotBlank(JK::signature, file.signature)
                   .WriteIfNot("onlyOnFirstInstall", file.only_on_first_install, false)
                   .EndObject();
    }

    json_writer.EndArray();
}


ApplicationPackage JsonConverter::CreateApplicationPackageFromJson(const JsonNode& json_node)
{
    std::string name = json_node.Get<std::string>(JK::name);
    std::string description = json_node.Get<std::string>(JK::description);
    const int64_t build_time = json_node.Contains("buildTime") ? json_node.GetDate("buildTime") : 0;

    ApplicationPackage::DeploymentType deploy_type = ApplicationPackage::DeploymentType::None;
    std::string url;

    if( json_node.Contains("deployment") )
    {
        const JsonNode deployment_json_node = json_node.Get("deployment");

        if( deployment_json_node.Contains(JK::type) )
        {
            const std::string_view deploy_type_string_sv = deployment_json_node.Get<std::string_view>(JK::type);

            if( deploy_type_string_sv == "CSWeb" )
            {
                deploy_type = ApplicationPackage::DeploymentType::CSWeb;
                url = deployment_json_node.GetOrConstruct<std::string>("cswebUrl");
            }

            else if( deploy_type_string_sv == "FTP" )
            {
                deploy_type = ApplicationPackage::DeploymentType::FTP;
                url = deployment_json_node.GetOrConstruct<std::string>("ftpUrl");

            }

            else if( deploy_type_string_sv == "Dropbox" )
            {
                deploy_type = ApplicationPackage::DeploymentType::Dropbox;
            }

            else if( deploy_type_string_sv == "LocalFile" )
            {
                deploy_type = ApplicationPackage::DeploymentType::LocalFile;
            }

            else if( deploy_type_string_sv == "LocalFolder" )
            {
                deploy_type = ApplicationPackage::DeploymentType::LocalFolder;
            }
        }
    }

    std::vector<ApplicationPackage::File> files = json_node.Contains(JK::files) ? CreateFileSpecListFromJson(json_node.Get(JK::files)) :
                                                                                  std::vector<ApplicationPackage::File>();

    std::vector<ApplicationPackage::Dictionary> dictionaries;

    if( json_node.Contains(JK::dictionaries) )
    {
        for( const JsonNode& dictionary_json_node : json_node.GetArray(JK::dictionaries) )
        {
            dictionaries.emplace_back(ApplicationPackage::Dictionary
                {
                    dictionary_json_node.Get<std::string>(JK::path),
                    dictionary_json_node.GetOrDefault<bool>("uploadForSync", false)
                });
        }
    }

    return ApplicationPackage(std::move(name), std::move(description), build_time, deploy_type, std::move(url), std::move(files), std::move(dictionaries));
}


std::vector<ApplicationPackage> JsonConverter::CreateApplicationPackageListFromJson(const JsonNode& json_node)
{
    std::vector<ApplicationPackage> packages;

    for( const JsonNode& package_json_node : json_node.GetArray() )
        packages.emplace_back(CreateApplicationPackageFromJson(package_json_node));

    return packages;
}


std::vector<ApplicationPackage::File> JsonConverter::CreateFileSpecListFromJson(const JsonNode& json_node)
{
    std::vector<ApplicationPackage::File> files;

    for( const JsonNode& file_json_node : json_node.GetArray() )
    {
        files.emplace_back(ApplicationPackage::File
            {
                file_json_node.Get<std::string>(JK::path),
                file_json_node.GetOrConstruct<std::string>(JK::signature),
                file_json_node.GetOrDefault("onlyOnFirstInstall", false)
            });
    }

    return files;
}
