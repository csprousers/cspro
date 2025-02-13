#include "stdafx.h"
#include "AppFileType.h"


namespace
{
    const std::map<AppFileType, std::wstring>& GetAppFileTypeExtensionMap()
    {
        static const std::map<AppFileType, std::wstring> extension_map =
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


const TCHAR* ToString(AppFileType app_file_type)
{
    switch( app_file_type )
    {
        case AppFileType::ApplicationBatch:      return _T("Batch Edit Application");
        case AppFileType::ApplicationEntry:      return _T("Data Entry Application");
        case AppFileType::ApplicationTabulation: return _T("Tabulation Application");
        case AppFileType::Dictionary:            return _T("Dictionary");
        case AppFileType::Form:                  return _T("Form");
        case AppFileType::Code:                  return _T("Logic");
        case AppFileType::Message:               return _T("Messages");
        case AppFileType::Order:                 return _T("Order");
        case AppFileType::QuestionText:          return _T("Question Text");
        case AppFileType::Report:                return _T("Report");
        case AppFileType::ResourceFolder:        return _T("Resource Folder");
        case AppFileType::TableSpec:             return _T("Table Specification");
        default:                                 return ReturnProgrammingError(_T(""));
    }
}


const TCHAR* GetFileExtension(AppFileType app_file_type)
{
    const std::map<AppFileType, std::wstring>& extension_map = GetAppFileTypeExtensionMap();
    const auto& lookup = extension_map.find(app_file_type);

    return ( lookup != extension_map.cend() )               ? lookup->second.c_str() :
           ( app_file_type == AppFileType::ResourceFolder ) ? _T("") :
                                                              ReturnProgrammingError(_T(""));
}


std::optional<AppFileType> GetAppFileTypeFromFileExtension(wstring_view extension)
{
    const std::map<AppFileType, std::wstring>& extension_map = GetAppFileTypeExtensionMap();
    const auto& lookup = std::find_if(extension_map.cbegin(), extension_map.cend(),
                                      [&](const auto& kv) { return SO::EqualsNoCase(kv.second, extension); });

    return ( lookup != extension_map.cend() ) ? std::make_optional(lookup->first) :
                                                std::nullopt;
}
