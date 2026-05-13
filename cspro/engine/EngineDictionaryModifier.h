#pragma once

class CIntDriver;
class CSymbolDict;
class EngineDictionary;


// --------------------------------------------------------------------------
// EngineDictionaryModifier
//
// When the data repository connected to one of an application's dictionaries
// is modified, there may be some actions to take pre- or post- modification.
//
// This will be used to access the interpreter from projects that may not
// depend on the engine, which is why the entry points are all virtual.
// --------------------------------------------------------------------------

class EngineDictionaryModifier
{
public:
    virtual ~EngineDictionaryModifier() { }

    static std::unique_ptr<EngineDictionaryModifier> Create(CIntDriver& interpreter, CSymbolDict& dict);
    static std::unique_ptr<EngineDictionaryModifier> Create(CIntDriver& interpreter, EngineDictionary& engine_dictionary);

    // Runs any routines necessary before modification begins:
    //   - Stores information about the current case being modified or inserted (from WriteCaseParameter).
    //   - Loads any binary data associated with the current case.
    //   - Resets any case iterators in use (e.g., by forcase).
    virtual void PrepareForModifications() = 0;

    // Runs any routines necessary once modifications are over:
    //   - Ensures that WriteCaseParameter is valid based on the modifications.
    virtual void FinishedWithModifications() = 0;
};
