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


// --------------------------------------------------------------------------
// cached compilation handling
// --------------------------------------------------------------------------

namespace CachedObjects
{
    std::optional<std::tuple<size_t, std::shared_ptr<MessageManager>>> user_message_manager;

    void AddToCacheKey(size_t& cache_key, const TextSource& text_source);
    void AddToCacheKey(size_t& cache_key, const std::vector<AppMessageFile>& app_message_files);
}


void CachedObjects::AddToCacheKey(size_t& cache_key, const TextSource& text_source)
{
    Hash::Combine(cache_key, &text_source);
    Hash::Combine(cache_key, text_source.GetModifiedIteration());
}


void CachedObjects::AddToCacheKey(size_t& cache_key, const std::vector<AppMessageFile>& app_message_files)
{
    for( const AppMessageFile& app_message_file : app_message_files )
        AddToCacheKey(cache_key, app_message_file.GetTextSource());
}



// --------------------------------------------------------------------------
// compilation error handling
// --------------------------------------------------------------------------

class DesignerVariableArgumentsMessageParameterEvaluator : public VariableArgumentsMessageParameterEvaluator
{
public:
    DesignerVariableArgumentsMessageParameterEvaluator(const DesignerCompilerMessageProcessor* designer_compiler_message_processor);

    SharableString GetProc() override;

private:
    const DesignerCompilerMessageProcessor* m_designerCompilerMessageProcessor;
};


DesignerVariableArgumentsMessageParameterEvaluator::DesignerVariableArgumentsMessageParameterEvaluator(const DesignerCompilerMessageProcessor* const designer_compiler_message_processor)
    :   m_designerCompilerMessageProcessor(designer_compiler_message_processor)
{
}


SharableString DesignerVariableArgumentsMessageParameterEvaluator::GetProc()
{
    return ( m_designerCompilerMessageProcessor != nullptr ) ? m_designerCompilerMessageProcessor->GetProcName() :
                                                               ReturnProgrammingError(SharableString());
}



class DesignerMessageIssuerHandler : public SystemMessageIssuer
{
public:
    DesignerMessageIssuerHandler(DesignerCompilerMessageProcessor* designer_compiler_message_processor);

    void OnIssue(MessageType message_type, int message_number, const std::string& message_text) override;
    void OnIssue(const Logic::ParserMessage& parser_message) override;
    void OnAbort(const std::string& message_text) override;

private:
    DesignerCompilerMessageProcessor* m_designerCompilerMessageProcessor;
};


DesignerMessageIssuerHandler::DesignerMessageIssuerHandler(DesignerCompilerMessageProcessor* const designer_compiler_message_processor)
    :   SystemMessageIssuer(std::make_unique<DesignerVariableArgumentsMessageParameterEvaluator>(designer_compiler_message_processor)),
        m_designerCompilerMessageProcessor(designer_compiler_message_processor)
{
}


void DesignerMessageIssuerHandler::OnIssue(const MessageType message_type, const int message_number, const std::string& message_text)
{
    if( m_designerCompilerMessageProcessor != nullptr )
    {
        const Logic::ParserMessage parser_message = m_designerCompilerMessageProcessor->GetEngineDriver()->m_pEngineCompFunc->CreateParserMessageFromIssaError(message_type, message_number, message_text);
        OnIssue(parser_message);
    }
}


void DesignerMessageIssuerHandler::OnIssue(const Logic::ParserMessage& parser_message)
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


void DesignerMessageIssuerHandler::OnAbort(const std::string& /*message_text*/)
{
    ASSERT(false);
}



// --------------------------------------------------------------------------
// DesignerApplicationLoader
// --------------------------------------------------------------------------

DesignerApplicationLoader::DesignerApplicationLoader(Application* const application, DesignerCompilerMessageProcessor* const designer_compiler_message_processor)
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
    return m_application;
}


std::shared_ptr<CDataDict> DesignerApplicationLoader::GetDictionary(const std::string& dictionary_file_path)
{
    return ReturnProgrammingError(FileApplicationLoader::GetDictionary(dictionary_file_path)); // APP_LOAD_TODO
}


std::shared_ptr<CDEFormFile> DesignerApplicationLoader::GetFormFile(const std::string& form_file_path)
{
    return ReturnProgrammingError(FileApplicationLoader::GetFormFile(form_file_path)); // APP_LOAD_TODO
}


std::shared_ptr<CTabSet> DesignerApplicationLoader::GetTableSpec(const std::string& /*table_spec_file_path*/)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<SystemMessageIssuer> DesignerApplicationLoader::GetSystemMessageIssuer()
{
    return std::make_unique<DesignerMessageIssuerHandler>(m_designerCompilerMessageProcessor);
}


std::shared_ptr<MessageManager> DesignerApplicationLoader::GetSystemMessages()
{
    // any custom runtime message files don't matter much while in the designer
    // so always use the default system messages file
    return std::make_unique<MessageManager>(SystemMessages::GetSharedMessageFile());
}


std::shared_ptr<MessageManager> DesignerApplicationLoader::GetUserMessages()
{
    // only load the user messages if they have been modified from the previous compilation
    size_t cache_key = 0;
    CachedObjects::AddToCacheKey(cache_key, m_application->GetMessageFiles());

    if( !CachedObjects::user_message_manager.has_value() || std::get<0>(*CachedObjects::user_message_manager) != cache_key )
        CachedObjects::user_message_manager.emplace(cache_key, FileApplicationLoader::GetUserMessages());

    return std::get<1>(*CachedObjects::user_message_manager);
}
