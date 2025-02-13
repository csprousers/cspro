#pragma once

#include <zUtilO/ExpansiveList.h>


// code based on https://support.sas.com/techsup/technote/ts140.pdf
// and the ReadStat library's implementation

class SasTransportWriter
{
public:
    static constexpr short MaxTransportNameLength   =   8;
    static constexpr short MaxSasNameLength         =  32;

    static constexpr short MaxTransportLabelLength  =  40;
    static constexpr short MaxSasLabelLength        = 256;

    static constexpr short MaxTransportStringLength = 200;

    struct DataVariable
    {
        std::string name;
        std::string label;

        bool numeric;
        short length;
        short decimals;
        std::string format;

        std::variant<std::shared_ptr<ExpansiveList<double>>,
                     std::shared_ptr<ExpansiveList<std::string>>> observations;
    };

    struct DataSet
    {
        std::string name;
        std::string label;
        std::vector<std::shared_ptr<DataVariable>> data_variables;
    };

public:
    // the file must already be open and will henceforth be controlled by this class
    SasTransportWriter(FILE* file);
    ~SasTransportWriter();

    // this method can be used to write a data set with all observations filled
    void WriteDataSet(const DataSet& data_set);

    // these methods can be used to write a data set on a flow basis
    void StartDirectDataSetWriting(const DataSet& data_set);
    void WriteDirectNumericObservation(double value);
    void WriteDirectStringObservation(const DataVariable& data_variable, const std::string& value);
    void StopDirectDataSetWriting();

private:
    void PadToRecordLengthBoundary();

    void WriteRecord(const char* record_text);

    void WriteInitialHeader();

    void WriteHeader(const char* name, int num1 = 0, int num2 = 0, int num3 = 0, int num4 = 0, int num5 = 0, int num6 = 0);

    void WriteDataSetMetadata(const DataSet& data_set);

    void WriteVariablesMetadata(const DataSet& data_set);

    void StartObservations();
    void StopObservations();

    void StartDirectDataSetObservationsIfNecessary();

    void WriteNumericObservation(double value);
    void WriteStringObservation(const DataVariable& data_variable, const std::string& value);

private:
    static constexpr size_t RecordLength = 80;

    FILE* m_file;
    char m_timestamp[17];
    char m_header[RecordLength + 1];

    bool m_writingDataSetDirectly;
    bool m_directDataSetWritingObservationsStarted;
};
