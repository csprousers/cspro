#pragma once

#include <zParadataO/NamedObject.h>
#include <zMessageO/MessageType.h>
#include <zLogicO/FunctionTable.h>


namespace Paradata
{
    class Event;
    class MessageEvent;

    // interface for an object that can fill the PROC information for events
    class IParadataDriver
    {
    public:
        virtual ~IParadataDriver() { }

        virtual bool GetRecordIteratorLoadCases() const = 0;

        virtual std::shared_ptr<NamedObject> CreateObject(NamedObject::Type type, wstring_view name) = 0;

        virtual std::unique_ptr<MessageEvent> CreateMessageEvent(std::variant<MessageType, FunctionCode> message_type_or_function_code,
                                                                 int message_number, std::wstring message_text) = 0;

        virtual void RegisterAndLogEvent(std::shared_ptr<Event> event, void* instance_object = nullptr) = 0;
    };
}
