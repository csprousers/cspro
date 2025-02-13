#include "stdafx.h"
#include "Logger.h"
#include "Syncer.h"
#include <zUtilO/Interapp.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>

using namespace Paradata;


Logger Logger::_logger; // the logger used throughout the application


Logger::Logger()
{
}


Logger::~Logger()
{
    Stop();
}


void Logger::SendPortableMessage(PortableMessage message, const Application* application/* = nullptr*/)
{
#ifdef WIN_DESKTOP
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(application);
#else
    if( IsOpen() )
        PlatformInterface::GetInstance()->GetApplicationInterface()->ParadataDriverManager(message, application);
#endif
}


const std::wstring& Logger::GetFilename()
{
    return IsOpen() ? _logger.m_filename :
                      SO::EmptyString;
}


bool Logger::Start(std::wstring filename, const Application* application/* = nullptr*/)
{
    if( !IsOpen() && !filename.empty() )
    {
        try
        {
            _logger.m_log = std::make_unique<Log>(filename);
            _logger.m_filename = std::move(filename);
            _logger.m_includedEvents.resize(ParadataTable_NumberTables, true);
        }

        catch( const Exception& exception ) // ignore errors
        {
            if constexpr(DebugMode())
            {
                ErrorMessage::Display(exception);
            }

            ASSERT(_logger.m_log == nullptr);
            return false;
        }

        if( application != nullptr )
        {
            const ParadataProperties& paradata_properties = application->GetApplicationProperties().GetParadataProperties();

            if( paradata_properties.GetCollectionType() == ParadataProperties::CollectionType::SomeEvents )
            {
                std::fill(_logger.m_includedEvents.begin(), _logger.m_includedEvents.end(), false);

                // application, session, and case events are always used
                _logger.m_includedEvents[static_cast<size_t>(ParadataTable::ApplicationEvent)] = true;
                _logger.m_includedEvents[static_cast<size_t>(ParadataTable::SessionEvent)] = true;
                _logger.m_includedEvents[static_cast<size_t>(ParadataTable::CaseEvent)] = true;

                for( size_t i = 0; i < ParadataTable_NumberTables; ++i )
                {
                    const TableDefinition& table_definition = GetTableDefinition(static_cast<ParadataTable>(i));

                    if( paradata_properties.IncludeEvent(table_definition.name) )
                        _logger.m_includedEvents[static_cast<size_t>(table_definition.type)] = true;
                }
            }
        }

        SendPortableMessage(PortableMessage::StartLogger);

        if( application != nullptr )
            UpdateBackgroundCollectionParameters(application);
    }

    return true;
}


bool Logger::Flush()
{
    bool success = false;

    if( IsOpen() )
    {
        try
        {
            _logger.m_log->WriteEvents(false);
            success = true;
        }

        catch( const Exception& exception )
        {
#if defined(_DEBUG) && defined(WIN_DESKTOP)
            ErrorMessage::Display(exception);
#endif
        }
    }

    return success;
}


void Logger::Stop()
{
    if( IsOpen() )
        SendPortableMessage(PortableMessage::StopLogger);

    _logger.m_log.reset();
}


void Logger::UpdateBackgroundCollectionParameters(const Application* application)
{
    SendPortableMessage(PortableMessage::UpdateBackgroundCollectionParameters, application);
}


sqlite3* Logger::GetSqlite()
{
    return IsOpen() ? _logger.m_log->m_db :
                      nullptr;
}


void Logger::LogEvent(std::shared_ptr<Event> event, const void* instance_object/* = nullptr*/)
{
    if( !IsOpen() || !_logger.m_includedEvents[static_cast<size_t>(event->GetType())] )
        return;

    try
    {
        _logger.m_log->LogEvent(std::move(event), instance_object);
    }

    catch( const Exception& exception ) // ignore errors
    {
        if constexpr(DebugMode())
        {
            ErrorMessage::Display(exception);
        }
    }
}


std::unique_ptr<Syncer> Logger::GetSyncer()
{
    ASSERT(IsOpen());
    return std::make_unique<Syncer>(*_logger.m_log);
}
