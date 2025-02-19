#pragma once

#include <zJson/zJson.h>


// --------------------------------------------------------------------------
// IncrementalJsonObjectArrayParser
//
// This class allows JSON to be incrementally parsed as an array of objects.
// The first non-whitespace text passed to the class must be the beginning of
// an array.
//
// The overridable method OnObject is called every time an object is parsed.
// Only objects at the root level (in the initially parsed array) are parsed.
//
// Text following the array of objects is passed to the overridable method
// ProcessPostParseText.
//
// JsonParseException exceptions are thrown on parsing errors.
//
// JsonStream::CreateObjectArrayIterator provides another way to iterator
// over an array of objects.
// --------------------------------------------------------------------------

class ZJSON_API IncrementalJsonObjectArrayParser
{
public:
    IncrementalJsonObjectArrayParser();
    virtual ~IncrementalJsonObjectArrayParser() { }

    // Updates the parser with additional text.
    // The OnObject method will be called anytime an object is fully processed.
    void Update(std::string_view json_text_sv);

    // When called, indicates to the parser that all text has been processed.
    // If not in a finalized state, an exception is thrown.
    void Finish() const;

    // Returns true if the array of objects has been fully processed.
    bool IsComplete() const;

protected:
    // Called everytime an object is parsed.
    virtual void OnObject(const JsonNode& json_node) = 0;

    virtual void ProcessPostParseText(std::string_view text_sv);

private:
    enum class State
    {
        ExpectingStartArrayOfObjects,
        ExpectingFirstObjectOrEndArrayOfObjects,
        ExpectingCommaOrEndArrayOfObjects,
        ExpectingNextObject,
        InObject,
        InStringLiteral,
        InStringLiteralEscape,
        ParseComplete
    };

    State m_state;
    size_t m_startObjectsCounter;
    std::string m_objectJsonText;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool IncrementalJsonObjectArrayParser::IsComplete() const
{
    return ( m_state == State::ParseComplete );
}


inline void IncrementalJsonObjectArrayParser::ProcessPostParseText(std::string_view /*text_sv*/)
{
}
