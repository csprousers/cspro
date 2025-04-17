#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/EngineSettings.h>
#include <zEngineO/LogicByteCode.h>
#include <zEngineO/RuntimeEvent.h>
#include <zEngineO/SymbolReference.h>
#include <zToolsO/PointerClasses.h>
#include <zLogicO/SymbolTable.h>

class Application;
class CommonStore;
class EngineJavaScriptProcessor;
class PFF;


struct ZENGINEO_API EngineData
{
    // --------------------------------------------------------------------------
    // data
    // --------------------------------------------------------------------------

    // a way to access engine methods and objects not currently in zEngineO
    std::shared_ptr<EngineAccessor> engine_accessor;

    // engine settings
    EngineSettings engine_settings;

    // the CommonStore
    std::shared_ptr<CommonStore> common_store;

    // the application controlling this engine data (if applicable)
    Application* application;

    // the PFF controlling this application (if applicable)
    cs::shared_or_raw_ptr<PFF> pff;

    // runtime events processor
    RuntimeEventsProcessor runtime_events_processor;

    // logic bytecode
    LogicByteCode logic_byte_code;

    // JavaScript processor
    std::unique_ptr<EngineJavaScriptProcessor> javascript_processor;

    // the version of logic that created the .pen file
    int compiled_logic_version;

    // numeric constants + string literals
    std::vector<double> numeric_constants;
    std::vector<SharableString> string_literals;

    // frequencies + imputations
    std::vector<std::shared_ptr<Frequency>> frequencies;
    std::vector<std::shared_ptr<Imputation>> imputations;

    // evaluated symbol references (that will be destructed prior to the symbol table
    // so that any symbols holding references to themselves will be properly destructed)
    std::vector<std::shared_ptr<std::unique_ptr<SymbolReference<std::shared_ptr<Symbol>>>>> evaluated_symbol_references;

    // the symbol table (which stores all symbols as shared pointers)
    Logic::SymbolTable symbol_table;

    // tables with copies of some symbols:
    std::vector<LogicArray*> arrays;                    // arrays
    std::vector<EngineDictionary*> engine_dictionaries; // dictionaries
    std::vector<LogicFile*> files_global_visibility;    // files (with global visibility)
    std::vector<Flow*> flows;                           // flows
    std::vector<FLOW*> flows_pre80;                     // flows
    std::vector<ValueSet*> value_sets_not_dynamic;      // value sets (dictionary-based)

    // container tables with copies of some symbols:
    std::vector<CTAB*> crosstabs;           // crosstabs
    std::vector<DICT*> dictionaries_pre80;  // dictionaries
    std::vector<GROUPT*> groups;            // groups
    std::vector<SECT*> sections;            // sections
    std::vector<VART*> variables;           // variables


    // --------------------------------------------------------------------------
    // methods
    // --------------------------------------------------------------------------

    explicit EngineData(std::shared_ptr<EngineAccessor> engine_accessor_);
    ~EngineData();

    // Adds the symbol to the symbol table and potentially to tables with copies.
    int AddSymbol(std::shared_ptr<Symbol> symbol, Logic::SymbolTable::NameMapAddition name_map_addition = Logic::SymbolTable::NameMapAddition::ToCurrentScope);

    // Clears all numeric constants, string literals, frequencies, imputations, evaluated symbol references, and symbols.
    void Clear();

    // Returns the CommonStore, opening it if necessary. Null is returned on error.
    const std::shared_ptr<CommonStore>& GetCommonStore();

    // Returns the engine's JavaScript processor, instantiating it if necessary.
    EngineJavaScriptProcessor& GetJavaScriptProcessor();

    // Helpers for evaluating bytecode based on the version in .pen file version.
    bool MeetsCompiledLogicVersion(int version) const    { return ( compiled_logic_version >= version ); }
    bool PredatesCompiledLogicVersion(int version) const { return ( compiled_logic_version < version ); }
};



// --------------------------------------------------------------------------
// access helpers
// --------------------------------------------------------------------------

#define GetNumericConstant(i)           ( m_engineData->numeric_constants[i] )

#define NPT_Ref(i)                      ( GetSymbolTable().GetAt(i) )

#define GetSymbolEngineDictionary(i)    assert_cast<EngineDictionary&>(NPT_Ref(i))
#define GetSymbolEngineRecord(i)        assert_cast<EngineRecord&>(NPT_Ref(i))
#define GetSymbolEngineItem(i)          assert_cast<EngineItem&>(NPT_Ref(i))

#define GetSymbolEngineBlock(i)         assert_cast<EngineBlock&>(NPT_Ref(i))
#define GetSymbolFlow(i)                assert_cast<Flow&>(NPT_Ref(i))

#define GetSymbolLogicArray(i)          assert_cast<LogicArray&>(NPT_Ref(i))
#define GetSymbolLogicAudio(i)          assert_cast<LogicAudio&>(NPT_Ref(i))
#define GetSymbolLogicDocument(i)       assert_cast<LogicDocument&>(NPT_Ref(i))
#define GetSymbolLogicFile(i)           assert_cast<LogicFile&>(NPT_Ref(i))
#define GetSymbolLogicGeometry(i)       assert_cast<LogicGeometry&>(NPT_Ref(i))
#define GetSymbolLogicHashMap(i)        assert_cast<LogicHashMap&>(NPT_Ref(i))
#define GetSymbolLogicImage(i)          assert_cast<LogicImage&>(NPT_Ref(i))
#define GetSymbolLogicList(i)           assert_cast<LogicList&>(NPT_Ref(i))
#define GetSymbolLogicMap(i)            assert_cast<LogicMap&>(NPT_Ref(i))
#define GetSymbolLogicNamedFrequency(i) assert_cast<NamedFrequency&>(NPT_Ref(i))
#define GetSymbolLogicPff(i)            assert_cast<LogicPff&>(NPT_Ref(i))
#define GetSymbolReport(i)              assert_cast<Report&>(NPT_Ref(i))
#define GetSymbolStringWriter(i)        assert_cast<StringWriter&>(NPT_Ref(i))
#define GetSymbolSystemApp(i)           assert_cast<SystemApp&>(NPT_Ref(i))
#define GetSymbolUserFunction(i)        assert_cast<UserFunction&>(NPT_Ref(i))
#define GetSymbolValueSet(i)            assert_cast<ValueSet&>(NPT_Ref(i))
#define GetSymbolWorkString(i)          assert_cast<WorkString&>(NPT_Ref(i))
#define GetSymbolWorkVariable(i)        assert_cast<WorkVariable&>(NPT_Ref(i))

#define GetSharedSymbol(i)              ( GetSymbolTable().GetSharedAt(i) )

#define LPT(i)                          assert_cast<FLOW*>(&NPT_Ref(i))
#define DPT(i)                          assert_cast<DICT*>(&NPT_Ref(i))
#define SPT(i)                          assert_cast<SECT*>(&NPT_Ref(i))
#define FPT(i)                          assert_cast<FORM*>(&NPT_Ref(i))
#define VPT(i)                          assert_cast<VART*>(&NPT_Ref(i))
#define GPT_Positive(i)                 assert_cast<GROUPT*>(&NPT_Ref(i))
#define GPT(i)                          ( ( i > 0 ) ? GPT_Positive(i) : ( i < 0 ) ? GIP(-1 * i) : nullptr )
#define XPT(i)                          assert_cast<CTAB*>(&NPT_Ref(i))
#define RLT(i)                          assert_cast<RELT*>(&NPT_Ref(i))
