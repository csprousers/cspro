#include "stdafx.h"
#include "SasTransportWriter.h"
#include <zToolsO/Special.h>

extern "C"
{
#include <external/ReadStat/readstat_bits.h>
#include <external/ReadStat/sas/ieee.h>
}

const short SasTransportWriter::MaxTransportStringLength; // a definition (of a short) is needed for clang


namespace
{
    constexpr char* const SasVersion = "6.12";  // the last version of 6
    constexpr char* const SasOs      = "CSPro";

    inline void AssignShort(short& destination, const short source)
    {
        destination = machine_is_little_endian() ? byteswap2(source) :
                                                   source;
    }

    inline void AssignLong(long& destination, const long source)
    {
        destination = machine_is_little_endian() ? byteswap4(source) :
                                                   source;
    }

    inline void AssignPaddedString(char* destination, const size_t destination_size, const std::string& source)
    {
        const size_t chars_to_copy = std::min(destination_size, source.length());
        memcpy(destination, source.c_str(), chars_to_copy);
        memset(destination + chars_to_copy, ' ', destination_size - chars_to_copy);
    }
}


SasTransportWriter::SasTransportWriter(FILE* file)
    :   m_file(file),
        m_writingDataSetDirectly(false),
        m_directDataSetWritingObservationsStarted(false)
{
    ASSERT(m_file != nullptr && PortableFunctions::ftelli64(m_file) == 0);

    WriteInitialHeader();
}


SasTransportWriter::~SasTransportWriter()
{
    if( m_file != nullptr )
        fclose(m_file);
}


void SasTransportWriter::PadToRecordLengthBoundary()
{
    const int64_t position = PortableFunctions::ftelli64(m_file);
    int64_t position_in_boundary = position % static_cast<int64_t>(RecordLength);

    if( position_in_boundary > 0 )
    {
        while( position_in_boundary++ < RecordLength )
            fputc(' ', m_file);
    }
}


void SasTransportWriter::WriteRecord(const char* record_text)
{
    const size_t record_length = strlen(record_text);
    ASSERT(record_length <= RecordLength);

    fwrite(record_text, 1, record_length, m_file);

    PadToRecordLengthBoundary();
}


void SasTransportWriter::WriteInitialHeader()
{
    // format the timestamp
    constexpr char* const Months[] =
    {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };

    struct tm tp = GetLocalTime();

    snprintf(m_timestamp, sizeof(m_timestamp), "%02d%s%02d:%02d:%02d:%02d",
                                               tp.tm_mday, Months[tp.tm_mon], tp.tm_year,
                                               tp.tm_hour, tp.tm_min, tp.tm_sec);


    // write the initial header
    WriteRecord("HEADER RECORD*******LIBRARY HEADER RECORD!!!!!!!000000000000000000000000000000");


    // write the creation header
    snprintf(m_header, sizeof(m_header), "%-8.8s" "%-8.8s" "%-8.8s"  "%-8.8s"    "%-8.8s" "%-24.24s" "%16.16s",
                                         "SAS",   "SAS",   "SASLIB", SasVersion, SasOs,   "",        m_timestamp);

    WriteRecord(m_header);


    // write the modification header
    WriteRecord(m_timestamp);
}


void SasTransportWriter::WriteHeader(const char* name,
                                     const int num1/* = 0*/, const int num2/* = 0*/, const int num3/* = 0*/,
                                     const int num4/* = 0*/, const int num5/* = 0*/, const int num6/* = 0*/)
{
    snprintf(m_header, sizeof(m_header), "HEADER RECORD*******%-8sHEADER RECORD!!!!!!!" "%05d%05d%05d" "%05d%05d%05d",
                                         name, num1, num2, num3, num4, num5, num6);

    WriteRecord(m_header);
}


void SasTransportWriter::WriteDataSet(const DataSet& data_set)
{
    ASSERT(!m_writingDataSetDirectly);

    WriteDataSetMetadata(data_set);

    // write the observations
    size_t number_observations = 0;

    if( !data_set.data_variables.empty() )
    {
        std::visit(
            [&](const auto& observations)
            {
                ASSERT(observations != nullptr);
                number_observations = observations->GetSize();

            }, data_set.data_variables.front()->observations);
    }

    if( number_observations == 0 )
        return;

    StartObservations();

    for( size_t i = 0; i < number_observations; ++i )
    {
        for( const auto& data_variable : data_set.data_variables )
        {
            if( data_variable->numeric )
            {
                ASSERT(std::holds_alternative<std::shared_ptr<ExpansiveList<double>>>(data_variable->observations));
                const auto& numeric_expansive_list = std::get<std::shared_ptr<ExpansiveList<double>>>(data_variable->observations);
                ASSERT(numeric_expansive_list->GetSize() == number_observations);

                double value;
                const bool has_value = numeric_expansive_list->GetValue(value);
                ASSERT(has_value && ( ( i + 1 ) < number_observations ) || !numeric_expansive_list->GetValue(value));

                WriteNumericObservation(value);
            }

            else
            {
                ASSERT(std::holds_alternative<std::shared_ptr<ExpansiveList<std::string>>>(data_variable->observations));
                const auto& string_expansive_list = std::get<std::shared_ptr<ExpansiveList<std::string>>>(data_variable->observations);
                ASSERT(string_expansive_list->GetSize() == number_observations);

                std::string value;
                bool has_value = string_expansive_list->GetValue(value);
                ASSERT(has_value && ( ( i + 1 ) < number_observations ) || !string_expansive_list->GetValue(value));

                WriteStringObservation(*data_variable, value);
            }
        }
    }

    StopObservations();
}


void SasTransportWriter::StartDirectDataSetWriting(const DataSet& data_set)
{
    ASSERT(!m_writingDataSetDirectly);

    m_writingDataSetDirectly = true;
    m_directDataSetWritingObservationsStarted = false;

    WriteDataSetMetadata(data_set);
}


void SasTransportWriter::StartDirectDataSetObservationsIfNecessary()
{
    ASSERT(m_writingDataSetDirectly);

    if( !m_directDataSetWritingObservationsStarted )
    {
        StartObservations();
        m_directDataSetWritingObservationsStarted = true;
    }
}


void SasTransportWriter::WriteDirectNumericObservation(const double value)
{
    StartDirectDataSetObservationsIfNecessary();
    WriteNumericObservation(value);
}


void SasTransportWriter::WriteDirectStringObservation(const DataVariable& data_variable, const std::string& value)
{
    StartDirectDataSetObservationsIfNecessary();
    WriteStringObservation(data_variable, value);
}


void SasTransportWriter::StopDirectDataSetWriting()
{
    ASSERT(m_writingDataSetDirectly);

    if( m_directDataSetWritingObservationsStarted )
        StopObservations();

    m_writingDataSetDirectly = false;
}


void SasTransportWriter::WriteDataSetMetadata(const DataSet& data_set)
{
    WriteHeader("MEMBER", 0, 0, 0, 160, 0, 140);
    WriteHeader("DSCRPTR");


    // write the first member header (the data set name)
    snprintf(m_header, sizeof(m_header), "%-8.8s" "%-8.8s"               "%-8.8s"   "%-8.8s"    "%-8.8s" "%-24.24s" "%16.16s",
                                         "SAS",   data_set.name.c_str(), "SASDATA", SasVersion, SasOs,   "",        m_timestamp);

    WriteRecord(m_header);


    // write the second member header (the data set label)
    snprintf(m_header, sizeof(m_header), "%16.16s"    "%-24.24s" "%-40.40s"              "%-8.8s",
                                         m_timestamp, "",        data_set.label.c_str(), "");

    WriteRecord(m_header);


    // write the number of variables
    WriteHeader("NAMESTR", 0, static_cast<int>(data_set.data_variables.size()));


    // write information about each variable
    WriteVariablesMetadata(data_set);
}


void SasTransportWriter::WriteVariablesMetadata(const DataSet& data_set)
{
    short variable_number = 1;
    long position_in_observation = 0;

    for( const auto& data_variable : data_set.data_variables )
    {
        struct NAMESTR
        {
            short ntype;     // VARIABLE TYPE: 1=NUMERIC, 2=CHAR
            short nhfun;     // HASH OF NNAME (always 0)
            short nlng;      // LENGTH OF VARIABLE IN OBSERVATION
            short nvar0;     // VARNUM
            char nname[8];   // NAME OF VARIABLE
            char nlabel[40]; // LABEL OF VARIABLE
            char nform[8];   // NAME OF FORMAT
            short nfl;       // FORMAT FIELD LENGTH OR 0
            short nfd;       // FORMAT NUMBER OF DECIMALS
            short nfj;       // 0=LEFT JUSTIFICATION, 1=RIGHT JUST
            char nfill[2];   // (UNUSED, FOR ALIGNMENT AND FUTURE)
            char niform[8];  // NAME OF INPUT FORMAT
            short nifl;      // INFORMAT LENGTH ATTRIBUTE
            short nifd;      // INFORMAT NUMBER OF DECIMALS
            long npos;       // POSITION OF VALUE IN OBSERVATION
            char rest[52];   // remaining fields are irrelevant
        };

        NAMESTR namestr { 0 };

        const short variable_width = data_variable->numeric ? 8 : data_variable->length;

        AssignShort(namestr.ntype, data_variable->numeric ? 1 : 2);
        AssignShort(namestr.nlng, variable_width);
        AssignShort(namestr.nvar0, variable_number);
        AssignPaddedString(namestr.nname, sizeof(namestr.nname), data_variable->name);
        AssignPaddedString(namestr.nlabel, sizeof(namestr.nlabel), data_variable->label);
        AssignPaddedString(namestr.nform, sizeof(namestr.nform), data_variable->format);
        AssignShort(namestr.nfl, data_variable->length);
        AssignShort(namestr.nfd, data_variable->decimals);
        AssignShort(namestr.nfj, data_variable->numeric ? 1 : 0);
        AssignPaddedString(namestr.niform, sizeof(namestr.niform), data_variable->numeric ? "F" : "$");
        AssignShort(namestr.nifl, data_variable->length);
        AssignShort(namestr.nifd, data_variable->decimals);
        AssignLong(namestr.npos, position_in_observation);

        fwrite(&namestr, 1, sizeof(namestr), m_file);

        ++variable_number;
        position_in_observation += variable_width;
    }

    PadToRecordLengthBoundary();
}


void SasTransportWriter::StartObservations()
{
    WriteHeader("OBS");
}


void SasTransportWriter::StopObservations()
{
    PadToRecordLengthBoundary();
}


void SasTransportWriter::WriteNumericObservation(const double value)
{
    static char MissingValues[8] = { 0 };
    const size_t size_value_to_write = sizeof(MissingValues);

    auto write_value = [&](const void* data)
    {
        fwrite(data, 1, size_value_to_write, m_file);
    };

    auto write_missing_value = [&](const char first_byte)
    {
        MissingValues[0] = first_byte;
        write_value(MissingValues);
    };

    // convert non-missing values to IBM mainframe format
    if( !IsSpecial(value) || value == DEFAULT )
    {
        char full_value[8];
        static_assert(sizeof(full_value) == size_value_to_write);

        cnxptiee(&value, CN_TYPE_NATIVE, full_value, CN_TYPE_XPORT);
        write_value(full_value);
    }

    // use the missing . code for blank values
    else if( value == NOTAPPL )
    {
        write_missing_value(0x2E);
    }

    // use the missing .A code for missing values
    else if( value == MISSING )
    {
        write_missing_value(0x41);
    }

    // use the missing .B code for refused values
    else
    {
        ASSERT(value == REFUSED);
        write_missing_value(0x42);
    }
}


void SasTransportWriter::WriteStringObservation(const DataVariable& data_variable, const std::string& value)
{
    ASSERT(data_variable.length <= MaxTransportStringLength);

    auto write_value = [&](const std::string& properly_sized_value)
    {
        fwrite(properly_sized_value.data(), 1, data_variable.length, m_file);
    };

    if( value.size() == static_cast<size_t>(data_variable.length) )
    {
        write_value(value);
    }

    else
    {
        std::string resized_value = value;
        resized_value.resize(data_variable.length, ' ');
        write_value(resized_value);
    }
}
