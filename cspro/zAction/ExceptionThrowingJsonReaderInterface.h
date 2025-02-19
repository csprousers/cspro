#pragma once

#include <zJson/JsonNode.h>

namespace ActionInvoker { class ExceptionThrowingJsonReaderInterface; }


class ActionInvoker::ExceptionThrowingJsonReaderInterface : public JsonReaderInterface
{
public:
    using JsonReaderInterface::JsonReaderInterface;

    void SetDirectory(std::string directory)
    {
        m_directory = std::move(directory);
    }

    void OnReportInvalidAccessUsingKey(const cs::string_sz key, const cs::string_sz node_text, const JsonParseException* const exception_to_be_thrown) override
    {
        if( exception_to_be_thrown == nullptr )
        {
            throw JsonParseException("The value of '%s' is invalid because it contained an invalid entry: %s",
                                     key.c_str(), node_text.c_str());
        }

        else
        {
            throw JsonParseException("The value of '%s' is invalid and resulted in an error ('%s'): %s",
                                     key.c_str(), exception_to_be_thrown->what(), node_text.c_str());
        }
    }
};
