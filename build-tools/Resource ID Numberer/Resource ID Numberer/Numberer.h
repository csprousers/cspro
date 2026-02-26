#pragma once


class Numberer
{
private:
    struct ResourceIdRange
    {
        int resource;
        int command;
        int control;
    };

    struct ResourceId
    {
        std::string name;
        int id;
    };

    struct ResourceFilePaths
    {
        std::string resource;
        std::string header;
        std::string shared_header;
    };

public:
    Numberer(const std::string& definitions_file_path, bool only_process_recent_changes);

    void Run();

private:
    void ReadDefinitionsFile(const std::string& definitions_file_path);

    void Run(const std::string& code_root);

    static std::vector<std::string> ReadIconNamesFromResourceFile(const std::string& resource_file_contents);

    static std::vector<std::shared_ptr<ResourceId>> ReadResourceIdsFromHeader(const std::string& header_contents);

    static bool ResourceIdSorter(const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs);

    static ResourceIdRange RenumberResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, ResourceIdRange resource_id_range);

    static void ArrangeResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, const std::vector<std::string>& ordered_names);

    static void SortAndWriteResourceIds(const ResourceFilePaths& resource_file_paths, bool shared_ids,
                                        std::vector<std::shared_ptr<ResourceId>>& resource_ids,
                                        const ResourceIdRange& resource_id_range,
                                        const std::string& initial_header_contents);

    void ProcessFiles(const ResourceFilePaths& resource_file_paths, const ResourceIdRange& resource_id_range,
                      const std::vector<std::vector<std::string>>* ordered_ranges);

private:
    bool m_onlyProcessRecentChanges;
    std::vector<std::string> m_codeRoots;
    std::vector<std::string> m_resourceExclusions;
    std::map<std::string, ResourceIdRange> m_projectResourceIdRanges; // project names added in lowercase
    std::map<std::string, std::vector<std::vector<std::string>>> m_projectOrderedRanges;
};
