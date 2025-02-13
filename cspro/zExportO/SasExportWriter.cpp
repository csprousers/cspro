#include "stdafx.h"
#include "SasExportWriter.h"
#include "EncodedTextWriter.h"
#include <zDictO/ValueSetResponse.h>


SasExportWriter::SasExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   ExportWriterBase(DataRepositoryType::SAS, std::move(case_access), connection_string),
        m_writeNextCaseItemsDirectly(false)
{
    m_useSasMissingCodes = ( m_connectionString.HasProperty(CSProperty::mappedSpecialValues, CSValue::native) ||
                             m_connectionString.HasProperty(_T("mapped-special-values"), _T("software-missing")) ); // pre-8.0

    CreateExportRecordMappings();

    Open();

    InitializeDataSets();

    // start the first data set
    m_sasTransportWriter->StartDirectDataSetWriting(*m_dataSets.front());
}


SasExportWriter::~SasExportWriter()
{
    Close();
}


std::wstring SasExportWriter::GetSyntaxPath(const ConnectionString& connection_string)
{
    const std::wstring* syntax_path_override = connection_string.GetProperty(CSProperty::syntaxPath);

    if( syntax_path_override == nullptr )
        syntax_path_override = connection_string.GetProperty(_T("syntax-file")); // pre-8.0

    return ( syntax_path_override != nullptr ) ? MakeFullPath(GetWorkingFolder(connection_string.GetFilename()), *syntax_path_override) :
                                                 PortableFunctions::PathAppendFileExtension(connection_string.GetFilename(), FileExtensions::WithDot::SasSyntax);
}


void SasExportWriter::Open()
{
    // open the transport file
    SetupEnvironmentToCreateFile(m_connectionString.GetFilename());

    FILE* file = PortableFunctions::FileOpen(m_connectionString.GetFilename(), _T("wb"));

    if( file == nullptr )
        throw CSProException(_T("Could not create the SAS data file: ") + m_connectionString.GetFilename());

    m_sasTransportWriter = std::make_unique<SasTransportWriter>(file);

    // open the syntax file
    m_syntaxFileWriter = std::make_unique<EncodedTextWriter>(m_type, *m_caseAccess, GetSyntaxPath(m_connectionString), m_connectionString);
}


void SasExportWriter::Close()
{
    if( m_sasTransportWriter != nullptr )
    {
        m_sasTransportWriter->StopDirectDataSetWriting();

        // write any data sets that were not directly written
        for( size_t i = 1; i < m_dataSets.size(); ++i )
            m_sasTransportWriter->WriteDataSet(*m_dataSets[i]);

        m_sasTransportWriter.reset();
    }

    if( m_syntaxFileWriter != nullptr )
    {
        WriteSyntaxFile();
        m_syntaxFileWriter.reset();
    }
}


bool SasExportWriter::IsReservedName(const std::wstring& /*name*/, bool /*record_name*/)
{
    // all SAS reserved words begin with a _ so they would not be a valid CSPro name
    return false;
}


void SasExportWriter::InitializeDataSets()
{
    for( std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
        {
            const bool data_set_will_be_written_directly = m_dataSets.empty();

            // add the data set
            SasTransportWriter::DataSet* data_set = m_dataSets.emplace_back(std::make_shared<SasTransportWriter::DataSet>()).get();
            export_record_mapping.tag = data_set;

            data_set->name = CreateSasName(data_set, export_record_mapping.formatted_record_name, m_usedDataSetNames);
            data_set->label = CreateSasLabel(data_set, CS2WS(export_record_mapping.case_record_metadata->GetDictionaryRecord().GetLabel()));


            // add each data variable
            std::set<std::string> used_data_variable_names[2];

            for( ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
            {
                const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

                SasTransportWriter::DataVariable* data_variable = data_set->data_variables.emplace_back(std::make_shared<SasTransportWriter::DataVariable>()).get();
                export_item_mapping.tag = data_variable;

                data_variable->name = CreateSasName(data_variable, export_item_mapping.formatted_item_name, used_data_variable_names);
                data_variable->label = CreateSasLabel(data_variable, CS2WS(dict_item.GetLabel()));

                if( export_item_mapping.case_item->IsTypeNumeric() )
                {
                    data_variable->numeric = true;

                    if( export_item_mapping.case_item->IsTypeFixed() )
                    {
                        data_variable->length = static_cast<short>(dict_item.GetCompleteLen());
                        data_variable->decimals = static_cast<short>(dict_item.GetDecimal());
                        data_variable->format = "F";
                    }

                    else
                    {
                        data_variable->length = 0;
                        data_variable->decimals = 0;
                        data_variable->format = "BEST";
                    }

                    if( !data_set_will_be_written_directly )
                        data_variable->observations = std::make_shared<ExpansiveList<double>>();
                }

                else
                {
                    ASSERT(export_item_mapping.case_item->IsTypeString());

                    data_variable->numeric = false;

                    if( export_item_mapping.case_item->IsTypeFixed() )
                    {
                        data_variable->length = std::min(static_cast<short>(dict_item.GetLen()), SasTransportWriter::MaxTransportStringLength);
                    }

                    else
                    {
                        data_variable->length = SasTransportWriter::MaxTransportStringLength;
                    }

                    data_variable->decimals = 0;
                    data_variable->format = "$";

                    if( !data_set_will_be_written_directly )
                        data_variable->observations = std::make_shared<ExpansiveList<std::string>>();
                }
            }
        }
    }
}


void SasExportWriter::StartRecord(const ExportRecordMapping& export_record_mapping)
{
    const SasTransportWriter::DataSet* data_set = static_cast<const SasTransportWriter::DataSet*>(export_record_mapping.tag);
    m_writeNextCaseItemsDirectly = ( data_set == m_dataSets.front().get() );
}


void SasExportWriter::StartRow()
{
}


void SasExportWriter::EndRow()
{
}


void SasExportWriter::WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index)
{
    const SasTransportWriter::DataVariable* data_variable = static_cast<const SasTransportWriter::DataVariable*>(export_item_mapping.tag);

    // numeric values
    if( export_item_mapping.case_item->IsTypeNumeric() )
    {
        const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);

        double value = numeric_case_item.GetValue(index);

        if( !m_useSasMissingCodes )
            ModifyValueForOutput(numeric_case_item, value);

        if( m_writeNextCaseItemsDirectly )
        {
            m_sasTransportWriter->WriteDirectNumericObservation(value);
        }

        else
        {
            std::get<std::shared_ptr<ExpansiveList<double>>>(data_variable->observations)->AddValue(value);
        }
    }


    // string values
    else
    {
        ASSERT(export_item_mapping.case_item->IsTypeString());
        const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(*export_item_mapping.case_item);

        const std::string value = UTF8Convert::WideToUTF8(string_case_item.GetValue(index));

        if( m_writeNextCaseItemsDirectly )
        {
            m_sasTransportWriter->WriteDirectStringObservation(*data_variable, value);
        }

        else
        {
            std::get<std::shared_ptr<ExpansiveList<std::string>>>(data_variable->observations)->AddValue(value);
        }
    }
}


std::string SasExportWriter::CreateSasName(const void* data_entity, const std::wstring& name, std::set<std::string> used_names[])
{
    auto create_name = [](const std::wstring& base_name, const size_t max_length, std::set<std::string>& used_names)
    {
        for( int i = 0; ; ++i )
        {
            std::wstring sas_name = base_name;

            if( i > 0 )
                SO::AppendFormat(sas_name, _T("_%d"), i);

            std::string utf8_sas_name = UTF8Convert::WideToUTF8(sas_name);

            // if the name is too long, while keeping the first few characters,
            // remove characters from the middle until it is the right length
            if( utf8_sas_name.length() > max_length )
            {
                const size_t characters_at_end_to_keep = ( i == 0 ) ? 0 :
                                                                      static_cast<size_t>(log(i) + 1 + 1);

                utf8_sas_name = utf8_sas_name.substr(0, max_length - characters_at_end_to_keep) +
                                utf8_sas_name.substr(utf8_sas_name.length() - characters_at_end_to_keep);

                ASSERT(utf8_sas_name.length() == max_length);
            }

            // return the name if it hasn't been used
            if( used_names.find(utf8_sas_name) == used_names.cend() )
            {
                used_names.insert(utf8_sas_name);
                return utf8_sas_name;
            }
        }
    };

    // create the name to be used in the syntax file
    const std::string sas_utf8_name = create_name(name, SasTransportWriter::MaxSasNameLength, used_names[0]);
    std::string transport_utf8_name = sas_utf8_name;

    // if necessary, create a valid name for the transport file
    if( transport_utf8_name.length() > SasTransportWriter::MaxTransportNameLength ||
        used_names[1].find(transport_utf8_name) != used_names[1].cend() )
    {
        transport_utf8_name = create_name(name, SasTransportWriter::MaxTransportNameLength, used_names[1]);
    }

    else
    {
        used_names[1].insert(transport_utf8_name);
    }

    if( transport_utf8_name != UTF8Convert::WideToUTF8(name) )
    {
        // store the longer name to be written to the syntax file
        m_renameMap.try_emplace(data_entity, sas_utf8_name);
    }

    return transport_utf8_name;
}


std::string SasExportWriter::CreateSasLabel(const void* data_entity, const std::wstring& label)
{
    std::string utf8_label = UTF8Convert::WideToUTF8(label);

    if( utf8_label.length() > SasTransportWriter::MaxSasLabelLength )
        utf8_label.resize(SasTransportWriter::MaxSasLabelLength);

    if( utf8_label.length() > SasTransportWriter::MaxTransportLabelLength )
    {
        // store the longer label to be written to the syntax file
        m_relabelMap.try_emplace(data_entity, utf8_label);

        utf8_label.resize(SasTransportWriter::MaxTransportLabelLength);
    }

    return utf8_label;
}


inline std::wstring SasExportWriter::EscapeSasLiteral(std::wstring text)
{
    return SO::Replace(text, _T("'"), _T("''"));
}


void SasExportWriter::WriteSyntaxFile()
{
    m_syntaxFileWriter->WriteFormattedLine(_T("libname user '%s';"),
                                           PortableFunctions::PathGetDirectory(m_connectionString.GetFilename()).c_str());
    m_syntaxFileWriter->WriteLine();

    m_syntaxFileWriter->WriteFormattedLine(_T("libname xptfile xport '%s' access=readonly;"),
                                           m_connectionString.GetFilename().c_str());
    m_syntaxFileWriter->WriteLine(_T("proc copy inlib=xptfile outlib=user;"));
    m_syntaxFileWriter->WriteLine();
    m_syntaxFileWriter->WriteLine();  

    CreateFormatsAndWriteSyntax();

    // write out syntax to set the long names/labels and to associate formats
    if( !m_renameMap.empty() || !m_relabelMap.empty() || !m_formatMap.empty() )
    {
        m_syntaxFileWriter->WriteLine(_T("proc datasets nolist library=user;"));
        m_syntaxFileWriter->WriteLine();

        // rename the data sets if necessary
        std::vector<std::string> data_set_names;
        std::vector<size_t> data_set_indices_to_rename;

        for( size_t i = 0; i < m_dataSets.size(); ++i )
        {
            const auto& rename_lookup = m_renameMap.find(m_dataSets[i].get());

            if( rename_lookup == m_renameMap.cend() )
            {
                data_set_names.emplace_back(m_dataSets[i]->name);
            }

            else
            {
                data_set_names.emplace_back(rename_lookup->second);
                data_set_indices_to_rename.emplace_back(i);
            }
        }

        if( !data_set_indices_to_rename.empty() )
        {
            for( size_t i = 0; i < data_set_indices_to_rename.size(); ++i )
            {
                const size_t index = data_set_indices_to_rename[i];

                m_syntaxFileWriter->WriteFormattedLine(_T("\t%-6.6s %s=%s%s"),
                                                       ( i == 0 ) ? _T("change") : _T(""),
                                                       UTF8Convert::UTF8ToWide(m_dataSets[index]->name).c_str(),
                                                       UTF8Convert::UTF8ToWide(data_set_names[index]).c_str(),
                                                       ( ( i + 1 ) == data_set_indices_to_rename.size() ) ? _T(";") : _T(""));
            }

            m_syntaxFileWriter->WriteLine();
        }

        // modify data set labels and process variables in the data set
        for( size_t i = 0; i < m_dataSets.size(); ++i )
            WriteDataSetSyntax(*m_dataSets[i], data_set_names[i]);

        m_syntaxFileWriter->WriteLine(_T("\tquit;"));
        m_syntaxFileWriter->WriteLine();
    }

    m_syntaxFileWriter->WriteLine();  
    m_syntaxFileWriter->WriteLine(_T("run;"));
}


void SasExportWriter::WriteDataSetSyntax(const SasTransportWriter::DataSet& data_set, const std::string& data_set_name)
{
    std::optional<std::string> data_set_label;
    bool modify_written = false;

    auto ensure_modify_written = [&]()
    {
        if( modify_written )
            return;

        std::wstring relabel_text;

        if( data_set_label.has_value() )
            relabel_text = FormatTextCS2WS(_T("(label='%s')"), EscapeSasLiteral(UTF8Convert::UTF8ToWide(*data_set_label)).c_str());

        m_syntaxFileWriter->WriteFormattedLine(_T("\tmodify %s%s;"),
                                               UTF8Convert::UTF8ToWide(data_set_name).c_str(),
                                               relabel_text.c_str());

        modify_written = true;
    };


    // relabel the data set if necessary
    const auto& relabel_data_set_lookup = m_relabelMap.find(&data_set);

    if( relabel_data_set_lookup != m_relabelMap.cend() )
    {
        data_set_label = relabel_data_set_lookup->second;
        ensure_modify_written();
    }

    for( const auto& data_variable : data_set.data_variables )
    {
        std::string data_variable_name = data_variable->name;

        // rename the data variable if necessary
        const auto& rename_lookup = m_renameMap.find(data_variable.get());

        if( rename_lookup != m_renameMap.cend() )
        {
            ensure_modify_written();

            m_syntaxFileWriter->WriteFormattedLine(_T("\trename %s=%s;"),
                                                   UTF8Convert::UTF8ToWide(data_variable_name).c_str(),
                                                   UTF8Convert::UTF8ToWide(rename_lookup->second).c_str());

            data_variable_name = rename_lookup->second;
        }

        // relabel the data variable if necessary and add a format to the data variable if available
        const auto& relabel_data_variable_lookup = m_relabelMap.find(data_variable.get());
        const auto& format_lookup = m_formatMap.find(data_variable.get());

        if( relabel_data_variable_lookup != m_relabelMap.cend() || format_lookup != m_formatMap.cend() )
        {
            std::wstring attrib_line = _T("\tattrib ") + UTF8Convert::UTF8ToWide(data_variable_name);

            if( relabel_data_variable_lookup != m_relabelMap.cend() )
            {
                SO::AppendFormat(attrib_line, _T(" label='%s'"),
                                              EscapeSasLiteral(UTF8Convert::UTF8ToWide(relabel_data_variable_lookup->second)).c_str());
            }

            if( format_lookup != m_formatMap.cend() )
            {
                SO::AppendFormat(attrib_line, _T(" format=%s."),
                                              format_lookup->second.c_str());
            }

            attrib_line.push_back(';');

            ensure_modify_written();

            m_syntaxFileWriter->WriteLine(attrib_line);
        }
    }

    if( modify_written )
        m_syntaxFileWriter->WriteLine();  
}


void SasExportWriter::CreateFormatsAndWriteSyntax()
{
    for( const std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( const ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
        {
            for( const ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
            {
                const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

                if( !dict_item.HasValueSets() )
                    continue;

                const SasTransportWriter::DataVariable* data_variable = static_cast<const SasTransportWriter::DataVariable*>(export_item_mapping.tag);
                const bool numeric = export_item_mapping.case_item->IsTypeNumeric();

                bool value_header_written = false;

                auto write_value_header = [&]()
                {
                    if( value_header_written )
                        return;

                    // create and associate the format name
                    if( m_formatMap.empty() )
                        m_syntaxFileWriter->WriteLine(_T("proc format;"));

                    const std::wstring format_name = FormatTextCS2WS(_T("%sF%06d_"),
                                                                     numeric ? _T("") : _T("$"),
                                                                     static_cast<int>(m_formatMap.size()) + 1);

                    m_formatMap.try_emplace(data_variable, format_name);

                    m_syntaxFileWriter->WriteLine();
                    m_syntaxFileWriter->WriteFormattedLine(_T("\tvalue %s"), format_name.c_str());

                    value_header_written = true;
                };

                // add the labels
                std::shared_ptr<const ValueProcessor> value_processor = ValueProcessor::CreateValueProcessor(dict_item, &dict_item.GetValueSet(0));

                if( numeric )
                {
                    const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);

                    for( const auto& response : value_processor->GetResponses() )
                    {
                        // only add discrete values
                        if( !response->IsDiscrete() )
                            continue;

                        double value = response->GetMinimumValue();
                        std::wstring value_text;

                        // process special values
                        if( IsSpecial(value) )
                        {
                            if( m_suppressMappedSpecialValues || ( value != MISSING && value != REFUSED ) )
                            {
                                continue;
                            }

                            else if( m_useSasMissingCodes && value == MISSING )
                            {
                                value_text = _T(".A");
                            }

                            else if( m_useSasMissingCodes && value == REFUSED )
                            {
                                value_text = _T(".B");
                            }

                            else
                            {
                                ModifyValueForOutput(numeric_case_item, value);
                            }
                        }

                        if( value_text.empty() )
                            value_text = ValueSetResponse::FormatValueForDisplay(dict_item, value);

                        write_value_header();

                        m_syntaxFileWriter->WriteFormattedLine(_T("\t\t%s='%s'"),
                                                               value_text.c_str(),
                                                               EscapeSasLiteral(CS2WS(response->GetLabel())).c_str());
                    }
                }

                else
                {
                    for( const auto& response : value_processor->GetResponses() )
                    {
                        write_value_header();

                        std::string utf8_value = UTF8Convert::WideToUTF8(response->GetCode());
                        utf8_value.resize(data_variable->length, ' ');

                        std::string utf8_label = UTF8Convert::WideToUTF8(response->GetLabel());
                        utf8_label.resize(std::max(utf8_label.length(), static_cast<size_t>(data_variable->length)), ' ');

                        m_syntaxFileWriter->WriteFormattedLine(_T("\t\t'%s'='%s'"),
                                                               EscapeSasLiteral(UTF8Convert::UTF8ToWide(utf8_value)).c_str(),
                                                               EscapeSasLiteral(UTF8Convert::UTF8ToWide(utf8_label)).c_str());
                    }
                }

                if( value_header_written )
                    m_syntaxFileWriter->WriteLine(_T("\t;"));
            }
        }
    }

    if( !m_formatMap.empty() )
    {
        m_syntaxFileWriter->WriteLine();  
        m_syntaxFileWriter->WriteLine();
    }
}
