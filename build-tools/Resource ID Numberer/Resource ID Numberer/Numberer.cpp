#include "stdafx.h"
#include "Numberer.h"
#include <zToolsO/DirectoryLister.h>
#include <zJson/JsonSpecFile.h>
#include <iostream>
#include <regex>


Numberer::Numberer(const std::string& definitions_file_path, const bool only_process_recent_changes)
    :   m_onlyProcessRecentChanges(only_process_recent_changes)
{
    ReadDefinitionsFile(definitions_file_path);
}


void Numberer::ReadDefinitionsFile(const std::string& definitions_file_path)
{
    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(definitions_file_path);

    m_codeRoot = json_reader->GetAbsolutePath("codeRoot");

    for( const JsonNode& exclusion_node : json_reader->GetArrayOrEmpty("resourceExclusions") )
        m_resourceExclusions.emplace_back(MakeFullPath(m_codeRoot, exclusion_node.Get<std::string>()));

    json_reader->Get("projects").ForeachNode(
        [&](const std::string_view project_name_sv, const JsonNode& ranges_node)
        {
            m_projectResourceIdRanges.try_emplace(SO::ToLower(project_name_sv),
                ResourceIdRange
                {
                    ranges_node.Get<int>("resource"),
                    ranges_node.Get<int>("command"),
                    ranges_node.Get<int>("control")
                });
        });

    json_reader->GetOrEmpty("orderedRanges").ForeachNode(
        [&](const std::string_view project_name_sv, const JsonNode& ranges_node)
        {
            std::vector<std::vector<std::string>> project_ranges;

            for( const JsonNode& range_node : ranges_node.GetArray() )
                project_ranges.emplace_back(range_node.Get<std::vector<std::string>>());

            m_projectOrderedRanges.try_emplace(SO::ToLower(project_name_sv), std::move(project_ranges));
        });
}


void Numberer::Run()
{
    const std::vector<std::string> listed_resource_file_paths = DirectoryLister().SetRecursive()
                                                                                 .SetNameFilter("*.rc")
                                                                                 .GetPaths(m_codeRoot);

    if( listed_resource_file_paths.empty() )
        throw CSProException("No resource files exist in: " + m_codeRoot);

    for( const std::string& resource_file_path : listed_resource_file_paths )
    {
        // skip excluded files
        const auto& exclusion_lookup = std::find_if(m_resourceExclusions.cbegin(), m_resourceExclusions.cend(),
                                                    [&](const std::string& exclusion_file_path) { return SO::EqualsNoCase(exclusion_file_path, resource_file_path); });

        if( exclusion_lookup != m_resourceExclusions.cend() )
            continue;

        const std::string resource_directory = PortableFunctions::PathGetDirectory(resource_file_path);
        const std::string project_name = PortableFunctions::PathGetFilename(PortableFunctions::PathRemoveTrailingSlash(resource_directory));

        ResourceFilePaths resource_file_paths
        {
            resource_file_path,
            Path::Combine(resource_directory, "resource.h"),
            Path::Combine(resource_directory, "resource_shared.h")
        };

        if( !PortableFunctions::FileIsRegular(resource_file_paths.header) )
            throw CSProException("Resource header does not exist: " + resource_file_paths.header);

        if( !PortableFunctions::FileIsRegular(resource_file_paths.shared_header) )
            resource_file_paths.shared_header.clear();

        // skip old files if only processing recent changes
        if( m_onlyProcessRecentChanges )
        {
            // consider "recent" to mean anything within the last eight hours
            const int64_t earliest_timestamp = GetTimestamp() - DateHelper::SecondsInHour(8);

            if( ( PortableFunctions::FileModifiedTime(resource_file_paths.header) < earliest_timestamp ) &&
                ( resource_file_paths.shared_header.empty() || PortableFunctions::FileModifiedTime(resource_file_paths.shared_header) < earliest_timestamp ) )
            {
                std::wcout << L"Skipping " << TC::ToWide(resource_file_paths.header).c_str() << std::endl;
                continue;
            }
        }

        std::wcout << L"Processing " << TC::ToWide(resource_file_paths.header).c_str() << std::endl;

        // get the resource ID range set for this project
        auto resource_id_range_lookup = m_projectResourceIdRanges.find(SO::ToLower(project_name));

        if( resource_id_range_lookup == m_projectResourceIdRanges.cend() )
        {
            resource_id_range_lookup = m_projectResourceIdRanges.find("default");

            if( resource_id_range_lookup == m_projectResourceIdRanges.cend() )
                throw CSProException("No resource ID ranges defined for '%s' and no default ranges exist", project_name.c_str());
        }

        // determine if there are any ordered ranges for this project
        const auto& ordered_ranges_lookup = m_projectOrderedRanges.find(SO::ToLower(project_name));
        const std::vector<std::vector<std::string>>* ordered_ranges = ( ordered_ranges_lookup != m_projectOrderedRanges.cend() ) ? &ordered_ranges_lookup->second :
                                                                                                                                   nullptr;

        // process the files
        ProcessFiles(resource_file_paths, resource_id_range_lookup->second, ordered_ranges);
    }
}


std::vector<std::string> Numberer::ReadIconNamesFromResourceFile(const std::string& resource_file_contents)
{
    std::vector<std::string> icon_names;
    const std::regex icon_regex("(\\S+)\\s+ICON\\s+\".+\"");
    std::cmatch matches;

    SO::ForeachLine<std::string>(resource_file_contents, false,
        [&](const std::string& line)
        {
            if( std::regex_match(line.c_str(), matches, icon_regex) )
                icon_names.emplace_back(matches.str(1));

            return true;
        });

    return icon_names;
}


std::vector<std::shared_ptr<Numberer::ResourceId>> Numberer::ReadResourceIdsFromHeader(const std::string& header_contents)
{
    std::vector<std::shared_ptr<ResourceId>> resource_ids;
    const std::regex resource_id_regex("#define\\s+([a-zA-Z]\\S+)\\s+(\\d+)");
    std::cmatch matches;

    SO::ForeachLine<std::string>(header_contents, false,
        [&](const std::string& line)
        {
            if( std::regex_match(line.c_str(), matches, resource_id_regex) )
                resource_ids.emplace_back(std::make_shared<ResourceId>(ResourceId { matches.str(1), std::stoi(matches.str(2)) }));

            return true;
        });

    return resource_ids;
}


namespace NamePrefixes
{
    const std::vector<const char*> name_prefixes =
    {
        "IDM_ABOUTBOX",
        "IDR_",
        "IDD_",
        "IDI_",
        "IDB_",
        "IDP_",
        "IDS_",
        "IDM_",
        "IDC_",
        "ID_"
    };

    constexpr int AboutBoxIndex = 0;
    constexpr int ControlIndex  = 8;
    constexpr int CommandIndex  = 9;

    int GetIndex(const std::string& name)
    {
        const auto& lookup = std::find_if(name_prefixes.cbegin(), name_prefixes.cend(),
                                          [&](const char* name_prefix) { return SO::StartsWith(name, name_prefix); });

        return ( lookup != name_prefixes.cend() ) ? int32_cast(std::distance(name_prefixes.cbegin(), lookup)) :
                                                    -1;
    }
}


bool Numberer::ResourceIdSorter(const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs)
{
    const int comparison = NamePrefixes::GetIndex(lhs->name) - NamePrefixes::GetIndex(rhs->name);

    if( comparison != 0 )
        return ( comparison < 0 );

    // a lowercase comparison is done on strings to match the previous C# version of this program that sorted _ before letters
    static_assert('_' > 'A' && '_' < 'a');

    return ( SO::ToLower(lhs->name) < SO::ToLower(rhs->name) );
}


Numberer::ResourceIdRange Numberer::RenumberResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, ResourceIdRange resource_id_range)
{
    for( ResourceId& resource_id : VI_V(resource_ids) )
    {
        const int prefix_index = NamePrefixes::GetIndex(resource_id.name);

        if( prefix_index == NamePrefixes::AboutBoxIndex )
        {
            constexpr int AboutBoxId = 16;
            resource_id.id = AboutBoxId;
        }

        else if( prefix_index == NamePrefixes::CommandIndex )
        {
            resource_id.id = resource_id_range.command++;
        }

        else if( prefix_index == NamePrefixes::ControlIndex )
        {
            resource_id.id = resource_id_range.control++;
        }

        else
        {
            resource_id.id = resource_id_range.resource++;
        }
    }

    return resource_id_range;
}


void Numberer::ArrangeResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, const std::vector<std::string>& ordered_names)
{
    std::optional<size_t> last_location;

    for( const std::string& name : ordered_names )
    {
        for( size_t i = 0; i < resource_ids.size(); ++i )
        {
            if( resource_ids[i]->name == name )
            {
                if( !last_location.has_value() )
                {
                    last_location = i;
                }

                else
                {
                    std::shared_ptr<ResourceId> temp_resource_id = std::move(resource_ids[i]);
                    resource_ids.erase(resource_ids.begin() + i);

                    if( i >= *last_location )
                        ++(*last_location);

                    resource_ids.insert(resource_ids.begin() + *last_location, std::move(temp_resource_id));
                }

                break;
            }
        }
    }
}


void Numberer::SortAndWriteResourceIds(const ResourceFilePaths& resource_file_paths, const bool shared_ids,
                                       std::vector<std::shared_ptr<ResourceId>>& resource_ids,
                                       const ResourceIdRange& resource_id_range,
                                       const std::string& initial_header_contents)
{
    std::string contents;

    contents.append("//{{NO_DEPENDENCIES}}\r\n");

    if( !shared_ids )
    {
        contents.append("// Microsoft Visual C++ generated include file.\r\n");
        contents.append(FormatText("// Used by %s\r\n", PortableFunctions::PathGetFilename(resource_file_paths.resource).c_str()));
        contents.append("//\r\n");
    }

    // sort the IDs by the ID
    std::sort(resource_ids.begin(), resource_ids.end(),
              [&](const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs) { return ( lhs->id < rhs->id ); });

    for( const ResourceId& resource_id : VI_V(resource_ids) )
    {
        const int spacing = std::max(0, 31 - static_cast<int>(resource_id.name.length()));
        contents.append(FormatText("#define %s %s%d\r\n", resource_id.name.c_str(), SO::GetRepeatingCharacterString(' ', spacing), resource_id.id));
    }

    if( !shared_ids )
    {
        contents.append("\r\n");
        contents.append("// Next default values for new objects\r\n");
        contents.append("// \r\n");
        contents.append("#ifdef APSTUDIO_INVOKED\r\n");
        contents.append("#ifndef APSTUDIO_READONLY_SYMBOLS\r\n");
        contents.append(FormatText("#define _APS_NEXT_RESOURCE_VALUE        %d\r\n", resource_id_range.resource));
        contents.append(FormatText("#define _APS_NEXT_COMMAND_VALUE         %d\r\n", resource_id_range.command));
        contents.append(FormatText("#define _APS_NEXT_CONTROL_VALUE         %d\r\n", resource_id_range.control));
        contents.append(FormatText("#define _APS_NEXT_SYMED_VALUE           %d\r\n", resource_id_range.resource));
        contents.append("#endif\r\n");
        contents.append("#endif\r\n");
    }

    // only write the contents if they changed
    if( contents != initial_header_contents )
    {
        const std::string& header_file_path = shared_ids ? resource_file_paths.shared_header :
                                                           resource_file_paths.header;

        FileIO::WriteText(header_file_path, contents, false);
    }
}


void Numberer::ProcessFiles(const ResourceFilePaths& resource_file_paths, const ResourceIdRange& resource_id_range,
                            const std::vector<std::vector<std::string>>* ordered_ranges)
{
    // read any icon names from the resource file
    const std::vector<std::string> icon_names = ReadIconNamesFromResourceFile(FileIO::ReadText(resource_file_paths.resource));

    // read the main resource header's IDs
    const std::string main_header_contents = FileIO::ReadText(resource_file_paths.header);
    std::vector<std::shared_ptr<ResourceId>> main_resource_ids = ReadResourceIdsFromHeader(main_header_contents);
    std::vector<std::shared_ptr<ResourceId>> all_resource_ids = main_resource_ids;

    // read the shared resource header's IDs if applicable
    std::string shared_header_contents;
    std::vector<std::shared_ptr<ResourceId>> shared_resource_ids;

    if( !resource_file_paths.shared_header.empty() )
    {
        shared_header_contents = FileIO::ReadText(resource_file_paths.shared_header);
        shared_resource_ids = ReadResourceIdsFromHeader(shared_header_contents);
        all_resource_ids.insert(all_resource_ids.end(), shared_resource_ids.cbegin(), shared_resource_ids.cend());
    }

    // sort the IDs without regard to any custom order
    std::sort(all_resource_ids.begin(), all_resource_ids.end(), ResourceIdSorter);

    // then keep them in icon order
    ArrangeResourceIds(all_resource_ids, icon_names);

    // and custom range order
    if( ordered_ranges != nullptr )
    {
        for( const std::vector<std::string>& ordered_names : *ordered_ranges )
            ArrangeResourceIds(all_resource_ids, ordered_names);
    }

    // renumber the IDs
    const ResourceIdRange modified_resource_id_range = RenumberResourceIds(all_resource_ids, resource_id_range);

    // make sure the IDs don't overlap another set
    for( const auto& [project_name, project_resource_id_range] : m_projectResourceIdRanges )
    {
        if( &project_resource_id_range == &resource_id_range )
            continue;

        auto ids_overlap = [&](const int project_id, const int original_id, const int modified_id)
        {
            return ( project_id > original_id && modified_id >= project_id );
        };

        if( ids_overlap(project_resource_id_range.resource, resource_id_range.resource, modified_resource_id_range.resource) ||
            ids_overlap(project_resource_id_range.command, resource_id_range.command, modified_resource_id_range.command) ||
            ids_overlap(project_resource_id_range.control, resource_id_range.control, modified_resource_id_range.control) )
        {
            throw CSProException("IDs overlap with those in project '%s'", project_name.c_str());
        }
    }

    // write the new resource headers
    SortAndWriteResourceIds(resource_file_paths, false, main_resource_ids, modified_resource_id_range, main_header_contents);

    if( !shared_header_contents.empty() )
        SortAndWriteResourceIds(resource_file_paths, true, shared_resource_ids, modified_resource_id_range, shared_header_contents);
}
