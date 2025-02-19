#include "StandardSystemIncludes.h"
#include "Comp.h"
#include "Engdrv.h"
#include "IntDrive.h"
#include "ParadataDriver.h"
#include <zEngineO/ApplicationLoader.h>
#include <zPlatformO/PlatformInterface.h>
#include <zAppO/Application.h>
#include <zMessageO/Messages.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zMessageO/VariableArgumentsMessageParameterEvaluator.h>
#include <zListingO/ErrorLister.h>
#include <zListingO/Lister.h>
#include <zParadataO/Logger.h>


namespace
{
    class EngineVariableArgumentsMessageParameterEvaluator : public VariableArgumentsMessageParameterEvaluator
    {
    public:
        EngineVariableArgumentsMessageParameterEvaluator(CIntDriver* const interpreter)
            :   m_interpreter(interpreter)
        {
        }

        SharableString GetProc() override
        {
            return ( m_interpreter != nullptr ) ? m_interpreter->ProcName() :
                                                  "Unknown";
        }

    private:
        CIntDriver* m_interpreter;
    };


    class EngineSystemMessageIssuer : public SystemMessageIssuer
    {
    public:
        EngineSystemMessageIssuer(CEngineDriver* const pEngineDriver, Listing::ErrorLister* const compiler_error_lister)
            :   SystemMessageIssuer(std::make_unique<EngineVariableArgumentsMessageParameterEvaluator>(pEngineDriver->m_pIntDriver.get())),
                m_pEngineDriver(pEngineDriver),
                m_callOnSystemMessage(false)
        {
        }

        void SetCallOnSystemMessage()
        {
            m_callOnSystemMessage = true;
        }

    protected:
        void OnIssue(const MessageType message_type, const int message_number, const std::string& message_text) override
        {
            ASSERT(message_type != MessageType::User);

            // special processing for abort messages
            if( message_type == MessageType::Abort )
            {
                if( m_pEngineDriver->GetCompilerErrorLister() != nullptr )
                {
                    m_pEngineDriver->GetCompilerErrorLister()->Write(message_text);
                }

                else if( m_pEngineDriver->GetLister() != nullptr )
                {
                    m_pEngineDriver->GetLister()->Write(message_type, message_number, message_text);
                }
            }

            else if( m_pEngineDriver->GetCompilerErrorLister() == nullptr )
            {
                // quit if the user supressed the system message
                if( m_callOnSystemMessage && !m_pEngineDriver->m_pIntDriver->ExecuteOnSystemMessage(message_type, message_number, message_text) )
                    return;

                // create the paradata event if necessary
                std::unique_ptr<Paradata::MessageEvent> message_event;
                std::unique_ptr<EngineParadataDriver>& paradata_driver = m_pEngineDriver->m_pIntDriver->m_paradataDriver;

                if( Paradata::Logger::IsOpen() )
                    message_event = paradata_driver->CreateMessageEvent(message_type, message_number, message_text);

                // display the message
                m_pEngineDriver->GetSystemMessageManager().IncrementMessageCount(message_number);

                const int selected_button_number = m_pEngineDriver->DisplayMessage(message_type, message_number, message_text, nullptr);

                // log the paradata message event
                if( message_event != nullptr )
                {
                    // there is only one button for system errors
                    if( Issamod == ModuleType::Entry )
                        message_event->SetPostDisplayReturnValue(selected_button_number);

                    paradata_driver->RegisterAndLogEvent(std::move(message_event));
                }
            }

#ifdef WIN_DESKTOP
            else
            {
                // eventually, when all compilation errors use Logic::ParserMessage, all compilation errors
                // should be routed to the method below, but for now, we have to translate the error to a parser message
                const Logic::ParserMessage parser_message = m_pEngineDriver->m_pEngineCompFunc->CreateParserMessageFromIssaError(message_type, message_number, message_text);
                OnIssue(parser_message);
            }
#endif
        }

        void OnIssue(const Logic::ParserMessage& parser_message) override
        {
            // the runtime engine ignores deprecation warnings
            if( parser_message.IsDeprecationWarning() )
                return;

            if( m_pEngineDriver->GetCompilerErrorLister() != nullptr )
            {
                m_pEngineDriver->GetCompilerErrorLister()->Write(parser_message);
            }

            else
            {
                ASSERT(false);
            }
        }

        void OnAbort(const std::string& message_text) override
        {
            m_pEngineDriver->CloseListerAndWriteFiles();

            // JH 11/29/06 - if we got here we are about to show "implement clean up"
            // or some other such less than informative message so at least show the
            // real error first.
            ErrorMessage::Display(message_text);

            WindowsDesktopMessage::Send(WM_IMSA_ENGINEABORT);

#ifndef WIN_DESKTOP
            if( PlatformInterface::GetInstance()->GetApplicationInterface() != nullptr )
                PlatformInterface::GetInstance()->GetApplicationInterface()->EngineAbort();
#endif
            exit(0);
        }

    private:
        CEngineDriver* m_pEngineDriver;
        bool m_callOnSystemMessage;
    };
}


void CEngineDriver::UpdateMessageIssuers(const bool set_on_system_message_defined/* = false*/)
{
    // if an OnSystemMessage function is defined, we only need to update that property
    if( set_on_system_message_defined )
    {
        auto system_message_issuer = std::dynamic_pointer_cast<EngineSystemMessageIssuer, SystemMessageIssuer>(m_systemMessageIssuer);

        if( system_message_issuer != nullptr )
        {
            system_message_issuer->SetCallOnSystemMessage();
            return;
        }
    }

    // otherwise setup the system messages issuer
    m_systemMessageIssuer.reset();

    if( m_pApplication->GetApplicationLoader() != nullptr )
        m_systemMessageIssuer = m_pApplication->GetApplicationLoader()->GetSystemMessageIssuer();

    if( m_systemMessageIssuer == nullptr )
        m_systemMessageIssuer = std::make_unique<EngineSystemMessageIssuer>(this, m_compilerErrorLister.get());

    // setup the user message evaluator
    if( m_userMessageManager != nullptr )
        m_userMessageEvaluator = std::make_unique<MessageEvaluator>(m_userMessageManager->GetSharedMessageFile());
}


int CEngineDriver::DisplayMessage(const MessageType message_type, const int message_number,
                                  SharableString message_text, const MessageSelectDetails* /*select_details*/)
{
    if( m_lister != nullptr )
        m_lister->Write(message_type, message_number, std::move(message_text));

    return 1;
}
