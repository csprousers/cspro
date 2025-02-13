#pragma once

class CaseConstructionReporter;
class CDataDict;
class ConcatenatorReporter;
class ConnectionString;


// Concatenate files by copying cases and checking for duplicates.

class CaseConcatenator
{
public:
    void Run(ConcatenatorReporter& concatenator_reporter,
             const std::vector<ConnectionString>& input_connection_strings, const ConnectionString& output_connection_string,
             std::shared_ptr<const CDataDict> dictionary, std::shared_ptr<CaseConstructionReporter> case_construction_reporter);
};
