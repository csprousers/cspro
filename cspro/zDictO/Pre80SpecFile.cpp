#include "StdAfx.h"
#include "DDClass.h"
#include <zUtilO/Specfile.h>
#include <zAppO/LanguageSerializerHelper.h>


std::wstring SerializeSecurityOptions(const TCHAR* dictionary_name, bool allow_data_viewer_modifications, bool allow_export, int cached_password_minutes);


namespace
{
    class Pre80SpecFileConverter
    {
    public:
        Pre80SpecFileConverter(NullTerminatedString filename, JsonWriter& json_writer, std::vector<std::tuple<CString, CString, int>> lines)
            :   m_jsonWriter(json_writer),
                m_lines(std::move(lines)),
                m_nextLine(0)
        {
            m_directory = PortableFunctions::PathGetDirectory<CString>(filename);
        }

        void Convert()
        {
            m_jsonWriter.BeginObject();

            m_jsonWriter.Write(JK::version, 7.7);
            m_jsonWriter.Write(JK::fileType, JK::dictionary);

            // first read the languages as they are required for proper label processing
            ReadLanguages();
            auto language_serializer_holder = m_jsonWriter.GetSerializerHelper().Register(std::make_shared<LanguageSerializerHelper>(m_languages));

            ReadDictionary();

            m_jsonWriter.EndObject();
        }

    private:
        void ReadLanguages();
        void ReadDictionary();
        void ReadLevels();
        void ReadRecords();
        void ReadItems();
        void ReadValueSets();
        void ReadValues();
        void ReadRelations();

        bool ProcessBase(bool named_entity, CString* name_read = nullptr);

        void WriteAliasesAndNotes(bool named_entity);

        void ProcessOccurrenceLabel(OccurrenceLabels& occurrence_labels, const std::optional<unsigned>& max_occurrences);
        void WriteOccurrencesNode(std::optional<bool> required, unsigned max_occurrences, const OccurrenceLabels& occurrence_labels);


        bool GetLine()
        {
            if( m_nextLine >= m_lines.size() )
                return false;

            std::tie(m_command, m_argument, std::ignore) = m_lines[m_nextLine];
            ++m_nextLine;

            return true;
        }

        bool GetLineIfNotSectionStart()
        {
            if( GetLine() )
            {
                if( !CommandStartsSection() )
                    return true;

                UngetLine();
            }

            return false;
        }

        bool GetLineWhenCommandIs(const TCHAR* command)
        {
            if( GetLine() )
            {
                if( CommandIs(command) )
                    return true;

                UngetLine();
            }

            return false;
        }

        void UngetLine()
        {
            ASSERT(m_nextLine > 0);
            --m_nextLine;
        }

        [[noreturn]] void ThrowInvalidCommand()
        {
            throw CSProException(_T("Unknown command on line %d: %s"), std::get<2>(m_lines[m_nextLine - 1]), (LPCTSTR)m_command);
        }

        bool CommandIs(const TCHAR* text) const
        {
            return ( m_command.CompareNoCase(text) == 0 );
        }

        bool CommandStartsSection() const
        {
            return ( m_command[0] == '[' );
        }

        bool ArgumentIs(const TCHAR* text) const
        {
            return ( m_argument.CompareNoCase(text) == 0 );
        }

        bool ArgumentIsYes() const
        {
            if( !m_argument.IsEmpty() && std::towupper(m_argument[0]) == 'Y' )
            {
                ASSERT(m_argument.CompareNoCase(_T("Yes")) == 0);
                return true;
            }

            return false;
        }


        // argument converters...
        template<typename T> T GetArgument() const;

        // ...as unsigned
        template<> unsigned GetArgument() const
        {
            return (unsigned)_ttoi(m_argument);
        }

        // ...as a LabelSet
        template<> LabelSet GetArgument() const
        {
            return GetLabelSet(m_argument);
        }

        static LabelSet GetLabelSet(const CString& argument)
        {
            constexpr const TCHAR LabelDelimiter = '|';

            LabelSet label_set;

            int last_delimiter_pos = -1;
            int delimiter_pos;
            size_t language_index = 0;

            auto add_label = [&]()
            {
                int label_length = delimiter_pos - last_delimiter_pos - 1;
                label_set.SetLabel(argument.Mid(last_delimiter_pos + 1, label_length), language_index);
                ++language_index;
            };

            while( ( delimiter_pos = argument.Find(LabelDelimiter, last_delimiter_pos + 1) ) != -1 )
            {
                add_label();
                last_delimiter_pos = delimiter_pos;
            }

            // add the last label
            delimiter_pos = argument.GetLength();
            add_label();

            return label_set;
        }


    private:
        JsonWriter& m_jsonWriter;
        CString m_directory;

        std::vector<std::tuple<CString, CString, int>> m_lines;
        size_t m_nextLine;
        CString m_command;
        CString m_argument;

        std::vector<Language> m_languages;
        std::stack<std::set<CString>> m_aliasesStack;
        std::stack<CString> m_notesStack;
    };


    void Pre80SpecFileConverter::ReadLanguages()
    {
        // the languages are defined prior to the levels
        while( GetLine() && !CommandIs(_T("[Level]")) )
        {
            if( CommandIs(_T("[Languages]")) )
            {
                const auto& languages_line_start = m_lines.begin() + m_nextLine - 1;

                while( GetLineIfNotSectionStart() )
                    m_languages.emplace_back((LPCTSTR)m_command, (LPCTSTR)m_argument);

                // write the languages
                if( !m_languages.empty() )
                    m_jsonWriter.Write(JK::languages, m_languages);

                // remove the language lines
                m_lines.erase(languages_line_start, languages_line_start + m_languages.size() + 1);

                break;
            }
        }

        // add a default language if none were defined (as the LanguageSerializerHelper requires this)
        if( m_languages.empty() )
            m_languages.emplace_back();

        // restart the line iterator
        m_nextLine = 0;
    }


    bool Pre80SpecFileConverter::ProcessBase(bool named_entity, CString* name_read/* = nullptr*/)
    {
        ASSERT(!m_notesStack.empty());
        ASSERT(named_entity || m_aliasesStack.size() < m_notesStack.size());

        if( CommandIs(_T("Label")) )
        {
            m_jsonWriter.Write(JK::labels, GetArgument<LabelSet>());
        }

        else if( CommandIs(_T("Note")) )
        {
            if( !m_argument.IsEmpty() )
            {
                m_argument.Replace(_T("\\r\\n"), _T("\n"));

                if( m_argument[m_argument.GetLength() - 1] != '\n' )
                    m_argument.AppendChar(' ');

                if( m_argument[0] == '~' )
                    m_argument.SetAt(0, ' ');

                m_notesStack.top().Append(m_argument);
            }
        }

        else if( named_entity && CommandIs(_T("Name")) )
        {
            if( name_read != nullptr )
                *name_read = m_argument;

            m_jsonWriter.Write(JK::name, m_argument);
        }

        else if( named_entity && CommandIs(_T("Alias")) )
        {
            m_aliasesStack.top().insert(m_argument);
        }

        else
        {
            return false;
        }

        return true;
    }


    void Pre80SpecFileConverter::WriteAliasesAndNotes(bool named_entity)
    {
        if( named_entity && !m_aliasesStack.top().empty() )
            m_jsonWriter.Write(JK::aliases, m_aliasesStack.top());

        m_jsonWriter.WriteIfNotBlank(JK::note, m_notesStack.top());
    }


    void Pre80SpecFileConverter::ProcessOccurrenceLabel(OccurrenceLabels& occurrence_labels, const std::optional<unsigned>& max_occurrences)
    {
        int separator_pos = m_argument.FindOneOf(_T(",:"));

        if( separator_pos > 0 )
        {
            size_t occurrence = (size_t)_ttoi(m_argument.Left(separator_pos)) - 1;

            if( occurrence >= 0 && occurrence < max_occurrences.value_or(UINT_MAX) )
                occurrence_labels.SetLabels(occurrence, GetLabelSet(m_argument.Mid(separator_pos + 1)));
        }
    }

    void Pre80SpecFileConverter::WriteOccurrencesNode(std::optional<bool> required, unsigned max_occurrences, const OccurrenceLabels& occurrence_labels)
    {
        m_jsonWriter.BeginObject(JK::occurrences);

        if( required.has_value() )
            m_jsonWriter.Write(JK::required, *required);

        m_jsonWriter.Write(JK::maximum, max_occurrences);

        if( max_occurrences > 1 && !occurrence_labels.GetLabels().empty() )
        {
            m_jsonWriter.Key(JK::labels);
            occurrence_labels.WriteJson(m_jsonWriter, max_occurrences);
        }

        m_jsonWriter.EndObject();
    }


    void Pre80SpecFileConverter::ReadDictionary()
    {
        RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
        RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());
        CString name;
        unsigned record_type_start = DictionaryDefaults::RecTypeStart;
        unsigned record_type_length = DictionaryDefaults::RecTypeLen;
        bool zero_fill = DictionaryDefaults::ZeroFill;
        bool dec_char = DictionaryDefaults::DecChar;
        bool version_before_74 = false;

        while( GetLine() )
        {
            if( CommandIs(_T("[NoEdit]")) ) // 20110824
            {
                m_jsonWriter.Write(JK::editable, false);
            }

            else if( CommandIs(_T("Version")) )
            {
                if( m_argument.Compare(_T("CSPro 7.4")) < 0 )
                    version_before_74 = true;
            }

            else if( ProcessBase(true, &name) )
            {
                // processed in the method
            }

            else if( CommandIs(_T("RecordTypeStart")) )
            {
                record_type_start = GetArgument<unsigned>();
            }

            else if( CommandIs(_T("RecordTypeLen")) )
            {
                record_type_length = GetArgument<unsigned>();
            }

            else if( CommandIs(_T("Positions")) )
            {
                m_jsonWriter.Write(JK::relativePositions, ArgumentIs(_T("Relative")));
            }

            else if( CommandIs(_T("ZeroFill")) )
            {
                zero_fill = ArgumentIsYes();
            }

            else if( CommandIs(_T("DecimalChar")) )
            {
                dec_char = ArgumentIsYes();
            }

            else if( CommandIs(_T("SecurityOptions")) )
            {
                if( version_before_74 )
                {
                    ASSERT(false);
                    version_before_74 = false;
                }

                m_jsonWriter.BeginObject(JK::security)
                            .Write(JK::settings, m_argument)
                            .EndObject();
            }

            else if( CommandIs(_T("[Level]")) )
            {
                m_jsonWriter.BeginArray(JK::levels);
                ReadLevels();
                m_jsonWriter.EndArray();
            }

            else if( CommandIs(_T("[Relation]")) )
            {
                m_jsonWriter.BeginArray(JK::relations);
                ReadRelations();
                m_jsonWriter.EndArray();
            }

            else if( !CommandIs(_T("[Dictionary]")) &&
                     !CommandIs(_T("ValueSetImages")) )
            {
                ThrowInvalidCommand();
            }
        }

        if( version_before_74 )
        {
            // for older dictionaries, default to allowing the dictionary to be used for exports
            m_jsonWriter.BeginObject(JK::security)
                        .Write(JK::settings, SerializeSecurityOptions(name, false, true, 0))
                        .EndObject();
        }

        m_jsonWriter.BeginObject(JK::recordType)
                    .Write(JK::start, record_type_start)
                    .Write(JK::length, record_type_length)
                    .EndObject();

        m_jsonWriter.BeginObject(JK::defaults)
                    .Write(JK::decimalMark, dec_char)
                    .Write(JK::zeroFill, zero_fill)
                    .EndObject();

        WriteAliasesAndNotes(true);
    }


    void Pre80SpecFileConverter::ReadLevels()
    {
        for( bool read_more_entries = true; read_more_entries; )
        {
            m_jsonWriter.BeginObject();

            RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
            RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());
            bool next_line_should_be_an_item = false;

            while( ( read_more_entries = GetLine() ) == true )
            {
                if( ProcessBase(true) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("[IdItems]")) )
                {
                    next_line_should_be_an_item = true;
                }

                else if( CommandIs(_T("[Item]")) && next_line_should_be_an_item )
                {
                    next_line_should_be_an_item = false;

                    m_jsonWriter.BeginObject(JK::ids);

                    m_jsonWriter.BeginArray(JK::items);
                    ReadItems();
                    m_jsonWriter.EndArray();

                    m_jsonWriter.EndObject();
                }

                else if( CommandIs(_T("[Record]")) )
                {
                    m_jsonWriter.BeginArray(JK::records);
                    ReadRecords();
                    m_jsonWriter.EndArray();
                }

                else if( CommandStartsSection() )
                {
                    if( !CommandIs(_T("[Level]")) )
                    {
                        read_more_entries = false;
                        UngetLine();
                    }

                    break;
                }

                else
                {
                    ThrowInvalidCommand();
                }

                ASSERT(!next_line_should_be_an_item || CommandIs(_T("[IdItems]")));
            }

            WriteAliasesAndNotes(true);

            m_jsonWriter.EndObject();
        }
    }


    void Pre80SpecFileConverter::ReadRecords()
    {
        for( bool read_more_entries = true; read_more_entries; )
        {
            m_jsonWriter.BeginObject();

            RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
            RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());
            bool required = DictionaryDefaults::Required;
            std::optional<unsigned> max_recs;
            OccurrenceLabels occurrence_labels;

            while( ( read_more_entries = GetLine() ) == true )
            {
                if( ProcessBase(true) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("RecordTypeValue")) )
                {
                    m_jsonWriter.Write(JK::recordType, (LPCTSTR)CIMSAString(m_argument).GetToken());
                }

                else if( CommandIs(_T("Required")) )
                {
                    required = ArgumentIsYes();
                }

                else if( CommandIs(_T("MaxRecords")) )
                {
                    max_recs = GetArgument<unsigned>();
                }

                else if( CommandIs(_T("OccurrenceLabel")) || CommandIs(_T("OccurenceLabel")) )
                {
                    // 20140226 CMD_OCCLABEL was spelled incorrectly, so we add the original misspelling above
                    ProcessOccurrenceLabel(occurrence_labels, max_recs);
                }

                else if( CommandIs(_T("[Item]")) )
                {
                    m_jsonWriter.BeginArray(JK::items);
                    ReadItems();
                    m_jsonWriter.EndArray();
                }

                else if( CommandStartsSection() )
                {
                    if( !CommandIs(_T("[Record]")) )
                    {
                        read_more_entries = false;
                        UngetLine();
                    }

                    break;
                }

                else if( !CommandIs(_T("RecordLen")) )
                {
                    ThrowInvalidCommand();
                }
            }

            WriteOccurrencesNode(required, max_recs.value_or(DictionaryDefaults::MaxRecs), occurrence_labels);

            WriteAliasesAndNotes(true);

            m_jsonWriter.EndObject();
        }
    }


    void Pre80SpecFileConverter::ReadItems()
    {
        for( bool read_more_entries = true; read_more_entries; )
        {            
            m_jsonWriter.BeginObject();

            RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
            RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());
            std::optional<unsigned> occurs;
            std::optional<std::wstring> content;
            bool dec_char = DictionaryDefaults::DecChar;
            bool zero_fill = DictionaryDefaults::ZeroFill;
            CaptureInfo capture_info;
            OccurrenceLabels occurrence_labels;

            while( ( read_more_entries = GetLine() ) == true )
            {
                if( ProcessBase(true) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("ItemType")) )
                {
                    m_jsonWriter.Write(JK::subitem, ArgumentIs(_T("SubItem")));
                }

                else if( CommandIs(_T("DataType")) )
                {
                    content = SO::TitleToCamelCase(CS2WS(m_argument));
                }

                else if( CommandIs(_T("Start")) )
                {
                    m_jsonWriter.Write(JK::start, GetArgument<unsigned>());
                }

                else if( CommandIs(_T("Len")) )
                {
                    m_jsonWriter.Write(JK::length, GetArgument<unsigned>());
                }

                else if( CommandIs(_T("Occurrences")) )
                {
                    occurs = GetArgument<unsigned>();
                }

                else if( CommandIs(_T("Decimal")) )
                {
                    m_jsonWriter.Write(JK::decimals, GetArgument<unsigned>());
                }
                
                else if( CommandIs(_T("DecimalChar")) )
                {
                    dec_char = ArgumentIsYes();                    
                }

                else if( CommandIs(_T("ZeroFill")) )
                {
                    zero_fill = ArgumentIsYes();
                }

                else if( CommandIs(_T("CaptureType")) )
                {
                    capture_info.SetCaptureType(CaptureInfo::GetCaptureTypeFromSerializableName(m_argument).value_or(CaptureType::Unspecified));
                }

                else if( CommandIs(L"CaptureDateFormat") && capture_info.GetCaptureType() == CaptureType::Date )
                {
                    capture_info.GetExtended<DateCaptureInfo>().SetFormat(m_argument);
                }

                else if( CommandIs(_T("OccurrenceLabel")) || CommandIs(_T("OccurenceLabel")) )
                {
                    ProcessOccurrenceLabel(occurrence_labels, occurs);
                }

                else if( CommandIs(_T("[ValueSet]")) )
                {
                    m_jsonWriter.BeginArray(JK::valueSets);
                    ReadValueSets();
                    m_jsonWriter.EndArray();
                }

                else if( CommandStartsSection() )
                {
                    if( !CommandIs(_T("[Item]")) )
                    {
                        read_more_entries = false;
                        UngetLine();
                    }

                    break;
                }

                else
                {
                    ThrowInvalidCommand();
                }
            }

            m_jsonWriter.Write(JK::contentType, content.value_or(_T("numeric")));
            m_jsonWriter.Write(JK::decimalMark, dec_char);
            m_jsonWriter.Write(JK::zeroFill, zero_fill);

            if( capture_info.IsSpecified() )
                m_jsonWriter.Write(JK::capture, capture_info);

            if( occurs.has_value() )
                WriteOccurrencesNode(std::nullopt, *occurs, occurrence_labels);

            WriteAliasesAndNotes(true);

            m_jsonWriter.EndObject();
        }
    }


    void Pre80SpecFileConverter::ReadValueSets()
    {
        for( bool read_more_entries = true; read_more_entries; )
        {
            m_jsonWriter.BeginObject();

            RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
            RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());

            while( ( read_more_entries = GetLine() ) == true )
            {
                if( ProcessBase(true) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("Link")) )
                {
                    m_jsonWriter.Write(JK::link, m_argument);
                }

                else if( CommandIs(_T("Value")) )
                {
                    m_jsonWriter.BeginArray(JK::values);
                    ReadValues();
                    m_jsonWriter.EndArray();
                }

                else if( CommandStartsSection() )
                {
                    if( !CommandIs(_T("[ValueSet]")) )
                    {
                        read_more_entries = false;
                        UngetLine();
                    }

                    break;
                }

                else
                {
                    ThrowInvalidCommand();
                }
            }

            WriteAliasesAndNotes(true);

            m_jsonWriter.EndObject();
        }
    }


    void Pre80SpecFileConverter::ReadValues()
    {
        for( bool read_more_entries = true; read_more_entries; )
        {
            m_jsonWriter.BeginObject();

            ASSERT(CommandIs(_T("Value")));
            CIMSAString value_text = m_argument;

            // process the label
            LabelSet label_set;
            int semicolon_pos = value_text.Find(';');

            if( semicolon_pos >= 0 )
            {
                label_set = GetLabelSet(value_text.Mid(semicolon_pos + 1));
                value_text = value_text.Left(semicolon_pos);
            }

            m_jsonWriter.Write(JK::labels, label_set);

            // process the value pairs
            m_jsonWriter.BeginArray(JK::pairs);

            while( !value_text.IsEmpty() )
            {
                TCHAR token;
                CString from = value_text.GetToken(_T(" :"), &token);
                CString to;

                if( token == ':' )
                    to = value_text.GetToken(_T(" "));

                if( !from.IsEmpty() )
                {
                    m_jsonWriter.BeginObject();

                    if( to.IsEmpty() )
                    {
                        m_jsonWriter.Write(JK::value, from);
                    }

                    else
                    {
                        m_jsonWriter.BeginArray(JK::range)
                                    .Write(from)
                                    .Write(to)
                                    .EndArray();
                    }

                    m_jsonWriter.EndObject();
                }
            }

            m_jsonWriter.EndArray();

            // process any other attributes
            RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());

            while( ( read_more_entries = GetLine() ) == true && !CommandIs(_T("Value")) )
            {
                if( ProcessBase(false) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("Name")) )
                {
                    CString special_value = CIMSAString(m_argument).GetToken();
                    ASSERT(SpecialValues::StringIsSpecial(special_value));
                    m_jsonWriter.Write(JK::special, special_value);
                }

                else if( CommandIs(_T("Image")) )
                {
                    m_jsonWriter.Write(JK::image, MakeFullPath(m_directory, CS2WS(m_argument)));
                }

                else if( CommandIs(_T("TextColor")) )
                {
                    m_jsonWriter.Write(JK::textColor, m_argument);
                }

                else if( CommandStartsSection() )
                {
                    read_more_entries = false;
                    UngetLine();
                    break;
                }

                else
                {
                    ThrowInvalidCommand();
                }
            }

            WriteAliasesAndNotes(false);

            m_jsonWriter.EndObject();
        }
    }


    void Pre80SpecFileConverter::ReadRelations()
    {
        // relations do not have aliases or notes but these are necessary to call ProcessBase
        RAII::PushOnStackAndPopOnDestruction aliases_holder(m_aliasesStack, std::set<CString>());
        RAII::PushOnStackAndPopOnDestruction notes_holder(m_notesStack, CString());

        do
        {
            m_jsonWriter.BeginObject();

            CString primary_name;
            struct Part { CString primary_link; CString secondary_name; CString secondary_link; };
            std::vector<Part> parts;

            while( GetLineIfNotSectionStart() )
            {
                if( ProcessBase(true) )
                {
                    // processed in the method
                }

                else if( CommandIs(_T("Primary")) )
                {
                    primary_name = m_argument;
                }

                else if( CommandIs(_T("PrimaryLink")) || CommandIs(_T("Secondary")) )
                {
                    auto& part = parts.emplace_back();

                    do
                    {
                        if( CommandIs(_T("PrimaryLink")) )
                        {
                            if( !part.secondary_name.IsEmpty() )
                            {
                                UngetLine();
                                break;
                            }

                            part.primary_link = m_argument;
                        }

                        else if( CommandIs(_T("Secondary")) )
                        {
                            if( !part.secondary_name.IsEmpty() )
                            {
                                UngetLine();
                                break;
                            }

                            part.secondary_name = m_argument;
                        }

                        else if( CommandIs(_T("SecondaryLink")) )
                        {
                            part.secondary_link = m_argument;
                        }

                        else
                        {
                            ThrowInvalidCommand();
                        }

                    } while( GetLineIfNotSectionStart() );
                }

                else
                {
                    ThrowInvalidCommand();
                }
            }

            m_jsonWriter.Write(JK::primary, primary_name);

            m_jsonWriter.WriteObjects(JK::links, parts,
                [&](const Part& part)
                {
                   m_jsonWriter.WriteIfNotBlank(JK::primaryLink, part.primary_link)
                               .WriteIfNotBlank(JK::secondary, part.secondary_name)
                               .WriteIfNotBlank(JK::secondaryLink, part.secondary_link);
                });

            m_jsonWriter.EndObject();

        } while( GetLineWhenCommandIs(_T("[Relation]")) );
    }    
}


std::wstring CDataDict::ConvertPre80SpecFile(NullTerminatedString filename)
{
    // read the entire spec file in one shot
    CSpecFile specfile;
    std::vector<std::tuple<CString, CString, int>> lines;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the dictionary: %s"), filename.c_str());

    while( true )
    {
        auto& pair = lines.emplace_back();

        if( specfile.GetLine(std::get<0>(pair), std::get<1>(pair), true) == SF_OK )
        {
            ASSERT(!std::get<0>(pair).IsEmpty());
            std::get<2>(pair) = specfile.GetLineNumber();
        }

        else
        {
            lines.resize(lines.size() - 1);
            break;
        }        
    }

    specfile.Close();

    try
    {
        auto json_writer = Json::CreateStringWriter();

        Pre80SpecFileConverter converter(filename, *json_writer, std::move(lines));
        converter.Convert();

        return json_writer->GetString();
    }

    catch( const CSProException& exception )
    {
        throw CSProException(_T("There was an error reading the dictionary %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }    
}
