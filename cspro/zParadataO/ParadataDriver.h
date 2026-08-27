#pragma once

#include <zParadataO/NamedObject.h>

enum FunctionCode : int;
enum class MessageType;
namespace Paradata { class Event; class MessageEvent; class ParadataDriver; }


// --------------------------------------------------------------------------
// Paradata::ParadataDriver
//
// An interface for an object that can fill the PROC information for events.
// --------------------------------------------------------------------------

class Paradata::ParadataDriver
{
public:
    virtual ~ParadataDriver() { }

    virtual bool GetRecordIteratorLoadCases() const = 0;

    virtual std::shared_ptr<NamedObject> CreateObject(NamedObject::Type type, std::string_view name_sv) = 0;

    virtual std::unique_ptr<MessageEvent> CreateMessageEvent(std::variant<MessageType, FunctionCode> message_type_or_function_code,
                                                             int message_number, SharableString message_text) = 0;

    virtual void RegisterAndLogEvent(std::shared_ptr<Event> event, const void* instance_object = nullptr) = 0;
};
