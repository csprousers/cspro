#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>
#include <zMessageO/SystemMessageIssuer.h>


class DataViewerCaseConstructionReporter : public StringBasedCaseConstructionReporter, public SystemMessageIssuer
{
public:
    void Reset()
    {
        m_errors.clear();
    }

    const std::vector<std::wstring>& GetErrors() const
    {
        return m_errors;
    }

protected:
    void WriteString(NullTerminatedString /*key*/, NullTerminatedString message) override
    {
        m_errors.emplace_back(message);
    }

    void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text) override
    {
        m_errors.emplace_back(message_text);
    }

private:
    std::vector<std::wstring> m_errors;
};
