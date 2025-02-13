#pragma once

#include <zConcatO/zConcatO.h>

class CaseConstructionReporter;
class CDataDict;
class ConcatenatorReporter;
class ConnectionString;
class PFF;


class ZCONCATO_API Concatenator
{
public:
    enum class RunSuccess { Success, SuccessWithErrors, UserCanceled, Errors };

    RunSuccess Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> dictionary = nullptr);

    void Run(ConcatenatorReporter& concatenator_reporter,
             const std::vector<ConnectionString>& input_connection_strings, const ConnectionString& output_connection_string,
             std::shared_ptr<const CDataDict> dictionary = nullptr, std::shared_ptr<CaseConstructionReporter> case_construction_reporter = nullptr);
};
