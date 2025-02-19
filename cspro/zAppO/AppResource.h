#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/TextSource.h>


// --------------------------------------------------------------------------
// AppResource
// --------------------------------------------------------------------------

class ZAPPO_API AppResource
{
public:
    AppResource(std::string path, bool include_in_compiled_application = true, bool recursive = true, std::string filter = std::string());
    AppResource();

    bool operator==(const AppResource& rhs) const;
    bool operator!=(const AppResource& rhs) const { return !operator==(rhs); }

    const std::string& GetPath() const { return m_path; }

    bool IsDirectory() const;
    bool IsFile() const { return !IsDirectory(); }

    bool GetIncludeInCompiledApplication() const    { return m_includeInCompiledApplication; }
    void SetIncludeInCompiledApplication(bool flag) { m_includeInCompiledApplication = flag; }

    bool GetRecursive() const    { return m_recursive; }
    void SetRecursive(bool flag) { m_recursive = flag; }

    const std::string& GetFilenameFilter() const { return m_filenameFilter; }
    void SetFilenameFilter(std::string filter)   { m_filenameFilter = std::move(filter); }

    // If including directories, directory paths are returned without a trailing slash.
    // An exception is thrown if the filename filter is not valid.
    std::vector<std::string> GetEvaluatedPaths(bool include_directories) const;

    // serialization
    // --------------------------------------------------------------------------
    static AppResource CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::string m_path;
    bool m_includeInCompiledApplication;

    // for directories only:
    bool m_recursive;
    std::string m_filenameFilter;
};
