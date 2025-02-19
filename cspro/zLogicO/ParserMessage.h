#pragma once

#include <zToolsO/CSProException.h>
#include <zCapiO/CapiLogicParameters.h>

namespace Logic { struct ParserError; struct ParserMessage; }


struct Logic::ParserMessage : public CSProException
{
    enum class Type { Error, Warning, DeprecationMajor, DeprecationMinor };

    struct LineNumberOverride { size_t line_number; };
    struct MessageFile { };
    using ExtendedLocation = std::variant<std::monostate, CapiLogicLocation, LineNumberOverride, MessageFile>;

    ParserMessage(Type _type)
        :   CSProException("Logic - Parser Message"),
            type(_type),
            message_number(INT_MAX),
            line_number(0),
            position_in_line(0)
    {
    }

    virtual ~ParserMessage() { }

    bool IsDeprecationWarning() const { return ( type == Type::DeprecationMajor || type == Type::DeprecationMinor ); }

    Type type;
    int message_number;
    std::string message_text;
    size_t line_number;
    size_t position_in_line;
    std::string compilation_unit_name;
    std::string proc_name;
    ExtendedLocation extended_location;
};



struct Logic::ParserError : public ParserMessage
{
    ParserError()
        :   ParserMessage(ParserMessage::Type::Error)
    {
    }
};
