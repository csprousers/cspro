#pragma once

#include <zUtilO/ConnectionString.h>

class ConcatenatorReporter;


// Concatenate files by appending text files together

class TextConcatenator
{
public:
    void Run(ConcatenatorReporter& concatenator_reporter,
             const std::vector<ConnectionString>& input_connection_strings,
             const ConnectionString& output_connection_string);
};
