#include "stdafx.h"
#include "IncrementalJsonObjectArrayParser.h"
#include "Json.h"


IncrementalJsonObjectArrayParser::IncrementalJsonObjectArrayParser()
    :   m_state(State::ExpectingStartArrayOfObjects),
        m_startObjectsCounter(0)
{
}


void IncrementalJsonObjectArrayParser::Update(const std::string_view json_text_sv)
{
    // this routine only parses the minimum needed to construct objects from an array
    // of objects, because the JSON will be properly parsed when passed to OnObject
    const char* json_text_itr = json_text_sv.data();
    const char* const json_text_end = json_text_itr + json_text_sv.length();

    for( ; json_text_itr < json_text_end; ++json_text_itr )
    {
        const char ch = *json_text_itr;
        bool add_character = true;

        // if in a string literal, all we have to check for is the string literal ending, or an escape sequence
        if( m_state == State::InStringLiteral )
        {
            if( ch == '"' )
            {
                m_state = State::InObject;
            }

            else if( ch == '\\' )
            {
                m_state = State::InStringLiteralEscape;
            }
        }

        // if in a string literal escape sequence, toggle back to being in a string literal
        else if( m_state == State::InStringLiteralEscape )
        {
            m_state = State::InStringLiteral;
        }

        // if parsing is complete, add the rest of the characters to the post-parse text and get out
        else if( m_state == State::ParseComplete )
        {
            ProcessPostParseText(std::string_view(json_text_itr, json_text_end - json_text_itr));
            return;
        }

        // while parsing and not in a string literal, we can always ignore whitespace
        else if( SO::IsWhitespaceChar(ch) )
        {
            add_character = false;
        }

        // at the beginning of the stream, we require '['
        else if( m_state == State::ExpectingStartArrayOfObjects )
        {
            if( ch != '[' )
                throw JsonParseException("The next element in the JSON stream must be an array");

            add_character = false;
            m_state = State::ExpectingFirstObjectOrEndArrayOfObjects;
        }

        // handle the beginning of objects
        else if( ch == '{' )
        {
            if( m_state == State::ExpectingFirstObjectOrEndArrayOfObjects ||
                m_state == State::ExpectingNextObject )
            {
                ASSERT(m_startObjectsCounter == 0);
                m_state = State::InObject;
            }

            else if( m_state != State::InObject )
            {
                throw JsonParseException("Unxpected '{' in JSON stream");
            }

            else
            {
                ++m_startObjectsCounter;
            }
        }

        // handle the end of objects
        else if( ch == '}' )
        {
            if( m_state != State::InObject )
            {
                throw JsonParseException("Unxpected '}' in JSON stream");
            }

            else if( m_startObjectsCounter > 0 )
            {
                --m_startObjectsCounter;
            }

            else
            {
                // parse the current object
                m_objectJsonText.push_back('}');
                OnObject(Json::Parse(m_objectJsonText));

                // reset for the next object
                add_character = false;
                m_objectJsonText.clear();
                m_state = State::ExpectingCommaOrEndArrayOfObjects;
            }
        }

        // handle the comma separating objects
        else if( ch == ',' && m_state == State::ExpectingCommaOrEndArrayOfObjects )
        {
            add_character = false;
            m_state = State::ExpectingNextObject;
        }

        // handle the end of the array of objects
        else if( ch == ']' && ( m_state == State::ExpectingFirstObjectOrEndArrayOfObjects ||
                                m_state == State::ExpectingCommaOrEndArrayOfObjects ) )
        {
            add_character = false;
            m_state = State::ParseComplete;
        }

        // handle the beginning of a string literal
        else if( ch == '"' && m_state == State::InObject )
        {
            m_state = State::InStringLiteral;
        }

        // at this point, any character must be within the object
        else if( m_state != State::InObject )
        {
            throw JsonParseException("Unxpected '%c' in JSON stream", ch);
        }


        // add the character to the JSON text of the object currently being processed
        if( add_character )
            m_objectJsonText.push_back(ch);
    }
}


void IncrementalJsonObjectArrayParser::Finish() const
{
    if( m_state != State::ParseComplete )
        throw JsonParseException("The JSON stream is incomplete");
}
