#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ApplicationPackage.h>


// Convert requests/responses to/from server between JSON text and corresponding C++ classes.

class SYNC_API JsonConverter
{
public:
    // TODO: eventually remove the following and use ApplicationPackage's JSON serialization routines
    static std::string ToJson(const ApplicationPackage& package);
    static std::string ToJson(const std::vector<ApplicationPackage::File>& files);
    static std::vector<ApplicationPackage::File> CreateFileSpecListFromJson(const JsonNode& json_node);
    static ApplicationPackage CreateApplicationPackageFromJson(const JsonNode& json_node);
    static std::vector<ApplicationPackage> CreateApplicationPackageListFromJson(const JsonNode& json_node);

private:
    static void WriteFileSpecArray(JsonWriter& json_writer, const std::vector<ApplicationPackage::File>& files);
};
