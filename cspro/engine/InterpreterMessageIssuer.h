#pragma once

#include <engine/Engdrv.h>
#include <zEngineO/Messages/MessageIssuer.h>


class InterpreterMessageIssuer : public MessageIssuer
{
public:
    InterpreterMessageIssuer(CEngineDriver* pEngineDriver)
        :   m_pEngineDriver(pEngineDriver)
    {
    }

    void IssueError(int message_number, ...) override
    {
        va_list parg;
        va_start(parg, message_number);

        m_pEngineDriver->GetSystemMessageIssuer().IssueVA(MessageType::Error, message_number, parg);

        va_end(parg);
    }

private:
    CEngineDriver* m_pEngineDriver;
};
