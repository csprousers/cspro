#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/TableDefinitions.h>

class Application;
struct sqlite3;
namespace Paradata { class Event; class Log; class Logger; enum class PortableMessage; class Syncer; }

enum class Paradata::PortableMessage
{
    StopLogger,
    StartLogger,
    QueryCachedEvents,
    UpdateBackgroundCollectionParameters
};



// this is the publicly accessible logger
class ZPARADATAO_API Paradata::Logger
{
public:
    Logger();
    ~Logger();

    static void SendPortableMessage(PortableMessage message, const Application* application = nullptr);

    static bool IsOpen() { return ( _logger.m_log != nullptr ); }

    static const std::string& GetFilePath();

    static bool Start(std::string file_path, const Application* application = nullptr);

    static bool Flush();

    static void Stop();

    static void UpdateBackgroundCollectionParameters(const Application* application);

    static sqlite3* GetSqlite();

    static void LogEvent(std::shared_ptr<Event> event, const void* instance_object = nullptr);

    static std::unique_ptr<Syncer> GetSyncer();

private:
    std::unique_ptr<Log> m_log;
    std::string m_filePath;
    std::vector<bool> m_includedEvents;

    // the logger singleton
    static Logger _logger;
};



// include the event header files (so only this file has to be included from other parts of the code)
#include <zParadataO/EventList.h>
