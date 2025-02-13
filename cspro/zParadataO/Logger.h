#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/TableDefinitions.h>

struct sqlite3;
class Application;


namespace Paradata
{
    enum class PortableMessage
    {
        StopLogger,
        StartLogger,
        QueryCachedEvents,
        UpdateBackgroundCollectionParameters
    };

    class Log;
    class Event;
    class Syncer;


    // this is the publicly accessible logger
    class ZPARADATAO_API Logger
    {
    public:
        Logger();
        ~Logger();

        static void SendPortableMessage(PortableMessage message, const Application* application = nullptr);

        static bool IsOpen() { return ( _logger.m_log != nullptr ); }

        static const std::wstring& GetFilename();

        static bool Start(std::wstring filename, const Application* application = nullptr);

        static bool Flush();

        static void Stop();

        static void UpdateBackgroundCollectionParameters(const Application* application);

        static sqlite3* GetSqlite();

        static void LogEvent(std::shared_ptr<Event> event, const void* instance_object = nullptr);

        static std::unique_ptr<Syncer> GetSyncer();

    private:
        std::unique_ptr<Log> m_log;
        std::wstring m_filename;
        std::vector<bool> m_includedEvents;

        // the logger singleton
        static Logger _logger;
    };
}


// include the event header files (so only this file has to be included from other parts of the code)
#include <zParadataO/EventList.h>
