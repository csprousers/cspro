#include "stdafx.h"
#include "Atom.h"
#include "VariablePropertyNameEvaluator.h"


JavaScript::VariablePropertyNameEvaluator::VariablePropertyNameEvaluator(QuickJSAccess& qjs, const bool for_getting, const std::string& name)
    :   m_qjs(qjs),
        m_forGetting(for_getting),
        m_name(name),
        m_nameItr(m_name.c_str()),
        m_objectValue(GlobalObjectValue(&m_qjs)),
        m_isObjectValueGlobalObject(true)
{
    Parse();
}


void JavaScript::VariablePropertyNameEvaluator::ThrowException() const
{
    throw Exception(FormatText("'%s' is not a defined %s.",
                               m_name.c_str(),
                               m_isObjectValueGlobalObject ?  "variable" : "property"));
}


bool JavaScript::VariablePropertyNameEvaluator::IsValidVariableStart(const char ch)
{
    return ( std::isalpha(ch) ||
             ch == '_' ||
             ch == '$' );
}


char JavaScript::VariablePropertyNameEvaluator::SkipPastWhitespace()
{
    char ch;

    while( std::isspace(( ch = *m_nameItr )) )
        ++m_nameItr;

    return ch;
}


void JavaScript::VariablePropertyNameEvaluator::Parse()
{
    while( true )
    {
        // determine if this character is a valid start
        char ch = SkipPastWhitespace();
        const bool entity_uses_bracket_notation = ( ch == '[' );

        if( !entity_uses_bracket_notation && !IsValidVariableStart(ch) )
            ThrowException();

        const char* const current_entity_start_pos = m_nameItr;

        // is using bracket notation, use JS_Eval to evaluate the expression within the brackets, which becomes the entity name
        if( entity_uses_bracket_notation )
        {
            // bracket notation can't be used until a variable has been provided
            if( !m_lastParsedNameAtom.has_value() )
                ThrowException();

            ReadNextEntityUsingBracketNotation();
            ASSERT(*current_entity_start_pos == '[' && *m_nameItr == ']');
            const std::string bracket_text(SO::Trim(std::string_view(current_entity_start_pos + 1, m_nameItr - current_entity_start_pos - 1)));

            if( bracket_text.empty() )
                ThrowException();

            const Value js_bracket_value(&m_qjs, JS_Eval(m_qjs.ctx, bracket_text.c_str(), bracket_text.length(), "", JS_EVAL_TYPE_GLOBAL));

            if( JS_IsException(*js_bracket_value) )
                m_qjs.ThrowException();

            m_lastParsedNameAtom = Atom(m_qjs, JS_ValueToAtom(m_qjs.ctx, *js_bracket_value));

            ++m_nameItr;
        }

        // otherwise the entity name will terminate at the end of the string, or at a dot
        else
        {
            ReadNextEntity();
            const std::string_view variable_text_sv = SO::Trim(std::string_view(current_entity_start_pos, m_nameItr - current_entity_start_pos));

            // variable names cannot contain spaces
            if( variable_text_sv.empty() || SO::FindFirstWhitespace(variable_text_sv) != std::string_view::npos )
                ThrowException();

            m_lastParsedNameAtom = Atom(m_qjs, variable_text_sv);
        }

        // look up the variable or property
        ASSERT(m_lastParsedNameAtom.has_value());
        Value js_entity(&m_qjs, JS_GetProperty(m_qjs.ctx,
                                               m_objectValue.GetValue(),
                                               m_lastParsedNameAtom->GetAtom()));

        if( JS_IsException(*js_entity) )
            m_qjs.ThrowException();

        // if the entity is the variable, it cannot be undefined
        if( m_forGetting && m_isObjectValueGlobalObject && JS_IsUndefined(*js_entity) )
            ThrowException();

        ch = SkipPastWhitespace();

        // our parsing is complete if at the end of the string...
        if( ch == '\0' )
        {
            m_value = std::move(js_entity);
            return;
        }

        // ...otherwise we must process more entities

        // make the entity the object (as long as the first entity is not "globalThis")
        if( !m_isObjectValueGlobalObject || m_objectValue != js_entity )
        {
            m_objectValue = std::move(js_entity);
            m_isObjectValueGlobalObject = false;
        }

        if( ch == '.' )
        {
            ++m_nameItr;
        }

        else if( ch != '[' )
        {
            ThrowException();
        }
    }
}


void JavaScript::VariablePropertyNameEvaluator::ReadNextEntity()
{
    ASSERT(IsValidVariableStart(*m_nameItr));

    while( true )
    {
        ++m_nameItr;
        const char ch = *m_nameItr;

        if( ch == '\0' || ch == '.' || ch == '[' )
            return;
    }
}


void JavaScript::VariablePropertyNameEvaluator::ReadNextEntityUsingBracketNotation()
{
    ASSERT(*m_nameItr == '[');

    // read until the closing brace
    while( true )
    {
        ++m_nameItr;
        const char ch = *m_nameItr;

        if( ch == ']' )
        {
            return;
        }

        else if( ch == '\'' || ch == '"' || ch == '`' )
        {
            SkipPastStringLiteral();
            ASSERT(ch == *m_nameItr);
        }

        else if( ch == '\0' )
        {
            ThrowException();
        }
    }
}


void JavaScript::VariablePropertyNameEvaluator::SkipPastStringLiteral()
{
    const char quotemark = *m_nameItr;
    const bool is_template = ( quotemark == '`' );
    ASSERT(is_template || quotemark == '\'' || quotemark == '"');

    ++m_nameItr;

    while( true )
    {
        char ch = *m_nameItr;

        // return on the closing quote
        if( ch == quotemark )
        {
            return;
        }

        // skip past escaped characters
        else if( ch == '\\' )
        {
            if( *(m_nameItr++) == '\0' )
                ThrowException();
        }

        // handle template literal embedded expressions
        else if( is_template && ch == '$' && m_nameItr[1] == '{' )
        {
            SkipPastTemplateLiteralExpression();
            ASSERT(*m_nameItr == '}');
        }

        else if( ch == '\0' )
        {
            ThrowException();
        }

        ++m_nameItr;
    }
}


void JavaScript::VariablePropertyNameEvaluator::SkipPastTemplateLiteralExpression()
{
    ASSERT(m_nameItr[0] == '$' && m_nameItr[1] == '{');
    m_nameItr += 2;

    int brace_count = 1;

    while( true )
    {
        const char ch = *m_nameItr;

        if( ch == '{' )
        {
            ++brace_count;
        }

        else if( ch == '}' )
        {
            if( --brace_count == 0 )
                return;
        }

        // skip past escaped characters
        else if( ch== '\\' )
        {
            if( *(m_nameItr++) == '\0' )
                ThrowException();
        }

        else if( ch == '\0' )
        {
            ThrowException();
        }

        ++m_nameItr;
    }
}
