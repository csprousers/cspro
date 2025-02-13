#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/LogicByteCode.h>
#include <zEngineO/RuntimeEvent.h>
#include <zLogicO/SymbolTable.h>

class Application;


struct ZENGINEO_API EngineData
{
    // --------------------------------------------------------------------------
    // data 
    // --------------------------------------------------------------------------

    std::shared_ptr<EngineAccessor>                 engine_accessor;            // a way to access engine methods and objects not currently in zEngineO

    Application*                                    application;                // the application controlling this engine data (if applicable)

    RuntimeEventsProcessor                          runtime_events_processor;   // runtime events processor

    LogicByteCode                                   logic_byte_code;            // logic byte code

    std::vector<double>                             numeric_constants;          // numeric constants
    std::vector<std::wstring>                       string_literals;            // string literals

    std::vector<std::shared_ptr<Frequency>>         frequencies;                // frequencies
    std::vector<std::shared_ptr<Imputation>>        imputations;                // imputations

    Logic::SymbolTable                              symbol_table;               // the symbol table (which stores all symbols as shared pointers)

                                                                                // tables with copies of some symbols:
    std::vector<LogicArray*>                        arrays;                     // arrays
    std::vector<EngineDictionary*>                  engine_dictionaries;        // dictionaries
    std::vector<LogicFile*>                         files_global_visibility;    // files (with global visibility)
    std::vector<Flow*>                              flows;                      // flows
    std::vector<FLOW*>                              flows_pre80;                // flows
    std::vector<ValueSet*>                          value_sets_not_dynamic;     // value sets (dictionary-based)

                                                                                // container tables with copies of some symbols:
    std::vector<CTAB*>                              crosstabs;                  // crosstabs
    std::vector<DICT*>                              dictionaries_pre80;         // dictionaries
    std::vector<GROUPT*>                            groups;                     // groups
    std::vector<SECT*>                              sections;                   // sections
    std::vector<VART*>                              variables;                  // variables


    // --------------------------------------------------------------------------
    // methods
    // --------------------------------------------------------------------------

    explicit EngineData(std::shared_ptr<EngineAccessor> engine_accessor_);
    ~EngineData();

    // adds the symbol to the symbol table and potentially to tables with copies
    int AddSymbol(std::shared_ptr<Symbol> symbol, Logic::SymbolTable::NameMapAddition name_map_addition = Logic::SymbolTable::NameMapAddition::ToCurrentScope);

    // clears all numeric constants, string literals, frequencies, imputations, and symbols
    void Clear();
};



// --------------------------------------------------------------------------
// access helpers
// --------------------------------------------------------------------------

#define GetNumericConstant(i)           ( m_engineData->numeric_constants[i] )
#define GetStringLiteral(i)             ( m_engineData->string_literals[i] )

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
