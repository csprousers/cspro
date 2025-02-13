#pragma once

#include <zToolsO/CSProException.h>
#include <zCapiO/CapiLogicParameters.h>


namespace Logic
{
    struct ParserMessage : public CSProException
    {
        enum class Type { Error, Warning, DeprecationMajor, DeprecationMinor };

        struct MessageFile { };
        using ExtendedLocation = std::variant<std::monostate, CapiLogicLocation, MessageFile>;

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
        std::wstring message_text;
        size_t line_number;
        size_t position_in_line;
        std::wstring compilation_unit_name;
        std::wstring proc_name;
        ExtendedLocation extended_location;
    };

    struct ParserError : public ParserMessage
    {
        ParserError()
            :   ParserMessage(ParserMessage::Type::Error)
        {
        }
    };
}
