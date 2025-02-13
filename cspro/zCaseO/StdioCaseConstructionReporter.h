#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>


class StdioCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
public:
    StdioCaseConstructionReporter(CStdioFile& warning_file, std::shared_ptr<ProcessSummary> process_summary = nullptr)
        :   StringBasedCaseConstructionReporter(process_summary),
            m_warningFile(warning_file)
    {
    }

protected:
    void WriteString(NullTerminatedString key, NullTerminatedString message) override
    {
        m_warningFile.WriteString(FormatText(_T("*** [%s]\n"), key.c_str()));
        m_warningFile.WriteString(FormatText(_T("*** %s\n\n"), message.c_str()));
    }

private:
    CStdioFile& m_warningFile;
};
