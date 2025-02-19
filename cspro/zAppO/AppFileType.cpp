#include "stdafx.h"
#include "AppFileType.h"


namespace
{
    const std::map<AppFileType, const char*>& GetAppFileTypeExtensionMap()
    {
        static const std::map<AppFileType, const char*> extension_map =
        {
            { AppFileType::ApplicationBatch,      FileExtensions::BatchApplication      },
            { AppFileType::ApplicationEntry,      FileExtensions::EntryApplication      },
            { AppFileType::ApplicationTabulation, FileExtensions::TabulationApplication },
            { AppFileType::Dictionary,            FileExtensions::Dictionary            },
            { AppFileType::Form,                  FileExtensions::Form                  },
            { AppFileType::Code,                  FileExtensions::Logic                 },
            { AppFileType::Message,               FileExtensions::Message               },
            { AppFileType::Order,                 FileExtensions::Order                 },
            { AppFileType::QuestionText,          FileExtensions::QuestionText          },
            { AppFileType::Report,                FileExtensions::HTML                  },
            { AppFileType::TableSpec,             FileExtensions::TableSpec             },
        };

        return extension_map;
    }
}


const char* ToString(const AppFileType app_file_type)
{
    switch( app_file_type )
    {
        case AppFileType::ApplicationBatch:      return "Batch Edit Application";
        case AppFileType::ApplicationEntry:      return "Data Entry Application";
        case AppFileType::ApplicationTabulation: return "Tabulation Application";
        case AppFileType::Dictionary:            return "Dictionary";
        case AppFileType::Form:                  return "Form";
        case AppFileType::Code:                  return "Logic";
        case AppFileType::Message:               return "Messages";
        case AppFileType::Order:                 return "Order";
        case AppFileType::QuestionText:          return "Question Text";
        case AppFileType::Report:                return "Report";
        case AppFileType::Resource:              return "Resource";
        case AppFileType::TableSpec:             return "Table Specification";
        default:                                 return ReturnProgrammingError("");
    }
}


const char* GetFileExtension(const AppFileType app_file_type)
{
    const std::map<AppFileType, const char*>& extension_map = GetAppFileTypeExtensionMap();
    const auto& lookup = extension_map.find(app_file_type);

    return ( lookup != extension_map.cend() )         ? lookup->second :
           ( app_file_type == AppFileType::Resource ) ? "" :
                                                        ReturnProgrammingError("");
}


std::optional<AppFileType> GetAppFileTypeFromFileExtension(const std::string_view extension_sv)
{
    const std::map<AppFileType, const char*>& extension_map = GetAppFileTypeExtensionMap();
    const auto& lookup = std::find_if(extension_map.cbegin(), extension_map.cend(),
                                      [&](const auto& kv) { return SO::EqualsNoCase(extension_sv, kv.second); });

    return ( lookup != extension_map.cend() ) ? std::make_optional(lookup->first) :
                                                std::nullopt;
}


int GetTreeOrder(const AppFileType app_file_type)
{
    switch( app_file_type )
    {
        case AppFileType::ApplicationEntry:      return 0;
        case AppFileType::ApplicationBatch:      return 1;
        case AppFileType::ApplicationTabulation: return 2;
        case AppFileType::Form:                  return 3;
        case AppFileType::Order:                 return 4;
        case AppFileType::TableSpec:             return 5;
        case AppFileType::Dictionary:            return 6;
        case AppFileType::QuestionText:          return 7;
        case AppFileType::Code:                  return 8;
        case AppFileType::Message:               return 9;
        case AppFileType::Report:                return 10;
        case AppFileType::Resource:              return 11;
        default:                                 return ReturnProgrammingError(12);
    }
}
