#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>


class OutputWndCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
public:
    OutputWndCaseConstructionReporter(OutputWnd& output_wnd)
        :   m_outputWnd(output_wnd)
    {
    }

protected:
    void WriteString(const std::string& /*key*/, std::string message) override
    {
        ASSERT(m_outputWnd.GetSafeHwnd() != nullptr);

        m_outputWnd.AddText(std::move(message));
    }

private:
    OutputWnd& m_outputWnd;
};
