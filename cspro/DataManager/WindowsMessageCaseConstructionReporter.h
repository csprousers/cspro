#pragma once

#include <zToolsO/NewlineSubstitutor.h>
#include <zCaseO/StringBasedCaseConstructionReporter.h>


// --------------------------------------------------------------------------
// WindowsMessageCaseConstructionReporter reports messages to a specific
// window, or to the top window.
// --------------------------------------------------------------------------

class WindowsMessageCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
public:
    WindowsMessageCaseConstructionReporter(CWnd* const parent_wnd, const std::string& dictionary_name)
        :   m_parentWnd(parent_wnd),
            m_messagePrefixes{ dictionary_name + ": ",
                               dictionary_name + "(%s): " }
    {
    }

protected:
    void WriteString(const std::string& key, const std::string message) override
    {
        if( key.empty() )
        {
            ReportMessage(m_messagePrefixes[0] + message);
        }

        else
        {
            // turn \n -> ␤
            const std::string prefix = FormatText(m_messagePrefixes[1].c_str(), NewlineSubstitutor::NewlineToUnicodeNL(key).c_str());

            ReportMessage(prefix + message);
        }
    }

private:
    void ReportMessage(const std::string& message)
    {
        HWND hWnd = ( m_parentWnd != nullptr ) ? m_parentWnd->m_hWnd :
                                                 GetForegroundWindow();

        SendMessage(hWnd, UWM::DataManager::CaseConstructionReporterMessage, reinterpret_cast<WPARAM>(&message), 0);
    }

private:
    CWnd* m_parentWnd;
    std::string m_messagePrefixes[2];
};
