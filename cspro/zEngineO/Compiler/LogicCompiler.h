#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/EngineData.h>
#include <zEngineO/ProcType.h>
#include <zEngineO/Compiler/SymbolCompilerModifier.h>
#include <zEngineO/Nodes/BaseNodes.h>
#include <zLogicO/BaseCompiler.h>
#include <zLogicO/FunctionTable.h>

class CodeFile;
class CompilerHelper;
template<typename T> class ConstantConserver;
class DictNamedBase;
class DynamicValueSet;
enum class EngineAppType : int;
class EnginePreprocessor;
class LoopStack;
class MessageEvaluator;
class MessageManager;
class ReportFile;
enum class SetAction : int;
class TextTemplateTokenizer;
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
    LogicCompiler(cs::non_null_shared_or_raw_ptr<EngineData> engine_data);
    ~LogicCompiler();


    // --------------------------------------------------------------------------
    // current compilation information and compilers
    // (CompilersCC.cpp)
    // --------------------------------------------------------------------------
public:
    void SetCompilationSymbol(const Symbol* symbol) noexcept;

    const Symbol* GetCompilationSymbol() const noexcept { return m_compilationSymbol; }
    SymbolType GetCompilationSymbolType() const noexcept;
    int GetCompilationLevelNumber_base1() const;

    bool IsCompiling(const Symbol& symbol) const noexcept;
    bool IsCompiling(SymbolType symbol_type) const noexcept;
    bool IsGlobalCompilation() const noexcept;
    bool IsNoLevelCompilation() const noexcept;

    EngineAppType GetEngineAppType() const;

    ProcType GetCompilationProcType() const { return m_procType; }
    void SetCompilationProcType(ProcType proc_type, ExtendedProcType extended_proc_type = ExtendedProcType::None);

    // Compiles the current source buffer for the symbol, if provided.
    // An optional compilation function can be provided.
    // The method will not throw exceptions unless a subclass' implementation of ReportError does.
    int CompileSourceBuffer(const Symbol* compilation_symbol, const std::function<int()>* compilation_function = nullptr);

    void CompileExternalCode();
    virtual void CompileExternalCode(const CodeFile& code_file);

    void RunPostCompilationChecks();

private:
    virtual void CompileExternalCodeLogic(const CodeFile& code_file) = 0; // COMPILER_DLL_TODO remove virtual
    virtual void CompileExternalCodeJavaScript(const CodeFile& code_file) = 0; // COMPILER_DLL_TODO remove virtual


    // --------------------------------------------------------------------------
    // methods to create compilation nodes
    // (NodeCreationCC.cpp)
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

    Nodes::SymbolVariableArguments& CreateSymbolVariableArgumentsNode(FunctionCode function_code, const Symbol& symbol, int number_arguments);
    Nodes::SymbolVariableArguments& CreateSymbolVariableArgumentsNode(FunctionCode function_code, const Symbol& symbol, int number_arguments, int initialize_value);

    int CreateSymbolVariableArgumentsWithSubscriptNode(FunctionCode function_code, const Symbol& symbol, int symbol_subscript_compilation, cs::span<const int> arguments);

    Nodes::SymbolVariableArgumentsWithSubscript& CreateSymbolVariableArgumentsWithSubscriptNode(FunctionCode function_code, const Symbol& symbol, int symbol_subscript_compilation,
                                                                                                int number_arguments, std::optional<int> initialize_value = std::nullopt);

    int WrapNodeAroundScopeChange(const Logic::LocalSymbolStack& local_symbol_stack, int program_index, bool store_local_symbol_names = false);


    // --------------------------------------------------------------------------
    // token and next token helpers
    // (NextTokenCC.cpp + TokenCC.cpp)
    // --------------------------------------------------------------------------
public:
    // gets the current token's data type (between Numeric and String); if unknown, DataType::Numeric is returned
    DataType GetCurrentTokenDataType();

    bool IsCurrentTokenString() { return IsString(GetCurrentTokenDataType()); }

    enum class NextTokenHelperResult { Unknown, NumericConstantNonNegative, StringLiteral, WorkString, Array, List, DictionaryRelatedSymbol };
    NextTokenHelperResult CheckNextTokenHelper(SymbolType preferred_symbol_type = SymbolType::None);

    std::optional<SymbolType> GetNextTokenSymbolType();


    // --------------------------------------------------------------------------
    // compiler helpers
    // (CompilerHelper.cpp)
    // --------------------------------------------------------------------------
public:
    template<typename T>
    T& GetCompilerHelper();

    LoopStack& GetLoopStack();


    // --------------------------------------------------------------------------
    // basic expressions
    // (ExpressionsCC.cpp)
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
    // routing methods
    // (RoutingCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileStatements(bool create_new_local_symbol_stack = true, bool allow_multiple_statements = true);

protected: // COMPILER_DLL_TODO make private
    static bool IsValidStatementStartToken(const TokenCode token_code) noexcept;
    static bool IsValidStatementEndToken(const TokenCode token_code) noexcept;

    int RouteFunctionCall();


    // --------------------------------------------------------------------------
    // "Control Flow" statements
    // (ControlFlowCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileIfStatement();
    int CompileWhileLoop();
    int CompileDoLoop();
    int CompileNextOrBreakInLoop();


    // --------------------------------------------------------------------------
    // math
    // (MathCC.cpp)
    // --------------------------------------------------------------------------
public:
    int ConserveConstant(double numeric_constant);
    int CreateNumericConstantNode(double numeric_constant);

    static bool IsNumericConstantInteger(double value);
    bool IsNumericConstantInteger() const;

    WorkVariable* CompileWorkVariableDeclaration();
    int CompileWorkVariables();
    int CompileWorkVariableReference();

    // Compiles assignment statements for: Array, function, and numeric.
    int CompileNumericComputeInstruction();


    // --------------------------------------------------------------------------
    // strings
    // (StringsCC.cpp)
    // --------------------------------------------------------------------------
public:
    int ConserveConstant(const std::string& string_literal);
    int ConserveConstant(std::string&& string_literal);
    int ConserveConstant(SharableString&& string_literal);
    int CreateStringLiteralNode(SharableString string_literal);

    int CompileStringExpression();
    int CompileStringExpressionWithStringLiteralCheck(const std::function<void(std::string)>& string_literal_check_callback);

    int CompilePortableColorText();
    int CompileSymbolNameText(SymbolType required_symbol_type = SymbolType::None, bool throw_exception_is_symbol_is_not_found = true);
    int CompileFillText();

    // Compiles assignment statements for: Array, function, string, and dictionary items.
    int CompileStringComputeInstruction();

    WorkString* CompileLogicStringDeclaration(TokenCode token_code, const WorkString* work_string_to_copy_attributes = nullptr);
    int CompileLogicStrings();


    // --------------------------------------------------------------------------
    // symbols
    // (SymbolsCC.cpp)
    // --------------------------------------------------------------------------
public:
    std::string CompileNewSymbolName(std::optional<TokenCode> additional_token_allowed = std::nullopt);
    int CompileSymbolWithModifiers();
    int CompileSymbolRouter();

    int& AddSymbolResetNode(Nodes::SymbolReset*& symbol_reset_node, const Symbol& symbol);
    unsigned CompileAlphaLength();
    int CompileSymbolInitialAssignment(const Symbol& symbol);

    bool IsFunctionParameterSymbol(const Symbol& symbol) const;

    void CompileAlias();
    void CompileEnsure();

    int CompileSymbolFunctions();


    // --------------------------------------------------------------------------
    // Array object
    // (ArrayCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicArray* CompileLogicArrayDeclarationOnly(bool use_function_parameter_syntax);
    int CompileLogicArrayDeclaration();
    int CompileLogicArrayComputeInstruction();
    int CompileLogicArrayReference();
    int CompileLogicArrayFunctions();


    // --------------------------------------------------------------------------
    // Audio object
    // (AudioCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicAudio* CompileLogicAudioDeclaration();
    int CompileLogicAudioDeclarations();
    int CompileLogicAudioComputeInstruction(const LogicAudio* logic_audio_from_declaration = nullptr);
    int CompileLogicAudioFunctions();


    // --------------------------------------------------------------------------
    // Barcode namespace
    // (BarcodeCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileBarcodeFunctions();


    // --------------------------------------------------------------------------
    // CS namespace
    // (ActionInvokerCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileActionInvokerFunctions();


    // --------------------------------------------------------------------------
    // "Data Access" functionality
    // (DataAccessCC.cpp)
    // --------------------------------------------------------------------------
public:
    int WrapNodeAroundValidDataAccessCheck(int program_index, const Symbol& symbol, std::variant<FunctionCode, DataType> function_code_or_data_type);


    // --------------------------------------------------------------------------
    // Dictionary-related objects (Case and DataSource)
    // (CaseCC.cpp + EngineDictionaryCC.cpp)
    // --------------------------------------------------------------------------
public:
    EngineDictionary* CompileEngineCaseDeclaration(EngineDictionary* engine_dictionary_to_copy_attributes = nullptr);
    int CompileEngineCases();
    int CompileEngineCaseComputeInstruction(EngineDictionary* engine_dictionary_from_declaration = nullptr);

    EngineDictionary* CompileEngineDataRepositoryDeclaration(EngineDictionary* engine_dictionary_to_copy_attributes = nullptr);
    int CompileEngineDataRepositories();

    int CompileCaseFunctions();


    // --------------------------------------------------------------------------
    // Dictionary-related functions, statements, and checks
    // (DictionaryCC.cpp)
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
    // (DocumentCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicDocument* CompileLogicDocumentDeclaration();
    int CompileLogicDocumentDeclarations();
    int CompileLogicDocumentComputeInstruction(const LogicDocument* logic_document_from_declaration = nullptr);
    int CompileLogicDocumentFunctions();


    // --------------------------------------------------------------------------
    // File object and other file-related functions
    // (FileCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicFile* CompileLogicFileDeclaration(bool compiling_function_parameter);
    int CompileLogicFiles();
    int CompileLogicFileFunctions();

    int CompileSetFileFunction();


    // --------------------------------------------------------------------------
    // Freq statement and object
    // (FreqCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileFrequencyDeclaration();
    int CompileNamedFrequencyComputeInstruction();
    int CompileNamedFrequencyReference();
    int CompileNamedFrequencyFunctions();


    // --------------------------------------------------------------------------
    // Geometry object
    // (GeometryCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicGeometry* CompileLogicGeometryDeclaration();
    int CompileLogicGeometryDeclarations();
    int CompileLogicGeometryComputeInstruction(const LogicGeometry* logic_geometry_from_declaration = nullptr);
    int CompileLogicGeometryFunctions();


    // --------------------------------------------------------------------------
    // HashMap object
    // (HashMapCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicHashMap* CompileLogicHashMapDeclaration(const LogicHashMap* hashmap_to_copy_attributes = nullptr);
    int CompileLogicHashMapDeclarations();
    int CompileLogicHashMapComputeInstruction(const LogicHashMap* hashmap_from_declaration = nullptr);
    int CompileLogicHashMapReference(const LogicHashMap* hashmap = nullptr);
    int CompileLogicHashMapFunctions();


    // --------------------------------------------------------------------------
    // Image object
    // (ImageCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicImage* CompileLogicImageDeclaration();
    int CompileLogicImageDeclarations();
    int CompileLogicImageComputeInstruction(const LogicImage* logic_image_from_declaration = nullptr);
    int CompileLogicImageFunctions();


    // --------------------------------------------------------------------------
    // Item-related functions and checks
    // (ItemCC.cpp)
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
    // (ImputeCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileImputeFunction();
    void CompileSetImpute();


    // --------------------------------------------------------------------------
    // JS (JavaScript) namespace
    // (JavaScriptCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileJavaScriptFunctions();

private:
    std::tuple<int, int> CompileJavaScriptConvertableValue();


    // --------------------------------------------------------------------------
    // JSON-related functions
    // (JsonCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileJsonText(const std::function<void(const JsonNode& json_node)>& json_node_callback = { });


    // --------------------------------------------------------------------------
    // List object
    // (ListCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicList* CompileLogicListDeclaration(const LogicList* list_to_copy_attributes = nullptr);
    int CompileLogicListDeclarations();
    int CompileLogicListComputeInstruction(const LogicList* list_from_declaration = nullptr);
    int CompileLogicListReference(const LogicList* logic_list = nullptr);
    int CompileLogicListFunctions();


    // --------------------------------------------------------------------------
    // Map object
    // (MapCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicMap* CompileLogicMapDeclaration();
    int CompileLogicMapDeclarations();
    int CompileLogicMapFunctions();


    // --------------------------------------------------------------------------
    // "Message" functions
    // (MessagesCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileMessageFunctions();
    int CompileMessageFunction(FunctionCode function_code);


    // --------------------------------------------------------------------------
    // Path namespace and other path-related functions
    // (PathCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompilePathFunctions();

private:
    int CompileDirectoryVariant(bool allow_string_expression, bool allow_path_type, bool allow_media_type, bool allow_symbol);
    int CompilePathFilter();


    // --------------------------------------------------------------------------
    // Pff object
    // (PffCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicPff* CompileLogicPffDeclaration();
    int CompileLogicPffDeclarations();
    int CompileLogicPffComputeInstruction(const LogicPff* pff_from_declaration = nullptr);
    int CompileLogicPffFunctions();


    // --------------------------------------------------------------------------
    // Report object
    // (ReportCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileReportFunctions();

    void CompileReports();
    virtual void CompileReport(const ReportFile& report_file);


    // --------------------------------------------------------------------------
    // StringWriter object
    // (StringWriterCC.cpp)
    // --------------------------------------------------------------------------
public:
    StringWriter* CompileStringWriterDeclaration(bool compiling_function_parameter);
    int CompileStringWriterDeclarations();
    int CompileStringWriterFunctions();


    // --------------------------------------------------------------------------
    // "Switch" functionality
    // (SwitchCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileSwitch(bool compiling_when, const std::function<std::vector<int>()>& result_destinations_compiler, const std::function<int(size_t)>& action_compiler);
    int CompileWhen();
    int CompileRecode();

    int CreateInNode(DataType data_type, int left_expr, int right_expr);
    int CompileInNodes(DataType data_type, CompilationExtendedInformation::InCrosstabInformation* in_crosstab_information = nullptr);


    // --------------------------------------------------------------------------
    // SystemApp object
    // (SystemAppCC.cpp)
    // --------------------------------------------------------------------------
public:
    SystemApp* CompileSystemAppDeclaration();
    int CompileSystemAppDeclarations();
    int CompileSystemAppFunctions();


    // --------------------------------------------------------------------------
    // Text Template functions
    // (TextTemplateCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileTextTemplateFunctions();

    // If the symbol is a StringWriter, the underlying type (e.g., a Report) is returned.
    // An exception is thrown is the symbol is not currently accessible.
    const Symbol& CheckTextTemplateIsCurrentlyAccessible(const Symbol& symbol);

private:
    std::unique_ptr<Logic::SourceBuffer> ConvertTextTemplateToSourceBuffer(const char* text_template_name, std::string_view text_template_sv, bool allow_logic_escapes);
    std::unique_ptr<Logic::SourceBuffer> ConvertTextTemplateToSourceBuffer(const char* text_template_name, TextTemplateTokenizer& text_template_tokenizer);


    // --------------------------------------------------------------------------
    // UserFunction object and invoke function
    // (UserFunctionCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileUserFunctionDeclarations();
    int CompileUserFunctionComputeInstruction();
    int CompileUserFunctionCall(bool allow_function_name_without_parentheses = false);

    int CompileInvokeFunction();

private:
    enum class UserFunctionParametersType;
    UserFunction* CompileUserFunction(bool compiling_function_pointer);
    void CompileUserFunctionParameters(UserFunction& user_function, bool function_was_previously_declared, UserFunctionParametersType parameters_type);


    // --------------------------------------------------------------------------
    // "User Interface" functions
    // (UserInterfaceCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileUserInterfaceFunctions();

    int CompileViewerOptions(bool allow_left_parenthesis_starting_token);


    // --------------------------------------------------------------------------
    // trace function
    // (TraceCC.cpp)
    // --------------------------------------------------------------------------
public:
    bool IsTracingLogic() const { return m_tracingLogic; }
    void CompileSetTrace();
    int CompileTraceFunction();
    int CreateTraceStatement();


    // --------------------------------------------------------------------------
    // ValueSet object and setvalueset function
    // (ValueSetCC.cpp)
    // --------------------------------------------------------------------------
public:
    DynamicValueSet* CompileDynamicValueSetDeclaration(const DynamicValueSet* value_set_to_copy_attributes = nullptr);
    int CompileDynamicValueSetDeclarations();
    int CompileDynamicValueSetComputeInstruction(const DynamicValueSet* value_set_from_declaration = nullptr);
    int CompileValueSetFunctions();

    int CompileSetValueSetFunction();


    // --------------------------------------------------------------------------
    // "Variable" compilers
    // (VariableCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileDestinationVariable(std::variant<DataType, std::reference_wrapper<const Symbol>> data_type_or_symbol);


    // --------------------------------------------------------------------------
    // Video object
    // (VideoCC.cpp)
    // --------------------------------------------------------------------------
public:
    LogicVideo* CompileLogicVideoDeclaration();
    int CompileLogicVideoDeclarations();
    int CompileLogicVideoComputeInstruction(const LogicVideo* logic_video_from_declaration = nullptr);
    int CompileLogicVideoFunctions();


    // --------------------------------------------------------------------------
    // generic function compilers
    // (FunctionsGenericCC.cpp + FunctionsVariousCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileExpression(DataType data_type);
    int CompileExpressionOrObject(const std::vector<GF::VariableType>& variable_types, const char* argument_name = "unknown");

    int CompileFunctionCall();

    int CompileFunctionsArgumentsFixedN();
    int CompileFunctionsArgumentsVaryingN();
    int CompileFunctionsArgumentSpecification();

    int CompileFunctionsRemovedFromLanguage();
    int CompileFunctionsVarious();


    // --------------------------------------------------------------------------
    // specialized function and statement compilers
    // (GpsCC.cpp + QueryCC.cpp + SyncCC.cpp + UserbarCC.cpp)
    // --------------------------------------------------------------------------
public:
    int CompileGpsFunction();
    int CompileParadataFunction();
    int CompileSqlQueryFunction(bool from_paradata_function = false);
    int CompileSyncFunctions();
    int CompileUserbarFunction();


    // --------------------------------------------------------------------------
    // BasicTokenCompiler overrides
    // --------------------------------------------------------------------------
public:
    std::string GetCurrentProcName() const override;


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
    friend class CEngineCompFunc; // COMPILER_DLL_TODO remove once all all functionality is in this class

    virtual int& get_COMPILER_DLL_TODO_Tokstindex() = 0;
    virtual int& get_COMPILER_DLL_TODO_InCompIdx() = 0;
    virtual std::tuple<int, bool>& get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester() = 0;

    virtual int CompileHas_COMPILER_DLL_TODO(int iVarNode) = 0;
    virtual int crelalpha_COMPILER_DLL_TODO() = 0;
    virtual int varsanal_COMPILER_DLL_TODO(int fmt) = 0;
    virtual int tvarsanal_COMPILER_DLL_TODO() = 0;
    virtual int rutfunc_COMPILER_DLL_TODO(Logic::FunctionCompilationType compilation_type) = 0;
    virtual int instruc_COMPILER_DLL_TODO(bool allow_multiple_statements = true) = 0;
    virtual DICT* GetInputDictionary(bool issue_error_if_no_input_dictionary) = 0;
    virtual void MarkAllDictionaryItemsAsUsed() = 0;
    virtual void MarkAllInSectionUsed(SECT* pSecT) = 0;
    virtual void SetCaseAccessSetRequiresFullAccess_COMPILER_DLL_TODO(Symbol& symbol) = 0;

    virtual int CompileReenterStatement_COMPILER_DLL_TODO(bool bNextTkn = true) = 0;
    virtual int CompileMoveStatement_COMPILER_DLL_TODO(bool bFromSelectStatement = false) = 0;
    virtual void rutasync_as_global_compilation_COMPILER_DLL_TODO(const Symbol& compilation_symbol, const std::function<void()>& compilation_function) = 0;
    virtual void CheckIdChanger(const VART* pVarT) = 0;

    virtual std::vector<const DictNamedBase*> GetImplicitSubscriptCalculationStack(const EngineItem& engine_item) const = 0;


    // --------------------------------------------------------------------------
    // data (anything that may be null will be noted)
    // --------------------------------------------------------------------------
protected:
    cs::non_null_shared_or_raw_ptr<EngineData> m_engineData;

    std::unique_ptr<EnginePreprocessor> m_preprocessor;

private:
    // The symbol that is currently being compiled (null if not applicable).
    const Symbol* m_compilationSymbol;

    // The type of the procedure currently being compiled.
    ProcType m_procType;
    ExtendedProcType m_extendedProcType;

    std::vector<std::shared_ptr<CompilerHelper>> m_compilerHelpers;

    SymbolCompilerModifier m_symbolCompilerModifier;

    std::unique_ptr<ConstantConserver<double>> m_numericConstantConserver;
    std::unique_ptr<ConstantConserver<SharableString>> m_stringLiteralConserver;

    std::set<int> m_declaredSymbolIndices;
    std::vector<const Symbol*> m_functionParameterSymbols;

    bool m_tracingLogic;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

#include <zEngineO/Compiler/NodeCreationCC.h>
