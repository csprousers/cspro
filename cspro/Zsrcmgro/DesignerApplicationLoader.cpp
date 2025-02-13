#include "StdAfx.h"
#include "DesignerApplicationLoader.h"
#include "Compiler.h"
#include "DesignerCompilerMessageProcessor.h"
#include <zToolsO/Hash.h>
#include <zToolsO/PortableFunctions.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zMessageO/SystemMessages.h>
#include <zMessageO/VariableArgumentsMessageParameterEvaluator.h>
#include <zDesignerF/CodeMenu.h>
#include <engine/Comp.h>
#include <engine/Engdrv.h>


namespace
{
    // --------------------------------------------------------------------------
    // cached compilation handling
    // --------------------------------------------------------------------------
    namespace CachedObjects
    {
        std::optional<std::tuple<size_t, std::shared_ptr<MessageManager>>> user_message_manager;

        size_t GetCacheKey(const std::vector<std::shared_ptr<TextSource>>& text_sources)
        {
            size_t cache_key = 0;
    
            for( const TextSource& message_text_source : VI_V(text_sources) )
            {
                Hash::Combine(cache_key, &message_text_source);
                Hash::Combine(cache_key, message_text_source.GetModifiedIteration());
            }

            return cache_key;
        }
    }


    // --------------------------------------------------------------------------
    // compilation error handling
    // --------------------------------------------------------------------------
    class DesignerVariableArgumentsMessageParameterEvaluator : public VariableArgumentsMessageParameterEvaluator
    {
    public:
        DesignerVariableArgumentsMessageParameterEvaluator(const DesignerCompilerMessageProcessor* designer_compiler_message_processor)
            :   m_designerCompilerMessageProcessor(designer_compiler_message_processor)
        {
        }

        std::wstring GetProc() override
        {
            return CS2WS(m_designerCompilerMessageProcessor->GetProcName());
        }

    private:
        const DesignerCompilerMessageProcessor* m_designerCompilerMessageProcessor;
    };


    class DesignerMessageIssuerHandler : public SystemMessageIssuer
    {
    public:
        DesignerMessageIssuerHandler(DesignerCompilerMessageProcessor* designer_compiler_message_processor)
            :   SystemMessageIssuer(std::make_shared<DesignerVariableArgumentsMessageParameterEvaluator>(designer_compiler_message_processor)),
                m_designerCompilerMessageProcessor(designer_compiler_message_processor)
        {
        }

        void OnIssue(MessageType message_type, int message_number, const std::wstring& message_text) override
        {
            if( m_designerCompilerMessageProcessor != nullptr )
            {
                Logic::ParserMessage parser_message = m_designerCompilerMessageProcessor->GetEngineDriver()->m_pEngineCompFunc->CreateParserMessageFromIssaError(message_type, message_number, message_text);
                OnIssue(parser_message);
            }
        }

        void OnIssue(const Logic::ParserMessage& parser_message) override
        {
            if( m_designerCompilerMessageProcessor == nullptr )
                return;

            // conditionally issue deprecation warnings
            if( parser_message.IsDeprecationWarning() )
            {
                switch( CodeMenu::DeprecationWarnings::GetLevel() )
                {
                    case CodeMenu::DeprecationWarnings::Level::None:
                    {
                        return;
                    }

                    case CodeMenu::DeprecationWarnings::Level::Most:
                    {
                        if( parser_message.type == Logic::ParserMessage::Type::DeprecationMinor )
                            return;
                    }
                }
            }

            m_designerCompilerMessageProcessor->AddParserMessage(parser_message);
        }

    private:
        DesignerCompilerMessageProcessor* m_designerCompilerMessageProcessor;
    };
}


DesignerApplicationLoader::DesignerApplicationLoader(Application* application, DesignerCompilerMessageProcessor* designer_compiler_message_processor)
    :   FileApplicationLoader(application),
        m_designerCompilerMessageProcessor(designer_compiler_message_processor)
{
}


void DesignerApplicationLoader::ResetCachedObjects()
{
    CachedObjects::user_message_manager.reset();
}


Application* DesignerApplicationLoader::GetApplication() 
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<CDataDict> DesignerApplicationLoader::GetDictionary(NullTerminatedString dictionary_filename)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<CDEFormFile> DesignerApplicationLoader::GetFormFile(const CString& form_filename)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<SystemMessageIssuer> DesignerApplicationLoader::GetSystemMessageIssuer()
{
    return std::make_shared<DesignerMessageIssuerHandler>(m_designerCompilerMessageProcessor);
}


std::shared_ptr<MessageManager> DesignerApplicationLoader::GetSystemMessages()
{
    // any custom runtime message files don't matter much while in the designer
    // so always use the default system messages file
    return std::make_shared<MessageManager>(SystemMessages::GetSharedMessageFile());
}


std::shared_ptr<MessageManager> DesignerApplicationLoader::GetUserMessages()
{
    // only load the user messages if they have been modified from the previous compilation
    const size_t cache_key = CachedObjects::GetCacheKey(m_application->GetMessageTextSources());

    if( !CachedObjects::user_message_manager.has_value() || std::get<0>(*CachedObjects::user_message_manager) != cache_key )
        CachedObjects::user_message_manager.emplace(cache_key, FileApplicationLoader::GetUserMessages());

    return std::get<1>(*CachedObjects::user_message_manager);
}
