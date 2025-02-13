#include "Stdafx.h"
#include "Excel2CSProSpec.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/Specfile.h>
#include <zJson/JsonSpecFile.h>
#include <zAppO/PFF.h>


CREATE_JSON_VALUE(createNewFile)
CREATE_JSON_VALUE(excelConverter)
CREATE_JSON_VALUE(modifyAddCases)
CREATE_JSON_VALUE(modifyAddDeleteCases)

enum class CaseManagementNative
{
    CreateNewFile = 0,
    ModifyAddCases = 1,
    ModifyAddDeleteCases = 2
};

CREATE_ENUM_JSON_SERIALIZER(CaseManagementNative,
    { CaseManagementNative::CreateNewFile,        JV::createNewFile },
    { CaseManagementNative::ModifyAddCases,       JV::modifyAddCases },
    { CaseManagementNative::ModifyAddDeleteCases, JV::modifyAddDeleteCases })


namespace Pre80Spec
{
    constexpr std::wstring_view Excel          = _T("Excel");
    constexpr std::wstring_view InputDict      = _T("InputDict");
    constexpr std::wstring_view OutputData     = _T("OutputData");
    constexpr std::wstring_view StartingRow    = _T("StartingRow");
    constexpr std::wstring_view CaseManagement = _T("CaseManagement");
    constexpr std::wstring_view RunOnlyIfNewer = _T("RunOnlyIfNewer");
    constexpr std::wstring_view Mapping        = _T("Mapping");

    std::wstring ConvertFile(const TCHAR* filename);

    System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ ConvertMappings(const CString& dictionary_filename,
        const std::vector<std::vector<CString>>& mapping_lines);
}

namespace
{
    std::unique_ptr<JsonSpecFile::Reader> CreateJsonReader(const TCHAR* filename)
    {
        std::wstring file_contents = FileIO::ReadText(filename);

        // see if the file contains one of the pre-8.0 spec commands
        for( const auto& command : { Pre80Spec::Excel,
                                     Pre80Spec::InputDict,
                                     Pre80Spec::OutputData,
                                     Pre80Spec::StartingRow,
                                     Pre80Spec::CaseManagement,
                                     Pre80Spec::RunOnlyIfNewer,
                                     Pre80Spec::Mapping } )
        {
            if( SO::StartsWithNoCase(file_contents, command) )
                return JsonSpecFile::CreateReader(filename, Pre80Spec::ConvertFile(filename));
        }

        return JsonSpecFile::CreateReader(filename, file_contents);
    }
}


CSPro::Data::Excel2CSPro::Spec::Spec()
{
    StartingRow = 2;
    CaseManagement = CSPro::Data::Excel2CSPro::CaseManagement::CreateNewFile;
    RunOnlyIfNewer = false;

    Mappings = gcnew System::Collections::Generic::List<RecordMapping^>();
}


void CSPro::Data::Excel2CSPro::Spec::Load(System::String^ filename)
{
    try
    {
        std::wstring ws_filename = ToWS(filename);
        auto json_reader = CreateJsonReader(ws_filename.c_str());

        try
        {
            json_reader->CheckVersion();
            json_reader->CheckFileType(JV::excelConverter);

            if( json_reader->Contains(JK::excel) )
                ExcelFilename = gcnew System::String(json_reader->GetAbsolutePath(JK::excel).c_str());

            if( json_reader->Contains(JK::dictionary) )
                DictionaryFilename = gcnew System::String(json_reader->GetAbsolutePath(JK::dictionary).c_str());

            if( json_reader->Contains(JK::output) )
            {
                OutputConnectionString = gcnew CSPro::Util::ConnectionString(gcnew System::String(json_reader->Get<CString>(JK::output)));
                OutputConnectionString->AdjustRelativePath(System::IO::Path::GetDirectoryName(filename));
            }

            StartingRow = json_reader->GetOrDefault<int>(JK::startingRow, StartingRow);
            CaseManagement = (CSPro::Data::Excel2CSPro::CaseManagement)json_reader->GetOrDefault<CaseManagementNative>(JK::caseManagement, (CaseManagementNative)CaseManagement);
            RunOnlyIfNewer = json_reader->GetOrDefault<bool>(JK::runOnlyIfNewer, RunOnlyIfNewer);

            bool errors_processing_mappings = false;

            for( const auto& record_node : json_reader->GetArrayOrEmpty(JK::records) )
            {
                try
                {
                    auto record_mapping = gcnew RecordMapping;

                    record_mapping->RecordName = gcnew System::String(record_node.Get<CString>(JK::name));

                    // worskheet
                    {
                        auto worksheet_node = record_node.Get(JK::worksheet);

                        if( worksheet_node.Contains(JK::name) )
                            record_mapping->WorksheetName = gcnew System::String(worksheet_node.Get<CString>(JK::name));

                        record_mapping->WorksheetIndex = worksheet_node.Get<int>(JK::index);
                    }

                    for( const auto& item_node : record_node.GetArrayOrEmpty(JK::items) )
                    {
                        auto item_mapping = gcnew ItemMapping;

                        item_mapping->ItemName = gcnew System::String(item_node.Get<CString>(JK::name));

                        if( item_node.Contains(JK::occurrence) )
                            item_mapping->Occurrence = item_node.Get<int>(JK::occurrence);

                        item_mapping->ColumnIndex = item_node.Get<int>(JK::column);

                        record_mapping->ItemMappings->Add(item_mapping);
                    }

                    Mappings->Add(record_mapping);
                }

                catch( const JsonParseException& )
                {
                    errors_processing_mappings = true;
                }
            }

            if( errors_processing_mappings )
                json_reader->LogWarning(_T("Some mappings were not included due to errors in the file"));
        }

        catch( const CSProException& exception )
        {
            json_reader->GetMessageLogger().RethrowException(ws_filename.c_str(), exception);
        }

        json_reader->GetMessageLogger().DisplayWarnings();
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


namespace
{
    void WriteMappings(JsonWriter& json_writer,
                       System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ mappings)
    {
        json_writer.BeginArray(JK::records);

        for( int i = 0; i < mappings->Count; ++i )
        {
            auto record_mapping = mappings[i];

            json_writer.BeginObject();

            json_writer.Write(JK::name, ToWS(record_mapping->RecordName));

            // worksheet
            {
                json_writer.BeginObject(JK::worksheet);

                if( record_mapping->WorksheetName != nullptr )
                    json_writer.Write(JK::name, ToWS(record_mapping->WorksheetName));

                json_writer.Write(JK::index, record_mapping->WorksheetIndex);

                json_writer.EndObject();
            }

            // items array
            {
                json_writer.BeginArray(JK::items);

                for( int j = 0; j < record_mapping->ItemMappings->Count; ++j )
                {
                    auto item_mapping = record_mapping->ItemMappings[j];

                    json_writer.BeginObject();

                    json_writer.Write(JK::name, ToWS(item_mapping->ItemName));

                    if( item_mapping->Occurrence.HasValue )
                        json_writer.Write(JK::occurrence, item_mapping->Occurrence.Value);

                    json_writer.Write(JK::column, item_mapping->ColumnIndex);

                    json_writer.EndObject();
                }

                json_writer.EndArray();
            }

            json_writer.EndObject();
        }

        json_writer.EndArray();
    }
}


void CSPro::Data::Excel2CSPro::Spec::Save(System::String^ filename)
{
    try
    {
        std::wstring ws_filename = ToWS(filename);
        auto json_writer = JsonSpecFile::CreateWriter(ws_filename.c_str(), JV::excelConverter);

        if( ExcelFilename != nullptr )
            json_writer->WriteRelativePath(JK::excel, ToWS(ExcelFilename));

        if( DictionaryFilename != nullptr )
            json_writer->WriteRelativePath(JK::dictionary, ToWS(DictionaryFilename));

        if( OutputConnectionString != nullptr )
        {
            json_writer->Write(JK::output, OutputConnectionString->GetNativeConnectionString().
                                           ToRelativeString(PortableFunctions::PathGetDirectory(ws_filename), true));
        }

        json_writer->Write(JK::startingRow, StartingRow)
                    .Write(JK::caseManagement, (CaseManagementNative)CaseManagement)
                    .Write(JK::runOnlyIfNewer, RunOnlyIfNewer);

        WriteMappings(*json_writer, Mappings);

        json_writer->EndObject();

        json_writer->Close();

        // save a PFF file for this spec file if one does not already exist
        std::wstring pff_filename = PortableFunctions::PathRemoveFileExtension(ws_filename) + FileExtensions::WithDot::Pff;

        if( !PortableFunctions::FileIsRegular(pff_filename) )
        {
            PFF pff;
            pff.SetPifFileName(WS2CS(pff_filename));
            pff.SetAppType(APPTYPE::EXCEL2CSPRO_TYPE);
            pff.SetAppFName(ws_filename.c_str());
            pff.Save();
        }
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


namespace Pre80Spec
{
    std::wstring ConvertFile(const TCHAR* filename)
    {
        CSpecFile specfile;

        if( !specfile.Open(filename, CFile::modeRead) )
            throw CSProException(_T("Failed to open the Excel to CSPro specification file: %s"), filename);

        auto json_writer = Json::CreateStringWriter();

        json_writer->BeginObject();

        json_writer->Write(JK::version, 7.7);
        json_writer->Write(JK::fileType, JV::excelConverter);

        try
        {
            CString dictionary_filename;
            std::vector<std::vector<CString>> mapping_lines;

            CString command;
            CString argument;

            while( specfile.GetLine(command, argument) == SF_OK )
            {
                if( SO::EqualsNoCase(command, Pre80Spec::Excel) )
                {
                    json_writer->Write(JK::excel, specfile.EvaluateRelativeFilename(argument));
                }

                else if( SO::EqualsNoCase(command, Pre80Spec::InputDict) )
                {
                    dictionary_filename = specfile.EvaluateRelativeFilename(argument);
                    json_writer->Write(JK::dictionary, dictionary_filename);
                }

                else if( SO::EqualsNoCase(command, Pre80Spec::OutputData) )
                {
                    ConnectionString output_data(argument);
                    output_data.AdjustRelativePath(PortableFunctions::PathGetDirectory(filename));
                    json_writer->Write(JK::output, output_data.ToRelativeString(PortableFunctions::PathGetDirectory(filename), true));
                }

                else if( SO::EqualsNoCase(command, Pre80Spec::StartingRow) )
                {
                    json_writer->Write(JK::startingRow, _ttoi(argument));
                }

                else if( SO::EqualsNoCase(command, Pre80Spec::CaseManagement) )
                {
                    for( const auto& potential_argument : { JV::createNewFile, JV::modifyAddCases, JV::modifyAddDeleteCases } )
                    {
                        if( SO::EqualsNoCase(argument, potential_argument) )
                        {
                            json_writer->Write(JK::caseManagement, potential_argument);
                            break;
                        }
                    }
                }

                else if( SO::EqualsNoCase(command, Pre80Spec::RunOnlyIfNewer) )
                {
                    json_writer->Write(JK::runOnlyIfNewer, SO::EqualsNoCase(argument, _T("Yes")));
                }

                else if( SO::EqualsNoCase(command, _T("Mapping")) )
                {
                    std::vector<CString> components = WS2CS_Vector(SO::SplitString(argument, ';'));

                    if( components.size() < 2 || components.size() > 3 )
                        throw CSProException(_T("Unrecognized mapping at line %d"), specfile.GetLineNumber());

                    mapping_lines.emplace_back(std::move(components));
                }

                else
                {
                    throw CSProException(_T("Unrecognized command at line %d"), specfile.GetLineNumber());
                }
            }

            WriteMappings(*json_writer, ConvertMappings(dictionary_filename, mapping_lines));

            specfile.Close();
        }

        catch( const CSProException& exception )
        {
            specfile.Close();

            throw CSProException(_T("There was an error reading the Excel to CSPro specification file %s:\n\n%s"),
                                 PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
        }

        json_writer->EndObject();

        return json_writer->GetString();
    }


    System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ ConvertMappings(const CString& dictionary_filename,
        const std::vector<std::vector<CString>>& mapping_lines)
    {
        // ignore any errors while converting the mappings
        auto mappings = gcnew System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>();

        if( mapping_lines.empty() || dictionary_filename.IsEmpty() )
            return mappings;

        CDataDict dictionary;

        try
        {
            dictionary.Open(dictionary_filename, true);
        }

        catch( const CSProException& )
        {
            return mappings;
        }

        // first create the record mappings
        std::map<std::wstring, int> record_map; // name, index into mappings list
        std::map<std::wstring, int> item_lines; // name, column index from the file

        for( const auto& components : mapping_lines )
        {
            ASSERT(components.size() >= 2);

            std::wstring name = SO::ToUpper(components.front());
            int index = _ttoi(CString(components[1]).TrimLeft('@'));

            const CDictRecord* record = dictionary.FindRecord(name);

            if( record != nullptr )
            {
                if( record_map.find(name) == record_map.cend() )
                {
                    auto record_mapping = gcnew CSPro::Data::Excel2CSPro::RecordMapping();

                    record_mapping->RecordName = gcnew System::String(name.c_str());
                    record_mapping->WorksheetIndex = index;

                    if( components.size() == 3 )
                        record_mapping->WorksheetName = gcnew System::String(components[2]);

                    record_map.try_emplace(std::move(name), mappings->Count);
                    mappings->Add(record_mapping);
                }
            }

            else
            {
                item_lines.try_emplace(std::move(name), index);
            }
        }

        // now add the item mappings
        for( const auto& [name, index] : item_lines )
        {
            wstring_view item_name = name;
            std::optional<std::wstring> record_name;
            std::optional<int> occurrence;

            size_t dot_pos = name.find('.');

            if( dot_pos != std::wstring::npos )
            {
                record_name = item_name.substr(0, dot_pos);
                item_name = item_name.substr(dot_pos + 1);
            }

            size_t left_paren_pos = item_name.find('(');

            if( left_paren_pos != std::wstring::npos )
            {
                size_t right_paren_pos = item_name.find(')', left_paren_pos + 1);

                if( right_paren_pos != std::wstring::npos )
                {
                    occurrence = _ttoi(std::wstring(item_name.substr(left_paren_pos + 1, right_paren_pos - left_paren_pos - 1)).c_str());
                    item_name = item_name.substr(0, left_paren_pos);
                }
            }

            const CDictRecord* dict_record;
            const CDictItem* dict_item;

            if( dictionary.LookupName<CDictItem>(CString(item_name), nullptr, &dict_record, &dict_item) )
            {
                // if the record name was not provided, use the record name
                // (the record name is not overriden because it had to be explicitly specified for ID items)
                if( !record_name.has_value() )
                    record_name = dict_record->GetName();

                const auto& record_lookup = record_map.find(*record_name);

                if( record_lookup != record_map.cend() )
                {
                    auto item_mapping = gcnew CSPro::Data::Excel2CSPro::ItemMapping();

                    item_mapping->ItemName = gcnew System::String(dict_item->GetName());

                    if( occurrence.has_value() )
                        item_mapping->Occurrence = *occurrence;

                    item_mapping->ColumnIndex = index;

                    mappings[record_lookup->second]->ItemMappings->Add(item_mapping);
                }
            }
        }

        return mappings;
    }
}
