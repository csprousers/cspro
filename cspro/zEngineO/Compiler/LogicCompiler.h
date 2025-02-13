#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/EngineData.h>
#include <zEngineO/Compiler/SymbolCompilerModifier.h>
#include <zEngineO/Nodes/BaseNodes.h>
#include <zLogicO/BaseCompiler.h>
#include <zLogicO/FunctionTable.h>

class CompilerHelper;
template<typename T> class ConstantConserver;
class DynamicValueSet;
enum class EngineAppType : int;
class LoopStack;
class MessageEvaluator;
class MessageManager;
struct NamedTextSource;
enum class SetAction : int;
namespace CompilationExtendedInformation { struct InCrosstabInformation; }
namespace GF { enum class VariableType: int; }


// --------------------------------------------------------------------------
// LogicCompiler
// --------------------------------------------------------------------------

class ZENGINEO_API LogicCompiler : public Logic::BaseCompiler
{
    friend class ArgumentSpecification;
    friend class OptionalNamedArgumentsCompiler;

public:
    LogicCompiler(std::shared_ptr<EngineData> engine_data);
    ~LogicCompiler();


    // --------------------------------------------------------------------------
    // methods to create compilation nodes
    // --------------------------------------------------------------------------
public:
    int* CreateCompilationSpace(int ints_needed);

    template<typename NodeType>
    NodeType& GetNode(int program_index);

    template<typename NodeType>
    NodeType& CreateNode(std::optional<FunctionCode> function_code = std::nullopt, int node_size_offset = 0);

    template<typename NodeType>
    NodeType& CreateVariableSizeNode(std::optional<FunctionCode> function_code, int number_arguments);

    template<typename NodeType>
    NodeType& CreateVariableSizeNode(int number_arguments);

    template<typename NodeType>
    void InitializeNode(NodeType& compilation_node, int value, int node_start_offset = 0);

    template<typename NodeType>
    int GetProgramIndex(const NodeType& compilation_node);

    template<typename NodeType>
    int GetOptionalProgramIndex(const NodeType* compilation_node);

    int CreateOperatorNode(FunctionCode function_code, int left_expr, int right_expr);

    int CreateListNode(cs::span<const int> arguments);

    int CreateVariableArgumentsNode(FunctionCode function_code, cs::span<const int> arguments);

    int CreateVariableArgumentsWithSizeNode(FunctionCode function_code, cs::span<const int> arguments);

    int CreateSymbolVariableArgumentsNode(FunctionCode function_code, const Symbol& symbol, cs::span<const int> arguments);

    Nodes::SymbolVariableArguments& CreateSymbolVariableArgumentsNode(FunctionCode function_code, const Symbol& symbol,
                                                                      int number_arguments, std::optional<int> initialize_value = std::nullopt);

    int CreateSymbolVariableArgumentsWithSubscriptNode(FunctionCode function_code, const Symbol& symbol, int symbol_subscript_compilation, cs::span<const int> arguments);

    Nodes::SymbolVariableArgumentsWithSubscript& CreateSymbolVariableArgumentsWithSubscriptNode(FunctionCode function_code, const Symbol& symbol, int symbol_subscript_compilation,
                                                                                                int number_arguments, std::optional<int> initialize_value = std::nullopt);

    int WrapNodeAroundScopeChange(const Logic::LocalSymbolStack& local_symbol_stack, int program_index, bool store_local_symbol_names = false);


    // --------------------------------------------------------------------------
    // current compilation information
    // --------------------------------------------------------------------------
public:
    void SetCompilationSymbol(const Symbol& symbol);

    const Symbol& GetCompilationSymbol() const   { return *m_compilationSymbol; }
    SymbolType GetCompilationSymbolType() const  { return m_compilationSymbol->GetType(); }
    int GetCompilationLevelNumber_base1() const;

    bool IsCompiling(const Symbol& symbol) const   { return ( &symbol == m_compilationSymbol ); }
    bool IsCompiling(SymbolType symbol_type) const { return ( symbol_type == GetCompilationSymbolType() ); }
    bool IsGlobalCompilation() const               { return IsCompiling(SymbolType::Application); }
    bool IsNoLevelCompilation() const;

    EngineAppType GetEngineAppType() const;


    // --------------------------------------------------------------------------
    // compiler helpers
    // --------------------------------------------------------------------------
public:
    template<typename T>
    T& GetCompilerHelper();

    LoopStack& GetLoopStack();


    // --------------------------------------------------------------------------
    // numbers
    // --------------------------------------------------------------------------
public:
    int ConserveConstant(double numeric_constant);
    int CreateNumericConstantNode(double numeric_constant);

    static bool IsNumericConstantInteger(double value);
    bool IsNumericConstantInteger() const;

    WorkVariable* CompileWorkVariableDeclaration();
    int CompileWorkVariables();


    // --------------------------------------------------------------------------
    // strings
    // --------------------------------------------------------------------------
public:
    int ConserveConstant(const std::wstring& string_literal);
    int ConserveConstant(std::wstring&& string_literal);
    int CreateStringLiteralNode(std::wstring string_literal);

    int CompileStringExpression();
    int CompileStringExpressionWithStringLiteralCheck(const std::function<void(const std::wstring&)>& string_literal_check_callback);

    int CompilePortableColorText();
    int CompileSymbolNameText(SymbolType required_symbol_type = SymbolType::None, bool throw_exception_is_symbol_is_not_found = true);
    int CompileFillText();

    int CompileStringComputeInstruction();

    WorkString* CompileLogicStringDeclaration(TokenCode token_code, const WorkString* work_string_to_copy_attributes = nullptr);
    int CompileLogicStrings();


    // --------------------------------------------------------------------------
    // symbols
    // --------------------------------------------------------------------------
public:
    std::wstring CompileNewSymbolName();
    int CompileSymbolWithModifiers();
    int CompileSymbolRouter();

    int& AddSymbolResetNode(Nodes::SymbolReset*& symbol_reset_node, const Symbol& symbol);
    unsigned CompileAlphaLength();
    int CompileSymbolInitialAssignment(const Symbol& symbol);

    void CompileAlias();
    void CompileEnsure();

    int CompileSymbolFunctions();


    // --------------------------------------------------------------------------
    // Array object
    // --------------------------------------------------------------------------
public:
    LogicArray* CompileLogicArrayDeclarationOnly(bool use_function_parameter_syntax);
    int CompileLogicArrayDeclaration();
    int CompileLogicArrayReference();
    int CompileLogicArrayFunctions();


    // --------------------------------------------------------------------------
    // Audio object
    // --------------------------------------------------------------------------
public:
    LogicAudio* CompileLogicAudioDeclaration();
    int CompileLogicAudioDeclarations();
    int CompileLogicAudioComputeInstruction(const LogicAudio* logic_audio_from_declaration = nullptr);
    int CompileLogicAudioFunctions();


    // --------------------------------------------------------------------------
    // Barcode namespace
    // --------------------------------------------------------------------------
public:
    int CompileBarcodeFunctions();


    // --------------------------------------------------------------------------
    // "Control Flow" statements
    // --------------------------------------------------------------------------
public:
    int CompileIfStatement();
    int CompileWhileLoop();
	int CompileDoLoop();
	int CompileNextOrBreakInLoop();


    // --------------------------------------------------------------------------
    // CS namespace
    // --------------------------------------------------------------------------
public:
    int CompileActionInvokerFunctions();


    // --------------------------------------------------------------------------
    // "Data Access" functionality
    // --------------------------------------------------------------------------
public:
    int WrapNodeAroundValidDataAccessCheck(int program_index, const Symbol& symbol, std::variant<FunctionCode, DataType> function_code_or_data_type);


    // --------------------------------------------------------------------------
    // Dictionary-related objects (Case and DataSource)
    // --------------------------------------------------------------------------
public:
    EngineDictionary* CompileEngineCaseDeclaration(const EngineDictionary* engine_dictionary_to_copy_attributes = nullptr);
    int CompileEngineCases();
    int CompileEngineCaseComputeInstruction(const EngineDictionary* engine_dictionary_from_declaration = nullptr);

    EngineDictionary* CompileEngineDataRepositoryDeclaration(const EngineDictionary* engine_dictionary_to_copy_attributes = nullptr);
    int CompileEngineDataRepositories();

    int CompileCaseFunctions();


    // --------------------------------------------------------------------------
    // Dictionary-related functions, statements, and checks
    // --------------------------------------------------------------------------
public:
    int CompileDictionaryFunctionsVarious();
    int CompileDictionaryFunctionsCaseSearch();
    int CompileDictionaryFunctionsCaseIO();
    int CompileDictionaryFunctionsCaseIO_pre80(FunctionCode function_code);

    int CompileForDictionaryLoop(TokenCode token_code);

    int CompileSetAccessFirstLast(SetAction set_action);
    int CompileDictionaryAccess(EngineDictionary& engine_dictionary, int* starts_with_expression = nullptr);
    int CompileDictionaryAccess(DICT* pDicT, int* starts_with_expression = nullptr);

    void VerifyDictionaryObject(const EngineDictionary* engine_dictionary = nullptr);
    void VerifyEngineCase(const EngineDictionary* engine_dictionary = nullptr);
    void VerifyEngineDataRepository(EngineDictionary* engine_dictionary = nullptr, int flags = 0);
    void VerifyEngineDataRepositoryWithEngineCase(const EngineDictionary& data_repository_engine_dictionary, const EngineDictionary& case_engine_dictionary);
    void VerifyDictionary(DICT* pDicT, int iFlags);


    // --------------------------------------------------------------------------
    // Document object
    // --------------------------------------------------------------------------
public:
    LogicDocument* CompileLogicDocumentDeclaration();
    int CompileLogicDocumentDeclarations();
    int CompileLogicDocumentComputeInstruction(const LogicDocument* logic_document_from_declaration = nullptr);
    int CompileLogicDocumentFunctions();


    // --------------------------------------------------------------------------
    // File object and other file-related functions
    // --------------------------------------------------------------------------
public:
    LogicFile* CompileLogicFileDeclaration(bool compiling_function_parameter = false);
    int CompileLogicFiles();
    int CompileLogicFileFunctions();

    int CompileSetFileFunction();


    // --------------------------------------------------------------------------
    // Freq statement and object
    // --------------------------------------------------------------------------
public:
    int CompileFrequencyDeclaration();
    int CompileNamedFrequencyComputeInstruction();
    int CompileNamedFrequencyReference();
    int CompileNamedFrequencyFunctions();


    // --------------------------------------------------------------------------
    // Geometry object
    // --------------------------------------------------------------------------
public:
    LogicGeometry* CompileLogicGeometryDeclaration();
    int CompileLogicGeometryDeclarations();
    int CompileLogicGeometryComputeInstruction(const LogicGeometry* logic_geometry_from_declaration = nullptr);
    int CompileLogicGeometryFunctions();


    // --------------------------------------------------------------------------
    // HashMap object
    // --------------------------------------------------------------------------
public:
    LogicHashMap* CompileLogicHashMapDeclaration(const LogicHashMap* hashmap_to_copy_attributes = nullptr);
    int CompileLogicHashMapDeclarations();
    int CompileLogicHashMapComputeInstruction(const LogicHashMap* hashmap_from_declaration = nullptr);
    int CompileLogicHashMapReference(const LogicHashMap* hashmap = nullptr);
    int CompileLogicHashMapFunctions();


    // --------------------------------------------------------------------------
    // Image object
    // --------------------------------------------------------------------------
public:
    LogicImage* CompileLogicImageDeclaration();
    int CompileLogicImageDeclarations();
    int CompileLogicImageComputeInstruction(const LogicImage* logic_image_from_declaration = nullptr);
    int CompileLogicImageFunctions();


    // --------------------------------------------------------------------------
    // Item-related functions and checks
    // --------------------------------------------------------------------------
public:
    int CompileItemSubscriptExplicit(const EngineItem& engine_item);
    int CompileItemSubscriptImplicit(const EngineItem& engine_item, Logic::FunctionDetails::StaticType static_type_of_use);

    int CompileItemFunctions();

private:
    template<bool fallback_to_static_compilation>
    int ValidateItemSubscriptAndCreateNode(const EngineItem& engine_item, const std::tuple<SubscriptValueType, int> subscripts[]);


    // --------------------------------------------------------------------------
    // impute function
    // --------------------------------------------------------------------------
public:
    int CompileImputeFunction();
    void CompileSetImpute();


    // --------------------------------------------------------------------------
    // JSON-related functions
    // --------------------------------------------------------------------------
public:
    int CompileJsonText(const std::function<void(const JsonNode<wchar_t>& json_node)>& json_node_callback = { });


    // --------------------------------------------------------------------------
    // List object
    // --------------------------------------------------------------------------
public:
    LogicList* CompileLogicListDeclaration(const LogicList* list_to_copy_attributes = nullptr);
    int CompileLogicListDeclarations();
    int CompileLogicListComputeInstruction(const LogicList* list_from_declaration = nullptr);
    int CompileLogicListReference(const LogicList* logic_list = nullptr);
    int CompileLogicListFunctions();


    // --------------------------------------------------------------------------
    // Map object
    // --------------------------------------------------------------------------
public:
    LogicMap* CompileLogicMapDeclaration();
    int CompileLogicMapDeclarations();
    int CompileLogicMapFunctions();


    // --------------------------------------------------------------------------
    // "Message" functions
    // --------------------------------------------------------------------------
public:
    int CompileMessageFunctions();
    int CompileMessageFunction(FunctionCode function_code);


    // --------------------------------------------------------------------------
    // Path namespace and other path-related functions
    // --------------------------------------------------------------------------
public:
    int CompilePathFunctions();

private:
    int CompileDirectoryVariant(bool allow_string_expression, bool allow_path_type, bool allow_media_type, bool allow_symbol);
    int CompilePathFilter();


    // --------------------------------------------------------------------------
    // Pff object
    // --------------------------------------------------------------------------
public:
    LogicPff* CompileLogicPffDeclaration();
    int CompileLogicPffDeclarations();
    int CompileLogicPffComputeInstruction(const LogicPff* pff_from_declaration = nullptr);
    int CompileLogicPffFunctions();


    // --------------------------------------------------------------------------
    // Report object
    // --------------------------------------------------------------------------
public:
    int CompileReportFunctions();
    void CheckReportIsCurrentlyWriteable(const Report& report);

    void CompileReports();
    virtual void CompileReport(const NamedTextSource& report_named_text_source);

private:
    std::unique_ptr<Logic::SourceBuffer> ConvertReportToSourceBuffer(wstring_view report_text_sv);


    // --------------------------------------------------------------------------
    // "Switch" functionality
    // --------------------------------------------------------------------------
public:
    int CompileSwitch(bool compiling_when, const std::function<std::vector<int>()>& result_destinations_compiler, const std::function<int(size_t)>& action_compiler);
    int CompileWhen();
    int CompileRecode();

    int CreateInNode(DataType data_type, int left_expr, int right_expr);
    int CompileInNodes(DataType data_type, CompilationExtendedInformation::InCrosstabInformation* in_crosstab_information = nullptr);


    // --------------------------------------------------------------------------
    // SystemApp object
    // --------------------------------------------------------------------------
public:
    SystemApp* CompileSystemAppDeclaration();
    int CompileSystemAppDeclarations();
    int CompileSystemAppFunctions();


    // --------------------------------------------------------------------------
    // UserFunction object and invoke function
    // --------------------------------------------------------------------------
public:
    UserFunction* CompileUserFunction(bool compiling_function_pointer = false);
    int CompileUserFunctionCall(bool allow_function_name_without_parentheses = false);

    bool IsFunctionParameterSymbol(const Symbol& symbol);

    int CompileInvokeFunction();


    // --------------------------------------------------------------------------
    // "User Interface" functions
    // --------------------------------------------------------------------------
public:
    int CompileUserInterfaceFunctions();

    int CompileViewerOptions(bool allow_left_parenthesis_starting_token);


    // --------------------------------------------------------------------------
    // trace function
    // --------------------------------------------------------------------------
public:
    bool IsTracingLogic() const { return m_tracingLogic; }
    void CompileSetTrace();
	int CompileTraceFunction();
    int CreateTraceStatement();


    // --------------------------------------------------------------------------
    // ValueSet object and setvalueset function
    // --------------------------------------------------------------------------
public:
    DynamicValueSet* CompileDynamicValueSetDeclaration(const DynamicValueSet* value_set_to_copy_attributes = nullptr);
    int CompileDynamicValueSetDeclarations();
    int CompileDynamicValueSetComputeInstruction(const DynamicValueSet* value_set_from_declaration = nullptr);
    int CompileValueSetFunctions();

    int CompileSetValueSetFunction();


    // --------------------------------------------------------------------------
    // "Variable" compilers
    // --------------------------------------------------------------------------
public:
    int CompileDestinationVariable(std::variant<DataType, const Symbol*> data_type_or_symbol);


    // --------------------------------------------------------------------------
    // generic function compilers
    // --------------------------------------------------------------------------
public:
    int CompileExpression(DataType data_type);
    int CompileExpressionOrObject(const std::vector<GF::VariableType>& variable_types, const TCHAR* argument_name = _T("unknown"));

    int CompileFunctionCall(int program_index = -1);

    int CompileFunctionsArgumentsFixedN();
    int CompileFunctionsArgumentsVaryingN();
    int CompileFunctionsArgumentSpecification();

    int CompileFunctionsRemovedFromLanguage();
    int CompileFunctionsVarious();


    // --------------------------------------------------------------------------
    // specialized function and statement compilers
    // --------------------------------------------------------------------------
public:
    int CompileGpsFunction();
    int CompileParadataFunction();
    int CompileSqlQueryFunction(bool from_paradata_function = false);
    int CompileSyncFunctions();
    int CompileUserbarFunction();


    // --------------------------------------------------------------------------
    // basic expressions
    // --------------------------------------------------------------------------
public:
    int exprlog();
    int expror();
    int termlog();
    int factlog();
    int expr();
    int term();
    int factor();
    int prim();


    // --------------------------------------------------------------------------
    // token and next token helpers
    // --------------------------------------------------------------------------
public:
    // gets the current token's data type (between Numeric and String); if unknown, DataType::Numeric is returned
    DataType GetCurrentTokenDataType();

    bool IsCurrentTokenString() { return IsString(GetCurrentTokenDataType()); }

    enum class NextTokenHelperResult { Unknown, NumericConstantNonNegative, StringLiteral, WorkString, Array, List, DictionaryRelatedSymbol };
    NextTokenHelperResult CheckNextTokenHelper(SymbolType preferred_symbol_type = SymbolType::None);

    std::optional<SymbolType> GetNextTokenSymbolType();


    // --------------------------------------------------------------------------
    // BasicTokenCompiler overrides
    // --------------------------------------------------------------------------
public:
    const std::wstring& GetCurrentProcName() const override;


    // --------------------------------------------------------------------------
    // BaseCompiler overrides and related
    // --------------------------------------------------------------------------
protected:
    void ProcessSymbol() override;
    void ProcessFunction() override;

private:
    void ProcessSymbolEngineItem(EngineItem& engine_item);


    // --------------------------------------------------------------------------
    // methods for subclasses to override
    // --------------------------------------------------------------------------
protected:
    virtual MessageManager& GetUserMessageManager() = 0;
    virtual MessageEvaluator& GetUserMessageEvaluator() = 0;


    // --------------------------------------------------------------------------
    // other methods
    // --------------------------------------------------------------------------
public:
    Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }


    // --------------------------------------------------------------------------
    // COMPILER_DLL_TODO...
    // --------------------------------------------------------------------------
public:
    virtual int& get_COMPILER_DLL_TODO_Tokstindex() = 0;
    virtual int& get_COMPILER_DLL_TODO_InCompIdx() = 0;
    virtual std::tuple<int, bool>& get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester() = 0;

    virtual int CompileHas_COMPILER_DLL_TODO(int iVarNode) = 0;
    virtual int crelalpha_COMPILER_DLL_TODO() = 0;
    virtual int varsanal_COMPILER_DLL_TODO(int fmt) = 0;
    virtual int tvarsanal_COMPILER_DLL_TODO() = 0;
    virtual int rutfunc_COMPILER_DLL_TODO() = 0;
    virtual int instruc_COMPILER_DLL_TODO(bool create_new_local_symbol_stack = true, bool allow_multiple_statements = true) = 0;
    virtual DICT* GetInputDictionary(bool issue_error_if_no_input_dictionary) = 0;
    virtual void MarkAllDictionaryItemsAsUsed() = 0;
    virtual void MarkAllInSectionUsed(SECT* pSecT) = 0;

    virtual int CompileReenterStatement_COMPILER_DLL_TODO(bool bNextTkn = true) = 0;
    virtual int CompileMoveStatement_COMPILER_DLL_TODO(bool bFromSelectStatement = false) = 0;
    virtual void rutasync_as_global_compilation_COMPILER_DLL_TODO(const Symbol& compilation_symbol, const std::function<void()>& compilation_function) = 0;
    virtual void CheckIdChanger(const VART* pVarT) = 0;

    virtual std::vector<const DictNamedBase*> GetImplicitSubscriptCalculationStack(const EngineItem& engine_item) const = 0;


    // --------------------------------------------------------------------------
    // data (anything that may be null will be noted)
    // --------------------------------------------------------------------------
protected:
    std::shared_ptr<EngineData> m_engineData;

private:
    const Symbol* m_compilationSymbol; // non-null during compilation

    std::vector<std::shared_ptr<CompilerHelper>> m_compilerHelpers;

    SymbolCompilerModifier m_symbolCompilerModifier;

    std::unique_ptr<ConstantConserver<double>> m_numericConstantConserver;
    std::unique_ptr<ConstantConserver<std::wstring>> m_stringLiteralConserver;

    std::vector<const Symbol*> m_functionParameterSymbols;

    bool m_tracingLogic;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

#include <zEngineO/Compiler/NodeCreationCC.h>


inline bool LogicCompiler::IsFunctionParameterSymbol(const Symbol& symbol)
{
    return ( std::find(m_functionParameterSymbols.cbegin(), m_functionParameterSymbols.cend(), &symbol) != m_functionParameterSymbols.cend() );
}
