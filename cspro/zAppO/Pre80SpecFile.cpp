#include "stdafx.h"
#include "Application.h"
#include "Properties/ApplicationProperties.h"
#include <zUtilO/Specfile.h>


namespace
{
    class ParadataNode
    {
    public:
        Json::ObjectCreator& GetNode() { return m_node; }

        bool ProcessLine(wstring_view command, const CString& argument);

        void WriteNode(JsonWriter& json_writer)
        {
            m_node.Set(JK::events, m_eventNames);
            json_writer.Write(JK::paradata, m_node.GetJsonNode());
        }

    private:
        Json::ObjectCreator m_node;
        std::set<CString> m_eventNames;
    };


    bool ParadataNode::ProcessLine(wstring_view command, const CString& argument)
    {
        const wstring_view Pre77Prefix = _T("Paradata");

        if( command.find(Pre77Prefix) == 0 )
            command = command.substr(Pre77Prefix.length());

        auto command_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(command, text);
        };

        auto argument_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(argument, text);
        };

        auto argument_as_bool = [&]()
        {
            return ( argument.CompareNoCase(CSPRO_ARG_YES) == 0 ) ? true :
                   ( argument.CompareNoCase(CSPRO_ARG_NO) == 0 ) ? false :
                   throw CSProException(_T("The argument '%s' was not a boolean (Yes/No)."), argument.GetString());
        };

        auto argument_as_int = [&]()
        {
            return _ttoi(argument);
        };

        if( command_is(_T("Collection")) )
        {
            m_node.Set(JK::collection, argument_is(_T("AllEvents"))  ? _T("all") :
                                       argument_is(_T("SomeEvents")) ? _T("partial") :
                                       argument_is(_T("No"))         ? _T("none") :
                                                                       argument);
        }

        else if( command_is(_T("RecordIteratorLoadCases")) )
        {
            m_node.Set(JK::recordIteratorLoadCases, argument_as_bool());
        }

        else if( command_is(_T("RecordValues")) )
        {
            m_node.Set(JK::recordValues, argument_as_bool());
        }

        else if( command_is(_T("RecordCoordinates")) )
        {
            m_node.Set(JK::recordCoordinates, argument_as_bool());
        }

        else if( command_is(_T("RecordInitialPropertyValues")) )
        {
            m_node.Set(JK::recordInitialPropertyValues, argument_as_bool());
        }

        else if( command_is(_T("DeviceStateIntervalMinutes")) || command_is(_T("DeviceStateMinutes")) )
        {
            m_node.Set(JK::deviceStateIntervalMinutes, argument_as_int());
        }

        else if( command_is(_T("GpsLocationIntervalMinutes")) || command_is(_T("GpsLocationMinutes")) )
        {
            m_node.Set(JK::gpsLocationIntervalMinutes, argument_as_int());
        }

        else if( command_is(_T("CollectEvent")) )
        {
            m_eventNames.insert(argument);
        }

        else
        {
            return false;
        }

        return true;
    }
}


std::wstring Application::ConvertPre80SpecFile(NullTerminatedString filename)
{
    CSpecFile specfile;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the Application file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, _T("application"));

    enum class Section { Properties, ExternalDictionaries, Logic, Messages, Reports, QuestionText,
                         FormsOrdersTabSpecs, Resources, DictionaryTypes, Sync, Mapping };
    std::optional<Section> current_section;

    std::optional<double> file_version;
    std::optional<EngineAppType> engine_app_type;

    auto properties_json_writer = Json::CreateStringWriter();
    properties_json_writer->BeginObject();

    Json::ObjectCreator partial_save_node;
    Json::ObjectCreator verify_node;
    Json::ObjectCreator notes_node;
    ParadataNode paradata_node;

    std::vector<DictionaryDescription> dictionary_descriptions;
    std::vector<CodeFile> code_files;
    std::vector<CString> message_filenames;
    std::unique_ptr<JsonStringWriter<wchar_t>> reports_json_writer;
    std::vector<CString> form_order_tab_spec_filenames;
    std::vector<CString> resource_folders;
    std::optional<Json::ObjectCreator> sync_node;
    std::optional<Json::ObjectCreator> mapping_node;

    try
    {
        CString command;
        CString argument;

        auto command_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(command, text);
        };

        auto argument_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(argument, text);
        };

        auto argument_as_bool = [&]()
        {
            return ( argument.CompareNoCase(CSPRO_ARG_YES) == 0 ) ? true :
                   ( argument.CompareNoCase(CSPRO_ARG_NO) == 0 ) ? false :
                   throw CSProException(_T("The argument '%s' was not a boolean (Yes/No)."), argument.GetString());
        };

        auto argument_as_int = [&]()
        {
            return _ttoi(argument);
        };

        auto throw_invalid_command = [&]()
        {
            throw CSProException(_T("Spec File: Invalid command: %s"), command.GetString());
        };

        while( specfile.GetLine(command, argument) == SF_OK )
        {
            if( command_is(_T("[Properties]")) )
            {
                current_section = Section::Properties;
            }

            else if( command_is(_T("[External Dictionaries]")) )
            {
                current_section = Section::ExternalDictionaries;
            }

            else if( command_is(_T("[AppCode]")) )
            {
                current_section = Section::Logic;
            }

            else if( command_is(_T("[Message]")) )
            {
                current_section = Section::Messages;
            }

            else if( command_is(_T("[Reports]")) )
            {
                current_section = Section::Reports;
                reports_json_writer = Json::CreateStringWriter();
                reports_json_writer->BeginArray();
            }

            else if( command_is(_T("[Help]")) )
            {
                current_section = Section::QuestionText;
            }

            else if( command_is(_T("[Forms]")) || command_is(_T("[Order]")) || command_is(_T("[TabSpecs]")) )
            {
                current_section = Section::FormsOrdersTabSpecs;
            }

            else if( command_is(_T("[Resources]")) )
            {
                current_section = Section::Resources;
            }

            else if( command_is(_T("[Dictionary Types]")) )
            {
                current_section = Section::DictionaryTypes;
            }

            else if( command_is(_T("[Sync]")) )
            {
                current_section = Section::Sync;
                sync_node.emplace();
            }

            else if( command_is(_T("[Mapping]")) )
            {
                current_section = Section::Mapping;
                mapping_node.emplace();
            }

            else if( !current_section.has_value() )
            {
                if( command_is(_T("[NoEdit]")) )
                {
                    json_writer->Write(JK::editable, false);
                }

                else if( command_is(_T("Version")) )
                {
                    file_version = GetCSProVersionNumeric(argument);

                    // for older applications, set some new properties to what their old state would have been
                    if( file_version < 7.4 )
                        paradata_node.GetNode().Set(JK::recordInitialPropertyValues, true);
                }

                else if( command_is(_T("Label")) )
                {
                    json_writer->Write(JK::label, argument);
                }

                else if( command_is(_T("Name")) )
                {
                    json_writer->Write(JK::name, argument);
                }

                else if( command_is(_T("AppType")) )
                {
                    engine_app_type = argument_is(_T("DataEntry"))  ? EngineAppType::Entry :
                                      argument_is(_T("Tabulation")) ? EngineAppType::Tabulation :
                                      argument_is(_T("Batch"))      ? EngineAppType::Batch :
                                                                      EngineAppType::Invalid;

                    json_writer->Write(JK::type, ( engine_app_type == EngineAppType::Entry ) ? _T("entry") : SO::ToLower(argument));

                    if( engine_app_type == EngineAppType::Tabulation && file_version < 3.0 )
                        throw CSProException("Can support only .xtb files of CSPro 3.0 or newer.");
                }

                else if( command_is(_T("OperatorID")) )
                {
                    properties_json_writer->Write(JK::askOperatorId, argument_as_bool());
                }

                else if( command_is(_T("PartialSave")) )
                {
                    partial_save_node.Set(JK::operatorEnabled, argument_as_bool());
                }

                else if( command_is(_T("AutoPartialSaveMinutes")) )
                {
                    partial_save_node.Set(JK::autoSaveMinutes, argument_as_int());
                }

                else if( command_is(_T("CAPI")) )
                {
                    properties_json_writer->Write(JK::showQuestionText, argument_as_bool());
                }

                else if( command_is(_T("CaseTree")) )
                {
                    properties_json_writer->Write(JK::caseTree,
                        ( argument_is(_T("Yes")) || argument_is(_T("Always")) )      ? _T("on") :
                        ( argument_is(_T("Desktop")) || argument_is(_T("Windows")) ) ? _T("desktopOnly") :
                        ( argument_is(_T("Mobile")) )                                ? _T("mobileOnly") :
                        ( argument_is(_T("Never")) )                                 ? _T("off") :
                                                                                       argument);
                }

                else if( command_is(_T("CenterForms")) )
                {
                    properties_json_writer->Write(JK::centerForms, argument_as_bool());
                }

                else if( command_is(_T("DecimalComma")) )
                {
                    properties_json_writer->Write(JK::decimalMark, argument_as_bool() ? _T("comma") : _T("dot"));
                }

                else if( command_is(_T("VerifyFrequency")) )
                {
                    verify_node.Set(JK::frequency, argument_as_int());
                }

                else if( command_is(_T("VerifyStart")) )
                {
                    if( argument_is(_T("RANDOM")) )
                    {
                        verify_node.Set(JK::start, SO::ToLower(argument));
                    }

                    else
                    {
                        verify_node.Set(JK::start, argument_as_int());
                    }                    
                }

                else if( command_is(_T("UseHtmlDialogs")) )
                {
                    properties_json_writer->Write(JK::htmlDialogs, argument_as_bool());
                }

                else if( command_is(_T("ShowEndCaseDialog")) )
                {
                    properties_json_writer->Write(JK::showEndCaseMessage, argument_as_bool());
                }

                else if( command_is(_T("CreateListing")) )
                {
                    properties_json_writer->Write(JK::createListing, argument_as_bool());
                }

                else if( command_is(_T("CreateLog")) )
                {
                    properties_json_writer->Write(JK::createLog, argument_as_bool());
                }

                else if( command_is(_T("NotesDeleteOtherOperators")) )
                {
                    notes_node.Set(JK::delete_, argument_as_bool() ? _T("all") : _T("operator"));
                }
                
                else if( command_is(_T("NotesEditOtherOperators")) )
                {
                    notes_node.Set(JK::edit, argument_as_bool() ? _T("all") : _T("operator"));
                }
                
                else if( command_is(_T("AutoAdvanceOnSelection")) )
                {
                    properties_json_writer->Write(JK::autoAdvanceOnSelection, argument_as_bool());
                }

                else if( command_is(_T("DisplayCodesAlongsideLabels")) )
                {
                    properties_json_writer->Write(JK::displayCodesAlongsideLabels, argument_as_bool());
                }

                else if( command_is(_T("ShowFieldLabels")) )
                {
                    properties_json_writer->Write(JK::showFieldLabels, argument_as_bool());
                }

                else if( command_is(_T("ShowErrorMessageNumbers")) )
                {
                    properties_json_writer->Write(JK::showErrorMessageNumbers, argument_as_bool());
                }

                else if( command_is(_T("ComboBoxShowOnlyDiscreteValues")) )
                {
                    properties_json_writer->Write(JK::showOnlyDiscreteValuesInComboBoxes, argument_as_bool());
                }

                else if( command_is(_T("ShowRefusals")) )
                {
                    properties_json_writer->Write(JK::showRefusals, argument_as_bool());
                }

                else if( !paradata_node.ProcessLine(command, argument) )
                {
                    // invalid commands were permitted so no exception will be thrown
                    ASSERT(command_is(_T("[CSPro Application]")) ||
                           command_is(_T("CAPIFont1")) ||
                           command_is(_T("CAPIFont2")) ||
                           command_is(_T("CAPIFont1Color")) ||
                           command_is(_T("CAPIFont2Color")) ||
                           command_is(_T("CAPIPPC")) ||
                           command_is(_T("CAPIPPCCONTROLS")) ||
                           command_is(_T("Note")) ||
                           command_is(_T("ValidateAlphaFields")));
                }
            }

            else if( current_section == Section::Properties )
            {
                if( command_is(_T("File")) )
                {
                    properties_json_writer->BeginArray(JK::import)
                                           .Write(specfile.EvaluateRelativeFilename(argument))
                                           .EndArray();
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::ExternalDictionaries )
            {
                if( command_is(_T("File")) )
                {
                    dictionary_descriptions.emplace_back(CS2WS(specfile.EvaluateRelativeFilename(argument)), DictionaryType::External);
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::Logic  )
            {
                if( bool main_file = command_is(_T("File")); main_file || command_is(_T("Include")) )
                {
                    code_files.emplace_back(main_file ? CodeType::LogicMain : CodeType::LogicExternal,
                                            std::make_shared<TextSource>(CS2WS(specfile.EvaluateRelativeFilename(argument))));
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::Messages )
            {
                if( bool main_file = command_is(_T("File")); main_file || command_is(_T("Include")) )
                {
                    message_filenames.insert(main_file ? message_filenames.begin() : message_filenames.end(), specfile.EvaluateRelativeFilename(argument));
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::Reports )
            {
                ASSERT(reports_json_writer != nullptr);

                reports_json_writer->BeginObject()
                                    .Write(JK::name, command)
                                    .Write(JK::path, specfile.EvaluateRelativeFilename(argument))
                                    .EndObject();
            }

            else if( current_section == Section::QuestionText )
            {
                if( command_is(_T("File")) )
                {
                    json_writer->Write(JK::questionText, std::vector<CString> { specfile.EvaluateRelativeFilename(argument) });
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::FormsOrdersTabSpecs )
            {
                if( command_is(_T("File")) )
                {
                    form_order_tab_spec_filenames.emplace_back(specfile.EvaluateRelativeFilename(argument));
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::Resources )
            {
                if( command_is(_T("Folder")) )
                {
                    resource_folders.emplace_back(specfile.EvaluateRelativeFilename(argument).TrimRight(PATH_CHAR));
                }

                else
                {
                    throw_invalid_command();
                }
            }
            
            else if( current_section == Section::DictionaryTypes )
            {
                if( command_is(_T("Dict-Type")) )
                {
                    std::vector<std::wstring> arguments = SO::SplitString(argument, ',');

                    if( arguments.size() < 2 || arguments.size() > 3 )
                        throw CSProException(_T("Invalid dictionary type: %s"), argument.GetString());

                    DictionaryType dictionary_type = SO::EqualsNoCase(arguments[1], _T("Input"))   ? DictionaryType::Input :
                                                     SO::EqualsNoCase(arguments[1], _T("Output"))  ? DictionaryType::Output :
                                                     SO::EqualsNoCase(arguments[1], _T("Working")) ? DictionaryType::Working :
                                                                                                     DictionaryType::External;

                    std::wstring parent_filename = ( arguments.size() == 3 ) ? CS2WS(specfile.EvaluateRelativeFilename(WS2CS(arguments[2]))) :
                                                                               std::wstring();

                    // the dictionary filename is relative to the parent (when applicable)
                    std::wstring dictionary_filename = parent_filename.empty() ? CS2WS(specfile.EvaluateRelativeFilename(WS2CS(arguments[0]))) :
                                                                                 MakeFullPath(PortableFunctions::PathGetDirectory(parent_filename), arguments[0]);

                    // remove the description added in the external dictionaries section
                    const auto& dd_lookup = std::find_if(dictionary_descriptions.cbegin(), dictionary_descriptions.cend(),
                                                         [&](const auto& dd) { return SO::EqualsNoCase(dd.GetDictionaryFilename(), dictionary_filename); });

                    if( dd_lookup != dictionary_descriptions.cend() )
                        dictionary_descriptions.erase(dd_lookup);

                    dictionary_descriptions.emplace_back(std::move(dictionary_filename), std::move(parent_filename), dictionary_type);
                }

                else
                {
                    throw_invalid_command();
                }
            }
                        
            else if( current_section == Section::Sync )
            {
                ASSERT(sync_node.has_value());

                if( command_is(_T("Server")) )
                {
                    sync_node->Set(JK::server, argument);
                }

                else if( command_is(_T("Direction")) )
                {
                    sync_node->Set(JK::direction, SO::ToLower(argument));
                }

                else if( !command_is(_T("AppDownloadPath")) &&
                         !command_is(_T("User")) &&
                         !command_is(_T("Password")) )
                {
                    throw_invalid_command();
                }
            }
                                    
            else if( current_section == Section::Mapping )
            {
                ASSERT(mapping_node.has_value());

                if( command_is(_T("LatitudeItem")) )
                {
                    mapping_node->Set(JK::latitude, argument);
                }

                else if( command_is(_T("LongitudeItem")) )
                {
                    mapping_node->Set(JK::longitude, argument);
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else
            {
                ASSERT(false);
            }
        }

        json_writer->Write(JK::dictionaries, dictionary_descriptions);
        json_writer->Write(JK::code, code_files);
        json_writer->Write(JK::messages, message_filenames);

        if( reports_json_writer != nullptr )
        {
            reports_json_writer->EndArray();
            json_writer->Write(JK::reports, Json::Parse(reports_json_writer->GetString()));
        }

        json_writer->Write(( engine_app_type == EngineAppType::Batch )      ? JK::order :
                           ( engine_app_type == EngineAppType::Tabulation ) ? JK::tableSpecs :
                                                                              JK::forms, form_order_tab_spec_filenames);

        json_writer->Write(JK::resources, resource_folders);

        properties_json_writer->Write(JK::partialSave, partial_save_node.GetJsonNode());
        properties_json_writer->Write(JK::verify, verify_node.GetJsonNode());
        properties_json_writer->Write(JK::notes, notes_node.GetJsonNode());
        paradata_node.WriteNode(*properties_json_writer);

        if( sync_node.has_value() )
        {
            properties_json_writer->Write(JK::sync, sync_node->GetJsonNode());
        }

        if( mapping_node.has_value() )
        {
            mapping_node->Set(JK::type, _T("map"));
            properties_json_writer->Write(JK::caseListing, mapping_node->GetJsonNode());
        }

        properties_json_writer->EndObject();
        json_writer->Write(JK::properties, Json::Parse(properties_json_writer->GetString()));

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the Application specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    json_writer->EndObject();

    return json_writer->GetString();
}


std::wstring ApplicationProperties::ConvertPre80SpecFile(NullTerminatedString filename)
{
    CSpecFile specfile;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the Application Properties file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JK::properties);

    enum class Section { Paradata, Mapping, MappingTileProvider };
    std::optional<Section> current_section;

    ParadataNode paradata_node;

    Json::ObjectCreator mapping_node;
    std::unique_ptr<JsonStringWriter<wchar_t>> mapping_tile_provider_json_writer;

    try
    {
        CString command;
        CString argument;

        auto read_header = [&](const TCHAR* header)
        {
            if( !specfile.IsHeaderOK(header) )
                throw CSProException(_T("The heading or section '%s' was missing"), header);
        };

        auto command_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(command, text);
        };

        auto argument_is = [&](wstring_view text)
        {
            return SO::EqualsNoCase(argument, text);
        };

        auto argument_as_bool = [&]()
        {
            return ( argument.CompareNoCase(CSPRO_ARG_YES) == 0 ) ? true :
                   ( argument.CompareNoCase(CSPRO_ARG_NO) == 0 ) ? false :
                   throw CSProException(_T("The argument '%s' was not a boolean (Yes/No)."), argument.GetString());
        };

        auto argument_as_int = [&]()
        {
            return _ttoi(argument);
        };

        auto throw_invalid_command = [&]()
        {
            throw CSProException(_T("Spec File: Invalid command: %s"), command.GetString());
        };

        // is this a correct spec file?
        read_header(_T("[CSPro Properties]"));

        // read the version number (ignoring errors)
        specfile.IsVersionOK(CSPRO_VERSION);

        while( specfile.GetLine(command, argument) == SF_OK )
        {
            if( command_is(_T("[Paradata]")) )
            {
                current_section = Section::Paradata;
            }

            else if( command_is(_T("[Mapping]")) )
            {
                current_section = Section::Mapping;
            }

            else if( bool esri = command_is(_T("[EsriMappingTileProvider]")); esri || command_is(_T("[MapboxMappingTileProvider]")) )
            {
                current_section = Section::MappingTileProvider;

                if( mapping_tile_provider_json_writer == nullptr )
                {
                    mapping_tile_provider_json_writer = Json::CreateStringWriter();
                    mapping_tile_provider_json_writer->BeginArray();
                }

                else
                {
                    mapping_tile_provider_json_writer->EndObject();
                }

                mapping_tile_provider_json_writer->BeginObject();

                mapping_tile_provider_json_writer->Write(JK::name, esri ? MappingTileProvider::Esri : MappingTileProvider::Mapbox);
            }

            else if( current_section == Section::Paradata )
            {
                if( !paradata_node.ProcessLine(command, argument) )
                    throw_invalid_command();
            }

            else if( current_section == Section::Mapping )
            {
                if( command_is(_T("CoordinateDisplay")) )
                {
                    mapping_node.Set(JK::coordinateDisplay, argument_is(_T("Decimal")) ? _T("decimal") : argument);
                }

                else if( command_is(_T("DefaultBaseMap")) )
                {
                    CString filename_or_base_map = specfile.EvaluateRelativeFilename(argument);

                    if( !PortableFunctions::FileIsRegular(filename_or_base_map) )
                        filename_or_base_map = argument;

                    mapping_node.Set(JK::defaultBaseMap, filename_or_base_map);
                }

                else if( command_is(_T("WindowsMappingTileProvider")) )
                {
                    mapping_node.Set(JK::windowsMappingTileProvider, argument);
                }

                else
                {
                    throw_invalid_command();
                }
            }

            else if( current_section == Section::MappingTileProvider )
            {
                ASSERT(mapping_tile_provider_json_writer != nullptr);

                if( command_is(_T("AccessToken")) )
                {
                    mapping_tile_provider_json_writer->Write(JK::accessToken, argument);
                }

                else
                {
                    mapping_tile_provider_json_writer->Write(command, argument);
                }
            }

            else
            {
                throw_invalid_command();
            }
        }

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the Application Properties specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    paradata_node.WriteNode(*json_writer);

    if( mapping_tile_provider_json_writer != nullptr )
    {
        mapping_tile_provider_json_writer->EndObject();
        mapping_tile_provider_json_writer->EndArray();
        mapping_node.Set(JK::tileProviders, Json::Parse(mapping_tile_provider_json_writer->GetString()));
    }

    json_writer->Write(JK::mapping, mapping_node.GetJsonNode());

    json_writer->EndObject();

    return json_writer->GetString();
}
