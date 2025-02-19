#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>


class StdioCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
public:
    StdioCaseConstructionReporter(FileIO::TextFile& warning_file, std::shared_ptr<ProcessSummary> process_summary = nullptr)
        :   StringBasedCaseConstructionReporter(std::move(process_summary)),
            m_warningFile(warning_file)
    {
    }

protected:
    void WriteString(const std::string& key, const std::string message) override
    {
        m_warningFile.WriteFormattedLine("*** [%s]", key.c_str());
        m_warningFile.WriteFormattedLine("*** %s\n", message.c_str());
    }

private:
    FileIO::TextFile& m_warningFile;
};
