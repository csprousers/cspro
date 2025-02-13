#pragma once

#include <zToolsO/StringNoCase.h>


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
        std::wstring name;
        int id;
    };

    struct ResourceFilenames
    {
        std::wstring resource;
        std::wstring header;
        std::wstring shared_header;
    };

public:
    Numberer(const std::wstring& definitions_filename, bool only_process_recent_changes);

    void Run();

private:
    void ReadDefinitionsFile(const std::wstring& definitions_filename);    
   
    static std::vector<std::wstring> ReadIconNamesFromResourceFile(const std::wstring& resource_file_contents);
    
    static std::vector<std::shared_ptr<ResourceId>> ReadResourceIdsFromHeader(const std::wstring& header_contents);
    
    static bool ResourceIdSorter(const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs);
    
    static ResourceIdRange RenumberResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, ResourceIdRange resource_id_range);
    
    static void ArrangeResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, const std::vector<std::wstring>& ordered_names);

    static void SortAndWriteResourceIds(const ResourceFilenames& resource_filenames, bool shared_ids, 
                                        std::vector<std::shared_ptr<ResourceId>>& resource_ids, 
                                        const ResourceIdRange& resource_id_range,
                                        const std::wstring& initial_header_contents);

    void ProcessFiles(const ResourceFilenames& resource_filenames, const ResourceIdRange& resource_id_range, 
                      const std::vector<std::vector<std::wstring>>* ordered_ranges);

private:
    bool m_onlyProcessRecentChanges;
    std::wstring m_codeRoot;
    std::vector<std::wstring> m_resourceExclusions;
    std::map<StringNoCase, ResourceIdRange> m_projectResourceIdRanges;
    std::map<StringNoCase, std::vector<std::vector<std::wstring>>> m_projectOrderedRanges;
};
