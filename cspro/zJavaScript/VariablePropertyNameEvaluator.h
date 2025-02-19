#pragma once

#include <zJavaScript/Atom.h>
#include <zJavaScript/ValueInternal.h>

namespace JavaScript { class VariablePropertyNameEvaluator; }


// --------------------------------------------------------------------------
// JavaScript::VariablePropertyNameEvaluator
//
// This class evaluates a name, attempting to bind it to a variable in the
// global object, and then further evaluating any properties specified in
// the name.
//
// JS_GetProperty is used to search for variable names (e.g., "myFunction"),
// or property names that are defined with dot notation.
//
// JS_Eval is used to evaluate property names specified using bracket
// notation.
//
// This combination of approaches allows for binding to names such as:
//     - myFunction
//     - globalThis.myFunction
//     - myObject.myProperty
//     - myObject['my' + 'Property']
//
// JavaScript::Exception exceptions are thrown on binding or evaluation
// errors, or if the variable binds to an undefined variable (when using
// ForGetting). If using ForPutting, the final evaluated variable or property
// can be undefined, but any class that the property belongs to must be
// defined. For example: "myUndefinedVariable" is valid, but
// "myUndefinedVariable.someProperty" is not.
// --------------------------------------------------------------------------

class JavaScript::VariablePropertyNameEvaluator
{
protected:
    // The constructor parses the name, throwing exceptions on error.
    VariablePropertyNameEvaluator(QuickJSAccess& qjs, bool for_getting, const std::string& name);

public:
    struct ForGetting;
    struct ForSetting;

    // Returns the value associated with the evaluated name.
    const Value& GetValue() const { return m_value; }
    Value ReleaseValue()          { return std::move(m_value); }

    // Returns the value representing the object associated with the evaluated name.
    // This value is either the global object, or an evaluated, defined, object.
    const Value& GetObjectValue() const    { return m_objectValue; }
    bool IsObjectValueGlobalObject() const { return m_isObjectValueGlobalObject; }

    // Returns the atom representing the name of the current value.
    JSAtom GetLastParsedNameAtom() const { ASSERT(m_lastParsedNameAtom.has_value());
                                           return m_lastParsedNameAtom->GetAtom(); }

    // Throws an exception indicating that the name is not defined.
    [[noreturn]] void ThrowException() const;

private:
    // Indicates if the character is the valid start of a JavaScript variable name.
    static bool IsValidVariableStart(char ch);

    // Skips past whitespace, returning the value of the first non-whitespace character.
    char SkipPastWhitespace();

    // The parsing routine.
    void Parse();

    // Reads the next entity, leaving m_nameItr past the end of the entity, which will be one of: '\0', '.', '['.
    void ReadNextEntity();

    // Reads the next entity, leaving m_nameItr at the end of the entity, which will be ']'.
    void ReadNextEntityUsingBracketNotation();

    // Skips past a string literal, leaving m_nameItr at the end of the string literal.
    void SkipPastStringLiteral();

    // Skips past a template literal's embedded expression, leaving m_nameItr at the end of the expression.
    void SkipPastTemplateLiteralExpression();

private:
    QuickJSAccess& m_qjs;
    bool m_forGetting;
    const std::string& m_name;
    const char* m_nameItr;
    Value m_value;
    Value m_objectValue;
    bool m_isObjectValueGlobalObject;
    std::optional<Atom> m_lastParsedNameAtom;
};


struct JavaScript::VariablePropertyNameEvaluator::ForGetting : public VariablePropertyNameEvaluator
{
    ForGetting(QuickJSAccess& qjs, const std::string& name) : VariablePropertyNameEvaluator(qjs, true, name) { }
};


struct JavaScript::VariablePropertyNameEvaluator::ForSetting : public VariablePropertyNameEvaluator
{
    ForSetting(QuickJSAccess& qjs, const std::string& name) : VariablePropertyNameEvaluator(qjs, false, name) { }
};
