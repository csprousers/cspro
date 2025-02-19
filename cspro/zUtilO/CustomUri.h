#pragma once

#include <zUtilO/zUtilO.h>

class ConnectionString;


class CLASS_DECL_ZUTILO CustomUri
{
public:
    constexpr static std::string_view CSProScheme_sv = "cspro://";

    // Returns true if the URI starts with http:// or https://.
    static bool UsesHttpScheme(std::string_view uri_sv);

    // Returns true if the URI starts with cspro://.
    static bool UsesCSProScheme(std::string_view uri_sv);

    // Returns true if the URI starts with http:// or https:// or cspro://.
    static bool UsesHttpOrCSProScheme(std::string_view uri_sv);

    // Returns the type of the URI using the CSPro scheme (if applicable).
    enum class UriType { Text, Data, Sync };
    static std::optional<UriType> GetUriType(std::string_view uri_sv);


    // --------------------------------------------------------------------------
    // Text URIs
    // --------------------------------------------------------------------------

    // Creates a URI that references a text file that looks like:
    //     - cspro://text/C:/Project/Report.txt
    // The URI Handler will open such URIs in Text Viewer.
    static std::string CreateTextUri(std::string file_path);

    // Returns the file path that is part of a text URI.
    static std::string ConvertTextUriToFilePath(std::string_view uri_sv);


    // --------------------------------------------------------------------------
    // Data URIs
    // --------------------------------------------------------------------------

    // Creates a URI that references a data source that looks like:
    //     - cspro://data/C:/Project/Survey2024.csdb
    //     - cspro://data/http://localhost/csweb/api?dictionaryPath=C%3A%2FProject%2FSurvey2024.dcf
    // The URI Handler will open such URIs in Data Manager.
    static std::string CreateDataUri(const ConnectionString& connection_string, const std::string* dictionary_file_path);

    // Adds to a data URI information about a case.
    static std::string AddCaseToDataUri(std::string data_uri, const std::string& key, const std::string* uuid);

    // Returns the connection string text that includes the resource and properties that are part of a data URI.
    static std::string ConvertDataUriToConnectionStringText(std::string_view uri_sv);


    // --------------------------------------------------------------------------
    // Sync URIs
    // --------------------------------------------------------------------------

    // Returns the sync connection string text that includes the resource and properties that are part of a sync URI.
    static std::string ConvertSyncUriToSyncConnectionStringText(std::string_view uri_sv);


private:
    struct UriBuilder;

    static std::string EvaluatePathAndQueryString(std::string_view path_sv, std::vector<std::tuple<std::string, std::string>>* query_string_properties);

    static std::string EncodeFilePath(std::string file_path);

    static std::string ConvertUriToPropertyString(std::string_view cspro_scheme_prefix_sv, std::string_view uri_sv);

    static UriBuilder CreateUriForConnectionString(const ConnectionString& connection_string);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool CustomUri::UsesCSProScheme(const std::string_view uri_sv)
{
    return SO::StartsWith(uri_sv, CSProScheme_sv);
}
