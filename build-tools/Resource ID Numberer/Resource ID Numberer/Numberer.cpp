#include "stdafx.h"
#include "Numberer.h"
#include <zToolsO/DirectoryLister.h>
#include <zJson/JsonSpecFile.h>
#include <iostream>
#include <regex>


Numberer::Numberer(const std::wstring& definitions_filename, bool only_process_recent_changes)
    :   m_onlyProcessRecentChanges(only_process_recent_changes)
{
    ReadDefinitionsFile(definitions_filename);
}


void Numberer::ReadDefinitionsFile(const std::wstring& definitions_filename)
{
    auto json_reader = JsonSpecFile::CreateReader(definitions_filename);

    m_codeRoot = json_reader->GetAbsolutePath(L"codeRoot");

    for( const auto& exclusion_node : json_reader->GetArrayOrEmpty(L"resourceExclusions") )
        m_resourceExclusions.emplace_back(MakeFullPath(m_codeRoot, exclusion_node.Get<std::wstring>()));

    json_reader->Get(L"projects").ForeachNode( 
        [&](wstring_view project_name, const auto& ranges_node)
        {
            m_projectResourceIdRanges.try_emplace(project_name,
                ResourceIdRange 
                {
                    ranges_node.Get<int>(L"resource"),
                    ranges_node.Get<int>(L"command"),
                    ranges_node.Get<int>(L"control")
                });
        });
    
    json_reader->GetOrEmpty(L"orderedRanges").ForeachNode(
        [&](wstring_view project_name, const auto& ranges_node)
        {
            std::vector<std::vector<std::wstring>> project_ranges;
            
            for( const auto& range_node : ranges_node.GetArray() )
                project_ranges.emplace_back(range_node.GetArray().GetVector<std::wstring>());

            m_projectOrderedRanges.try_emplace(project_name, std::move(project_ranges));
        });
}


void Numberer::Run()
{
    std::vector<std::wstring> listed_resource_filenames = DirectoryLister().SetRecursive()
                                                                           .SetNameFilter(L"*.rc")
                                                                           .GetPaths(m_codeRoot);

    if( listed_resource_filenames.empty() )
        throw CSProException(L"No resource files exist in: %s", m_codeRoot.c_str());

    for( const std::wstring& resource_filename : listed_resource_filenames )
    {
        // skip excluded files
        const auto& exclusion_lookup = std::find_if(m_resourceExclusions.cbegin(), m_resourceExclusions.cend(), 
                                                    [&](const std::wstring& exclusion_filename) { return SO::EqualsNoCase(exclusion_filename, resource_filename); });

        if( exclusion_lookup != m_resourceExclusions.cend() )
            continue;

        std::wstring resource_directory = PortableFunctions::PathGetDirectory(resource_filename);
        std::wstring project_name = PortableFunctions::PathGetFilename(PortableFunctions::PathRemoveTrailingSlash(resource_directory));

        ResourceFilenames resource_filenames
        {
            resource_filename,
            PortableFunctions::PathAppendToPath(resource_directory, L"resource.h"),
            PortableFunctions::PathAppendToPath(resource_directory, L"resource_shared.h")
        };

        if( !PortableFunctions::FileIsRegular(resource_filenames.header) )
            throw CSProException(L"Resource header does not exist: %s", resource_filenames.header.c_str());

        if( !PortableFunctions::FileIsRegular(resource_filenames.shared_header) )
            resource_filenames.shared_header.clear();

        // skip old files if only processing recent changes
        if( m_onlyProcessRecentChanges )
        {
            // consider "recent" to mean anything within the last eight hours
            int64_t earliest_timestamp = GetTimestamp<int64_t>() - DateHelper::SecondsInHour(8);

            if( ( PortableFunctions::FileModifiedTime(resource_filenames.header) < earliest_timestamp ) &&
                ( resource_filenames.shared_header.empty() || PortableFunctions::FileModifiedTime(resource_filenames.shared_header) < earliest_timestamp ) )
            {
                std::wcout << L"Skipping " << resource_filenames.header.c_str() << std::endl;
                continue;
            }
        }

        std::wcout << L"Processing " << resource_filenames.header.c_str() << std::endl;

        // get the resource ID range set for this project
        auto resource_id_range_lookup = m_projectResourceIdRanges.find(project_name);

        if( resource_id_range_lookup == m_projectResourceIdRanges.cend() )
        {
            resource_id_range_lookup = m_projectResourceIdRanges.find(L"default");

            if( resource_id_range_lookup == m_projectResourceIdRanges.cend() )
                throw CSProException(L"No resource ID ranges defined for '%s' and no default ranges exist", project_name.c_str());
        }

        // determine if there are any ordered ranges for this project
        auto ordered_ranges_lookup = m_projectOrderedRanges.find(project_name);
        const std::vector<std::vector<std::wstring>>* ordered_ranges = ( ordered_ranges_lookup != m_projectOrderedRanges.cend() ) ? &ordered_ranges_lookup->second :
                                                                                                                                    nullptr;

        // process the files
        ProcessFiles(resource_filenames, resource_id_range_lookup->second, ordered_ranges);
    }
}


std::vector<std::wstring> Numberer::ReadIconNamesFromResourceFile(const std::wstring& resource_file_contents)
{
    std::vector<std::wstring> icon_names;
    std::wregex icon_regex(L"(\\S+)\\s+ICON\\s+\".+\"");
    std::wcmatch matches;

    SO::ForeachLine(resource_file_contents, false,
        [&](const std::wstring& line)
        {
            if( std::regex_match(line.c_str(), matches, icon_regex) )
                icon_names.emplace_back(matches.str(1));

            return true;
        });

    return icon_names;
}


std::vector<std::shared_ptr<Numberer::ResourceId>> Numberer::ReadResourceIdsFromHeader(const std::wstring& header_contents)
{
    std::vector<std::shared_ptr<ResourceId>> resource_ids;
    std::wregex resource_id_regex(L"#define\\s+([a-zA-Z]\\S+)\\s+(\\d+)");
    std::wcmatch matches;

    SO::ForeachLine(header_contents, false,
        [&](const std::wstring& line)
        {
            if( std::regex_match(line.c_str(), matches, resource_id_regex) )
                resource_ids.emplace_back(std::make_shared<ResourceId>(ResourceId { matches.str(1), std::stoi(matches.str(2)) }));

            return true;
        });

    return resource_ids;
}


namespace NamePrefixes
{
    const std::vector<const TCHAR*> name_prefixes = 
    {
        L"IDM_ABOUTBOX",
        L"IDR_",
        L"IDD_",
        L"IDI_",
        L"IDB_",
        L"IDP_",
        L"IDS_",
        L"IDM_",
        L"IDC_",
        L"ID_"
    };

    constexpr int AboutBoxIndex = 0;
    constexpr int ControlIndex  = 8;
    constexpr int CommandIndex  = 9;

    int GetIndex(const std::wstring& name)
    {
        const auto& lookup = std::find_if(name_prefixes.cbegin(), name_prefixes.cend(),
                                          [&](const TCHAR* name_prefix) { return SO::StartsWith(name, name_prefix); });

        return ( lookup != name_prefixes.cend() ) ? std::distance(name_prefixes.cbegin(), lookup) : 
                                                    -1;
    }
}


bool Numberer::ResourceIdSorter(const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs)
{
    int comparison = NamePrefixes::GetIndex(lhs->name) - NamePrefixes::GetIndex(rhs->name);

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
        int prefix_index = NamePrefixes::GetIndex(resource_id.name);

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


void Numberer::ArrangeResourceIds(std::vector<std::shared_ptr<ResourceId>>& resource_ids, const std::vector<std::wstring>& ordered_names)
{
    std::optional<size_t> last_location;

    for( const std::wstring& name : ordered_names )
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


void Numberer::SortAndWriteResourceIds(const ResourceFilenames& resource_filenames, bool shared_ids,
                                       std::vector<std::shared_ptr<ResourceId>>& resource_ids, 
                                       const ResourceIdRange& resource_id_range,
                                       const std::wstring& initial_header_contents)
{
    std::wstring contents;

    contents.append(L"//{{NO_DEPENDENCIES}}\r\n");

    if( !shared_ids )
    {
        contents.append(L"// Microsoft Visual C++ generated include file.\r\n");
        contents.append(FormatTextCS2WS(L"// Used by %s\r\n", PortableFunctions::PathGetFilename(resource_filenames.resource)));
        contents.append(L"//\r\n");
    }

    // sort the IDs by the ID
    std::sort(resource_ids.begin(), resource_ids.end(), 
              [&](const std::shared_ptr<ResourceId>& lhs, const std::shared_ptr<ResourceId>& rhs) { return ( lhs->id < rhs->id ); });

    for( const ResourceId& resource_id : VI_V(resource_ids) )
    {
        int spacing = std::max(0, 31 - static_cast<int>(resource_id.name.length()));
        contents.append(FormatTextCS2WS(L"#define %s %s%d\r\n", resource_id.name.c_str(), SO::GetRepeatingCharacterString(' ', spacing), resource_id.id));
    }

    if( !shared_ids )
    {
        contents.append(L"\r\n");
        contents.append(L"// Next default values for new objects\r\n");
        contents.append(L"// \r\n");
        contents.append(L"#ifdef APSTUDIO_INVOKED\r\n");
        contents.append(L"#ifndef APSTUDIO_READONLY_SYMBOLS\r\n");
        contents.append(FormatTextCS2WS(L"#define _APS_NEXT_RESOURCE_VALUE        %d\r\n", resource_id_range.resource));
        contents.append(FormatTextCS2WS(L"#define _APS_NEXT_COMMAND_VALUE         %d\r\n", resource_id_range.command));
        contents.append(FormatTextCS2WS(L"#define _APS_NEXT_CONTROL_VALUE         %d\r\n", resource_id_range.control));
        contents.append(FormatTextCS2WS(L"#define _APS_NEXT_SYMED_VALUE           %d\r\n", resource_id_range.resource));
        contents.append(L"#endif\r\n");
        contents.append(L"#endif\r\n");
    }

    // only write the contents if they changed
    if( contents != initial_header_contents )
    {
        const std::wstring& header_filename = shared_ids ? resource_filenames.shared_header : 
                                                           resource_filenames.header;

        FileIO::WriteText(header_filename, contents, false);
    }
}


void Numberer::ProcessFiles(const ResourceFilenames& resource_filenames, const ResourceIdRange& resource_id_range,
                            const std::vector<std::vector<std::wstring>>* ordered_ranges)
{
    // read any icon names from the resource file
    std::vector<std::wstring> icon_names = ReadIconNamesFromResourceFile(FileIO::ReadText(resource_filenames.resource));
    
    // read the main resource header's IDs
    std::wstring main_header_contents = FileIO::ReadText(resource_filenames.header);
    std::vector<std::shared_ptr<ResourceId>> main_resource_ids = ReadResourceIdsFromHeader(main_header_contents);
    std::vector<std::shared_ptr<ResourceId>> all_resource_ids = main_resource_ids;

    // read the shared resource header's IDs if applicable
    std::wstring shared_header_contents;
    std::vector<std::shared_ptr<ResourceId>> shared_resource_ids;

    if( !resource_filenames.shared_header.empty() )
    {
        shared_header_contents = FileIO::ReadText(resource_filenames.shared_header);
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
        for( const std::vector<std::wstring>& ordered_names : *ordered_ranges )
            ArrangeResourceIds(all_resource_ids, ordered_names);
    }

    // renumber the IDs
    ResourceIdRange modified_resource_id_range = RenumberResourceIds(all_resource_ids, resource_id_range);

    // make sure the IDs don't overlap another set
    for( const auto& [project_name, project_resource_id_range] : m_projectResourceIdRanges )
    {
        if( &project_resource_id_range == &resource_id_range )
            continue;

        auto ids_overlap = [&](int project_id, int original_id, int modified_id)
        {
            return ( project_id > original_id && modified_id >= project_id );
        };

        if( ids_overlap(project_resource_id_range.resource, resource_id_range.resource, modified_resource_id_range.resource) ||
            ids_overlap(project_resource_id_range.command, resource_id_range.command, modified_resource_id_range.command) ||
            ids_overlap(project_resource_id_range.control, resource_id_range.control, modified_resource_id_range.control) )
        {
            throw CSProException(L"IDs overlap with those in project '%s'", project_name.c_str());
        }
    }

    // write the new resource headers
    SortAndWriteResourceIds(resource_filenames, false, main_resource_ids, modified_resource_id_range, main_header_contents);

    if( !shared_header_contents.empty() )
        SortAndWriteResourceIds(resource_filenames, true, shared_resource_ids, modified_resource_id_range, shared_header_contents);
}
