#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/EngineData.h>
#include <zToolsO/CancelFlag.h>
#include <zToolsO/Special.h>
#include <zToolsO/Tools.h>
#include <zUtilO/DataTypes.h>
#include <zUtilO/Viewers.h>
#include <zMessageO/MessageType.h>
#include <zLogicO/TokenCode.h>

class ApplicationInterface;
class BinarySymbol;
class CIntDriver;
class ConnectionString;
enum class EncodeType : int;
class EngineParadataDriver;
class FrequencyDriver;
enum FunctionCode : int;
class PortableColor;
class JsonReaderInterface;
class UserFunctionArgumentEvaluator;
class VirtualFileMappingHandler;
namespace ActionInvoker { class Caller; class Runtime; }
namespace JavaScript { class Value; }
namespace Nodes { struct ItemSubscript; struct List; struct SymbolComputeWithSubscript;
                  struct SymbolVariableArgumentsWithSubscript; struct SymbolValue; }
namespace Paradata { class Event; class FieldInfo; }
namespace SpecialFunction { enum class Code : int; }


class ZENGINEO_API LogicInterpreter
{
public:
    LogicInterpreter(cs::non_null_shared_or_raw_ptr<EngineData> engine_data,
                     cs::non_null_shared_or_raw_ptr<ApplicationInterface> application_interface);
    virtual ~LogicInterpreter();


    // --------------------------------------------------------------------------
    // symbol table routines
    // --------------------------------------------------------------------------
public:
    Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

protected:
    Logic::SymbolTable& m_symbolTable;


    // --------------------------------------------------------------------------
    // execution flags
    // --------------------------------------------------------------------------

public: // INTERPRETER_DLL_TODO reevaluate if these should be public, and also don't use Hungarian notation
    CancelFlag m_bStopProc;


    // --------------------------------------------------------------------------
    // bytecode and node routines
    // (BytecodeRT.cpp)
    // --------------------------------------------------------------------------
public:
    template<typename NodeType>
    const NodeType& GetNode(int program_index) const;

    const Nodes::List& GetListNode(int program_index) const;
    const Nodes::List& GetOptionalListNode(int program_index) const;
    std::vector<int> GetListNodeContents(int program_index) const;

protected:
    const LogicByteCode& m_logicByteCode;


    // --------------------------------------------------------------------------
    // instruction routines
    // (InstructionsRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ExecuteInstruction(FunctionCode function_code, int program_index);
    double ExecuteInstruction(int program_index);

protected:
    using Instruction = std::variant<double (LogicInterpreter::*)(int), double (CIntDriver::*)(int)>;
    static Instruction m_instructions[];

    static size_t MaxInstructionCode_EV_TODO;
    double ex_unimplemented_LogicInterpreter(int program_index);


    // --------------------------------------------------------------------------
    // general evaluation routines
    // --------------------------------------------------------------------------
public:
    template<typename T>
    T Evaluate(int program_index);

    template<typename T>
    std::optional<T> EvaluateOptional(int program_index);

    template<typename T, typename DVT>
    T EvaluateOptional(int program_index, DVT&& default_value);

    template<typename T>
    T EvaluateOptionalOrConstruct(int program_index);

    bool EvaluateConditional(int program_index);
    bool EvaluateOptionalConditional(int program_index, bool default_value);
    std::optional<bool> EvaluateOptionalConditional(int program_index);

    template<typename ST = SharableString> // ST can also be std::string
    std::variant<double, ST> EvaluateVariant(DataType value_data_type, int program_index);


    // --------------------------------------------------------------------------
    // general assignment routines
    // --------------------------------------------------------------------------
public:
    template<typename T>
    static T GetInvalidValue();

    double AssignInvalidValue(DataType data_type);

    double AssignVariantValue(std::variant<double, SharableString>&& value);
    double AssignVariantValue(const std::variant<double, SharableString>& value);


    // --------------------------------------------------------------------------
    // message routines
    // --------------------------------------------------------------------------
public:
    // Issues the system message.
    template<typename... Args>
    void IssueMessage(MessageType message_type, int message_number, Args const&... args);

    // Returns a formatted system message.
    template<typename... Args>
    std::string GetFormattedMessage(int message_number, Args const&... args);

    // Returns an evaluated user message.
    virtual SharableString EvaluateUserMessage(int message_node_index, FunctionCode function_code, int* out_message_number = nullptr) = 0; // INTERPRETER_DLL_TODO remove as virtual

private:
    virtual void IssueMessageWorker(MessageType message_type, int message_number, ...) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual std::string GetFormattedMessageWorker(int message_number, ...) = 0; // INTERPRETER_DLL_TODO remove as virtual


    // --------------------------------------------------------------------------
    // general execution routines
    // (ExecutionRT.cpp)
    // --------------------------------------------------------------------------

public:
    // Returns a flag that indicates that the interpreter should stop execution.
    virtual bool IsExecutionInterrupted() const { return ReturnProgrammingError(false); } // INTERPRETER_DLL_TODO remove as virtual and replace this implementation

protected: // INTERPRETER_DLL_TODO change to private
    // Throws a previously-caught program control exception (if applicable).
    void RethrowProgramControlExceptions();

protected: // INTERPRETER_DLL_TODO change to private
    std::exception_ptr m_caughtProgramControlException;


    // --------------------------------------------------------------------------
    // numeric routines +
    // WorkVariable object functions +
    // math functions
    // (MathRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_numeric_constant(int program_index);

    double ex_WorkVariable_evaluate(int program_index);
    double ex_WorkVariable_compute(int program_index);

    double ex_add(int program_index);
    double ex_sub(int program_index);
    double ex_minus(int program_index);
    double ex_mult(int program_index);
    double ex_div(int program_index);
    double ex_mod(int program_index);
    double ex_exp(int program_index);
    double ex_eq(int program_index);
    double ex_ne(int program_index);
    double ex_le(int program_index);
    double ex_lt(int program_index);
    double ex_ge(int program_index);
    double ex_gt(int program_index);
    double ex_equ(int program_index);
    double ex_or(int program_index);
    double ex_not(int program_index);
    double ex_and(int program_index);
    double ex_abs(int program_index);
    double ex_ex(int program_index);
    double ex_inc(int program_index);
    double ex_int(int program_index);
    double ex_log(int program_index);
    double ex_low_high(int program_index);
    double ex_special(int program_index);
    double ex_sqrt(int program_index);
    double ex_round(int program_index);
    double ex_seed(int program_index);
    double ex_random(int program_index);
    double ex_tonumber(int program_index);

private:
    bool PreprocessSpecialValues(double& v1, double &v2, double& result) const;


    // --------------------------------------------------------------------------
    // string routines +
    // string escaping routines +
    // WorkString object functions +
    // string functions
    // (StringRT.cpp)
    // --------------------------------------------------------------------------
public:
    SharableString EvaluateSharableString(int program_index);
    SharableString EvaluateSharableString(DataType value_data_type, int program_index);
    SharableString EvaluateNullableSharableString(int program_index);
    std::string EvaluateString(int program_index);
    std::string EvaluateString(DataType value_data_type, int program_index);

    template<typename T>
    double AssignString(T&& value);

    double AssignStringNull();

    SharableString GetWorkingSharableString(size_t index);
    std::string GetWorkingString(size_t index);

    double ex_string_literal(int program_index);
    double ex_string_compute(int program_index);

    // If using the original logic settings, "\\n" characters will be converted to "\n" (or "\r\n"),
    // and optionally, "\\\\" characters will be converted to "\\"
    enum class V0_EscapeType { NewlinesToSlashN, NewlinesToSlashRN, NewlinesToSlashN_Backslashes, NewlinesToSlashRN_Backslashes };

    SharableString ConvertV0Escapes(SharableString text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);
    std::string ConvertV0Escapes(std::string text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);
    SharableString ApplyV0Escapes(SharableString text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);
    std::string ApplyV0Escapes(std::string text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);

    double ex_WorkString_evaluate(int program_index);
    double ex_WorkString_compute(int program_index);

    double ex_string_eq(int program_index);
    double ex_string_ne(int program_index);
    double ex_string_lt(int program_index);
    double ex_string_le(int program_index);
    double ex_string_ge(int program_index);
    double ex_string_gt(int program_index);
    double ex_compare(int program_index);
    double ex_compareNoCase(int program_index);
    double ex_concat(int program_index);
    double ex_ischecked(int program_index);
    double ex_length(int program_index);
    double ex_pos_poschar(int program_index);
    double ex_regexmatch(int program_index);
    double ex_replace(int program_index);
    double ex_startswith(int program_index);
    double ex_strip(int program_index);
    double ex_tolower_toupper(int program_index);
    double ex_decryptstring(int program_index);
    double ex_encode(int program_index);

private:
    template<TokenCode token_code>
    double ex_string_operators(int program_index);

private:
    // temporary strings created by logic functions
    std::vector<SharableString> m_workingStrings;

    // the encoding type for the encode function
    EncodeType m_currentEncodeType;


    // --------------------------------------------------------------------------
    // Action Invoker functions
    // (ActionInvokerRT.cpp)
    // --------------------------------------------------------------------------
public:
    // Sets the Action Invoker runtime.
    void SetActionInvokerRuntime(std::shared_ptr<ActionInvoker::Runtime> runtime);

    double ex_ActionInvoker(int program_index);

private:
    std::shared_ptr<ActionInvoker::Runtime> m_actionInvokerRuntime;
    std::unique_ptr<ActionInvoker::Caller> m_actionInvokerCaller;


    // --------------------------------------------------------------------------
    // Array object functions
    // (ArrayRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Array_var(int program_index);
    double ex_Array_compute(int program_index);
    double ex_Array_clear(int program_index);
    double ex_Array_length(int program_index);

protected: // INTERPRETER_DLL_TODO change to private
    double ex_Array_length(const LogicArray& logic_array, size_t dimension);

    // Returns the index, or an empty vector if the index is invalid.
    std::vector<size_t> EvaluateArrayIndex(int arrayvar_node_expression, LogicArray** out_logic_array);


    // --------------------------------------------------------------------------
    // Audio object functions
    // (AudioRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Audio_compute(int program_index);
    double ex_Audio_clear(int program_index);
    double ex_Audio_concat(int program_index);
    double ex_Audio_length(int program_index);
    double ex_Audio_load(int program_index);
    double ex_Audio_play(int program_index);
    double ex_Audio_save(int program_index);
    double ex_Audio_stop(int program_index);
    double ex_Audio_record(int program_index);
    double ex_Audio_recordInteractive(int program_index);


    // --------------------------------------------------------------------------
    // Barcode namespace functions
    // (BarcodeRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Barcode_read(int program_index);
    double ex_Barcode_createQRCode(int program_index);


    // --------------------------------------------------------------------------
    // compression and hash functions
    // (CompressionRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_compress(int program_index);
    double ex_decompress(int program_index);
    double ex_hash(int program_index);


    // --------------------------------------------------------------------------
    // date functions
    // (DateRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_timestamp(int program_index);
    double ex_timestring(int program_index);
    double ex_sysdate(int program_index);
    double ex_systime(int program_index);
    double ex_dateadd(int program_index);
    double ex_datediff(int program_index);
    double ex_datevalid(int program_index);
    double ex_cmcode(int program_index);
    double ex_setlb_setub(int program_index);
    double ex_adjlba(int program_index);
    double ex_adjuba(int program_index);
    double ex_adjlbi(int program_index);
    double ex_adjubi(int program_index);


    // --------------------------------------------------------------------------
    // Document object functions
    // (DocumentRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Document_compute(int program_index);
    double ex_Document_clear(int program_index);
    double ex_Document_load(int program_index);
    double ex_Document_save(int program_index);
    double ex_Document_view(int program_index);
    double ex_Document_view(const LogicDocument& logic_document, const ViewerOptions* viewer_options);


    // --------------------------------------------------------------------------
    // Geometry object functions
    // (GeometryRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Geometry_compute(int program_index);
    double ex_Geometry_clear(int program_index);
    double ex_Geometry_load(int program_index);
    double ex_Geometry_save(int program_index);
    double ex_Geometry_tracePolygon_walkPolygon(int program_index);
    double ex_Geometry_area_perimeter(int program_index);
    double ex_Geometry_minLatitude_maxLatitude_minLongitude_maxLongitude(int program_index);
    double ex_Geometry_getProperty(int program_index);
    double ex_Geometry_setProperty(int program_index);

private:
    bool EnsureGeometryExistsAndHasValidContent(const LogicGeometry& logic_geometry, const char* action_for_displayed_error_message);


    // --------------------------------------------------------------------------
    // HashMap object functions
    // (HashMapRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_HashMap_var(int program_index);
    double ex_HashMap_compute(int program_index);
    double ex_HashMap_clear(int program_index);
    double ex_HashMap_contains(int program_index);
    double ex_HashMap_getKeys(int program_index);
    double ex_HashMap_length(int program_index);
    double ex_HashMap_remove(int program_index);

private:
    // Returns the index, or an empty vector if the index is invalid.
    std::vector<std::variant<double, SharableString>> EvaluateHashMapIndex(const Nodes::List& dimension_expressions_node, int number_dimension_expressions);
    std::vector<std::variant<double, SharableString>> EvaluateHashMapIndex(const Nodes::List& dimension_expressions_node);
    std::vector<std::variant<double, SharableString>> EvaluateHashMapIndex(int hashmap_node_expression, LogicHashMap** out_hashmap, bool bounds_checking);


    // --------------------------------------------------------------------------
    // Image object functions
    // (ImageRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Image_compute(int program_index);
    double ex_Image_clear(int program_index);
    double ex_Image_getExif(int program_index);
    double ex_Image_load(int program_index);
    double ex_Image_resample(int program_index);
    double ex_Image_save(int program_index);
    double ex_Image_captureSignature_takePhoto(int program_index);
    double ex_Image_captureSignature_native(int program_index);
    double ex_Image_takePhoto_native(int program_index);
    double ex_Image_view(int program_index);
    double ex_Image_view(const LogicImage& logic_image, const ViewerOptions* viewer_options);
    double ex_Image_width_height(int program_index);


    // --------------------------------------------------------------------------
    // Item functions
    // (ItemRT.cpp)
    // --------------------------------------------------------------------------
public:
    // Evaluates the symbol reference and any associated subscript.
    // The subscript is not checked for validity.
    template<typename SymbolT = Symbol*>
    SymbolReference<SymbolT> EvaluateSymbolReference(int symbol_index, int subscript_compilation);

    // Returns a pointer (or shared pointer) to a symbol.
    // If the symbol's subscript is invalid, a runtime message appears and null is returned.
    // If the symbol is an item, the wrapped symbol (Document, Image, etc.,) is returned.
    template<typename SymbolT = Symbol*, typename SymbolReferenceT>
    auto GetFromSymbolOrEngineItem(const SymbolReference<SymbolReferenceT>& symbol_reference, bool use_exceptions = false);

    template<typename SymbolT = Symbol*>
    auto GetFromSymbolOrEngineItem(int symbol_index, int subscript_compilation, bool use_exceptions = false);

    // Evaluates the subscript if provided. Because a subscript is not needed for static functions,
    // if the subscript is invalid, a runtime message will appear, but the symbol will still be returned.
    template<typename SymbolT = Symbol>
    SymbolT& GetFromSymbolOrEngineItemForStaticFunction(int symbol_index, int subscript_compilation);

private:
    template<typename SymbolT>
    SymbolT EvaluateSymbolReference_GetSymbol(int symbol_index);

    const Nodes::SymbolVariableArgumentsWithSubscript& GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(int program_index);
    const Nodes::SymbolComputeWithSubscript& GetOrConvertPre80SymbolComputeWithSubscriptNode(int program_index);

private:
    std::map<int, std::unique_ptr<int[]>> m_convertedPre80Nodes;


    // --------------------------------------------------------------------------
    // JS (JavaScript) namespace functions
    // (JavaScriptRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_JavaScript_eval(int program_index);
    double ex_JavaScript_invoke(int program_index);
    double ex_JavaScript_hasValue(int program_index);
    double ex_JavaScript_getValueJson(int program_index);
    double ex_JavaScript_setValueFromJson(int program_index);
    double ex_JavaScript_getValue(int program_index);
    double ex_JavaScript_setValue(int program_index);
    double ex_JavaScript_UserFunctionCall(int program_index);

private:
    template<typename CF>
    auto ExecuteWithJavaScriptProcessor(const CF& callback_function);

    std::optional<JavaScript::Value> ConvertValueToJavaScript(EngineJavaScriptProcessor& javascript_processor,
                                                              int symbol_type_or_index, int expression_or_symbol_subscript_compilation);
    bool ConvertValueFromJavaScript(EngineJavaScriptProcessor& javascript_processor, const JavaScript::Value& js_value,
                                    int symbol_type_or_index, int expression_or_symbol_subscript_compilation,
                                    std::optional<SymbolType>& evaluated_symbol_type);


    // --------------------------------------------------------------------------
    // JSON-related functions
    // (JsonRT.cpp)
    // --------------------------------------------------------------------------
public:
    JsonReaderInterface* GetEngineJsonReaderInterface();
    std::string GetSymbolJson(const Symbol& symbol, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode* serialization_options_node);
    void SetSymbolValueFromJson(Symbol& symbol, const JsonNode& json_node);
    double ex_Symbol_getJson_getValueJson(int program_index);
    double ex_Symbol_setValueFromJson(int program_index);

private:
    std::unique_ptr<JsonReaderInterface> m_engineJsonReaderInterface;


    // --------------------------------------------------------------------------
    // List object functions
    // (ListRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_List_var(int program_index);
    double ex_List_compute(int program_index);
    double ex_List_add(int program_index);
    double ex_List_clear(int program_index);
    double ex_List_insert(int program_index);
    double ex_List_length(int program_index);
    double ex_List_remove(int program_index);
    double ex_List_removeDuplicates(int program_index);
    double ex_List_removeIn(int program_index);
    double ex_List_seek(int program_index);
    double ex_List_show(int program_index);
    double ex_List_show_pre77(int program_index);
    double ex_List_sort(int program_index);

private:
    // Returns the one-based index, or std::nullopt if the index is invalid.
    std::optional<size_t> EvaluateListIndex(int listvar_node_expression, LogicList** out_logic_list, bool for_assignment);


    // --------------------------------------------------------------------------
    // Localhost functions
    // (LocalhostRT.cpp)
    // --------------------------------------------------------------------------
public:
    // This can return a string (with the URL), or std::unique_ptr<VirtualFileMappingHandler>.
    // In the latter case, the calling function is in charge of the lifecycle of the virtual file.
    template<typename T = std::string>
    T LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::string> content_type_override = std::nullopt,
                                            bool evaluate_immediately = false);
private:
    std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_localHostVirtualFileMappingHandlers;


    // --------------------------------------------------------------------------
    // Map object functions
    // (MapRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Map_show(int program_index);
    double ex_Map_hide(int program_index);
    double ex_Map_addMarker(int program_index);
    double ex_Map_setMarkerImage(int program_index);
    double ex_Map_setMarkerText(int program_index);
    double ex_Map_setMarkerOnClick_setMarkerOnClickInfo(int program_index);
    double ex_Map_setMarkerDescription(int program_index);
    double ex_Map_setMarkerOnDrag(int program_index);
    double ex_Map_setMarkerLocation(int program_index);
    double ex_Map_getMarkerLatitude_getMarkerLongitude(int program_index);
    double ex_Map_removeMarker(int program_index);
    double ex_Map_setOnClick(int program_index);
    double ex_Map_showCurrentLocation(int program_index);
    double ex_Map_addTextButton(int program_index);
    double ex_Map_addImageButton(int program_index);
    double ex_Map_removeButton(int program_index);
    double ex_Map_setBaseMap(int program_index);
    double ex_Map_setTitle(int program_index);
    double ex_Map_zoomTo(int program_index);
    double ex_Map_clear_clearButtons_clearGeometry_clearMarkers(int program_index);
    double ex_Map_getLastClickLatitude_getLastClickLongitude(int program_index);
    double ex_Map_addGeometry(int program_index);
    double ex_Map_removeGeometry(int program_index);
    double ex_Map_saveSnapshot(int program_index);

private:
    template<typename T>
    bool SetBaseMap(LogicMap& logic_map, T base_map_selection);


    // --------------------------------------------------------------------------
    // network functions
    // (NetworkRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_connection(int program_index);


    // --------------------------------------------------------------------------
    // path functions
    // (PathRT.cpp)
    // --------------------------------------------------------------------------
public:
    const std::string& GetCurrentApplicationFilePath();
    const std::string& GetCurrentWorkingDirectory();

    std::string GetAbsolutePath(std::string path);
    void MakeAbsolutePath(std::string& path);
    std::string EvaluatePath(int program_index);
    SharableString EvaluatePathOrUrl(int program_index);

    void MakeAbsolutePath(ConnectionString& connection_string);
    ConnectionString EvaluateConnectionString(int program_index);

private:
    std::string m_currentWorkingDirectory;


    // --------------------------------------------------------------------------
    // Pff object functions
    // (Pff RT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Pff_compute(int program_index);
    double ex_Pff_load(int program_index);
    double ex_Pff_save(int program_index);
    double ex_Pff_getProperty(int program_index);
    double ex_Pff_setProperty(int program_index);
    double ex_Pff_exec(int program_index);


    // --------------------------------------------------------------------------
    // property functions
    // (PropertiesRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_diagnostics(int program_index);


    // --------------------------------------------------------------------------
    // Report object functions
    // (ReportRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Report_view(int program_index);
    double ex_Report_save(int program_index);

private:
    double ex_Report_view(Report& report, const ViewerOptions* viewer_options);
    std::unique_ptr<std::string> GenerateReport(Report& report, const std::string* output_file_path);


    // --------------------------------------------------------------------------
    // StringWriter object functions
    // (StringWriterRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_StringWriter_clear(int program_index);
    double ex_StringWriter_toString(int program_index);


    // --------------------------------------------------------------------------
    // "Switch" functions
    // (SwitchRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_recode(int program_index);
    double ex_when(int program_index);

    double ex_in(int program_index);
    double ex_randomin(int program_index);

private:
    std::optional<std::tuple<const int*, const int*>> EvaluateSwitchConditions(int program_index);

    bool InWorker(int in_node_expression, const std::variant<double, SharableString>& value,
                  const std::function<const std::variant<double, SharableString>&(int)>* expression_evaluator = nullptr);


    // --------------------------------------------------------------------------
    // Symbol functions
    // (SymbolRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Symbol_getLabel(int program_index);
    double ex_Symbol_getName(int program_index);


    // --------------------------------------------------------------------------
    // SystemApp object functions
    // (SystemAppRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_SystemApp_clear(int program_index);
    double ex_SystemApp_setArgument(int program_index);
    double ex_SystemApp_getResult(int program_index);
    double ex_SystemApp_exec(int program_index);


    // --------------------------------------------------------------------------
    // system functions
    // (SystemRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_getusername(int program_index);
    double ex_getos(int program_index);
    double ex_getdeviceid(int program_index);
    double ex_uuid(int program_index);
    double ex_sysparm(int program_index);
    double ex_savesetting(int program_index);
    double ex_loadsetting(int program_index);


    // --------------------------------------------------------------------------
    // Text Template functions
    // (TextTemplateRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_TextTemplate_write_writeEncoded_writeEncodedLine_writeLine(int program_index);

protected: // INTERPRETER_DLL_TODO change to private
    SharableString EncodeText(SharableString text, EncodeType encode_type);
    SharableString EncodeText(SharableString text, const Symbol& symbol);

    // If the symbol is a StringWriter, the underlying type (e.g., a Report) is returned.
    // The returned text builder is null if the text template is inaccessible.
    std::tuple<Symbol*, std::string*> GetTextTemplateBuilder(Symbol& symbol);


    // --------------------------------------------------------------------------
    // UserFunction object
    // (UserFunctionRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_UserFunction_compute(int program_index);


    // --------------------------------------------------------------------------
    // user interface functions
    // (UserInterfaceRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_view(int program_index);
    double ex_prompt(int program_index);
    double ex_accept(int program_index);
    double ex_htmldialog(int program_index);
    double ex_setfont(int program_index);

protected:
    std::optional<CSize> EvaluateSize(int width_program_index, int height_program_index);
    std::unique_ptr<ViewerOptions> EvaluateViewerOptions(const int viewer_options_node_program_index);


    // --------------------------------------------------------------------------
    // ValueSet object and value set-related functions
    // (ValueSetRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_minvalue_maxvalue(int program_index);
    double ex_invalueset(int program_index);
    double ex_getimage(int program_index);
    double ex_setvalueset(int program_index);
    double ex_setvalueset_pre80(int program_index);
    double ex_setvaluesets(int program_index);
    double ex_randomizevs(int program_index);

    double ex_ValueSet_compute(int program_index);
    double ex_ValueSet_add(int program_index);
    double ex_ValueSet_clear(int program_index);
    double ex_ValueSet_length(int program_index);
    double ex_ValueSet_remove(int program_index);
    double ex_ValueSet_removeDuplicates(int program_index);
    double ex_ValueSet_show(int program_index);
    double ex_ValueSet_show_pre77(int program_index);
    double ex_ValueSet_sort(int program_index);


    // --------------------------------------------------------------------------
    // "Variable" functions
    // (VariableRT.cpp)
    // --------------------------------------------------------------------------
public:
    template<typename T> bool AssignValueToSymbol(const Nodes::SymbolValue& symbol_value_node, T value);
    template<typename T> T EvaluateSymbolValue(const Nodes::SymbolValue& symbol_value_node);
    template<typename T> void ModifySymbolValue(const Nodes::SymbolValue& symbol_value_node, const std::function<void(T&)>& modify_value_function);


    // --------------------------------------------------------------------------
    // Video object functions
    // (VideoRT.cpp)
    // --------------------------------------------------------------------------
public:
    double ex_Video_compute(int program_index);
    double ex_Video_clear(int program_index);
    double ex_Video_length(int program_index);
    double ex_Video_load(int program_index);
    double ex_Video_save(int program_index);
    double ex_Video_width_height(int program_index);


    // --------------------------------------------------------------------------
    // other class variables
    // --------------------------------------------------------------------------
protected:
    cs::non_null_shared_or_raw_ptr<EngineData> m_engineData;
    cs::non_null_shared_or_raw_ptr<ApplicationInterface> m_applicationInterface;
    bool m_usingLogicSettingsV0;


    // --------------------------------------------------------------------------
    // INTERPRETER_DLL_TODO...
    // --------------------------------------------------------------------------
private:
    virtual double evalexpr_INTERPRETER_DLL_TODO(double (CIntDriver::*instruction)(int), int program_index) = 0;
    virtual void RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::shared_ptr<Paradata::Event> event, const void* instance_object = nullptr) = 0;
    virtual SharableString EvaluateTextFill(int program_index) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual bool Report_Evaluate_INTERPRETER_DLL_TODO(Report& report) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double RunSoonToBeRemoveFeature(std::string_view feature_sv, int program_index, void* tag) = 0;
    virtual bool HasSpecialFunction(SpecialFunction::Code special_function) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double ExecSpecialFunction(int symbol_index, SpecialFunction::Code special_function, std::vector<std::variant<double, SharableString>> arguments) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual int Get_m_iExSymbol_INTERPRETER_DLL_TODO() = 0;
    virtual Symbol* GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<Symbol*>& symbol_reference, bool use_exceptions) = 0;
    virtual std::shared_ptr<Symbol> GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<std::shared_ptr<Symbol>>& symbol_reference, bool use_exceptions) = 0;
    virtual EvaluatedEngineItemSubscript EvaluateEngineItemSubscript(const EngineItem& engine_item, const Nodes::ItemSubscript& item_subscript_node) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual bool IsDataAccessible(const Symbol& symbol, bool issue_error_if_inaccessible) = 0;
    virtual int SelectDlgHelper_pre77(int iFunCode, const CString& csHeading, const std::vector<std::vector<CString>*>* paData,
                                      const std::vector<CString>* paColumnTitles, std::vector<bool>* pbaSelections,
                                      const std::vector<PortableColor>* row_text_colors) = 0;
    virtual EngineParadataDriver& GetEngineParadataDriver_INTERPRETER_DLL_TODO() = 0;
    virtual bool ExecuteProgramStatements(int program_index) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual void ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& argument_evaluator) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual std::unique_ptr<UserFunctionArgumentEvaluator> EvaluateArgumentsForCallbackUserFunction(int program_index, FunctionCode function_code) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double ex_Freq_view(const NamedFrequency& named_frequency, const ViewerOptions* viewer_options, int frequency_parameters_node_index) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double exCase_view(const CSymbolDict& dictionary, const ViewerOptions* viewer_options) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double ExExecPFF_INTERPRETER_DLL_TODO(LogicPff& logic_pff) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual FrequencyDriver* GetFrequencyDriver_INTERPRETER_DLL_TODO() = 0;
    virtual void AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, double value) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual void AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, SharableString value) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual double EvaluateVARTValue_double_INTERPRETER_DLL_TODO(int variable_compilation) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual SharableString EvaluateVARTValue_SharableString_INTERPRETER_DLL_TODO(int variable_compilation) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual void ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(double&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr) = 0; // INTERPRETER_DLL_TODO remove as virtual
    virtual void ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(SharableString&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr) = 0; // INTERPRETER_DLL_TODO remove as virtual
    int SymbolTableSearchWithPreference_INTERPRETER_DLL_TODO(std::string_view full_symbol_name_sv, SymbolType preferred_symbol_type) const { return SymbolTableSearch_INTERPRETER_DLL_TODO(full_symbol_name_sv, preferred_symbol_type, nullptr); }
    int SymbolTableSearch_INTERPRETER_DLL_TODO(std::string_view full_symbol_name_sv, const std::vector<SymbolType>& allowable_symbol_types, SymbolType preferred_symbol_type = SymbolType::None) const { return SymbolTableSearch_INTERPRETER_DLL_TODO(full_symbol_name_sv, preferred_symbol_type, &allowable_symbol_types); }
    virtual int SymbolTableSearch_INTERPRETER_DLL_TODO(std::string_view full_symbol_name_sv, SymbolType preferred_symbol_type,
                                                       const std::vector<SymbolType>* allowable_symbol_types) const = 0; // INTERPRETER_DLL_TODO refactor
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename NodeType>
const NodeType& LogicInterpreter::GetNode(const int program_index) const
{
    return *reinterpret_cast<const NodeType*>(m_logicByteCode.GetCodeAtPosition(program_index));
}


inline double LogicInterpreter::ExecuteInstruction(const FunctionCode function_code, const int program_index)
{
    const Instruction& instruction = m_instructions[static_cast<size_t>(function_code)];
    ASSERT(instruction.index() == 1 || std::get<0>(instruction) != nullptr);

    return ( instruction.index() == 0 ) ? (this->*(std::get<0>(instruction)))(program_index) :
                                          evalexpr_INTERPRETER_DLL_TODO(std::get<1>(instruction), program_index);
}


inline double LogicInterpreter::ExecuteInstruction(const int program_index)
{
    const int* const function_code_ptr = m_logicByteCode.GetCodeAtPosition(program_index);
    return ExecuteInstruction(static_cast<FunctionCode>(*function_code_ptr), program_index);
}


template<typename T>
T LogicInterpreter::Evaluate(const int program_index)
{
    if      constexpr(std::is_same_v<T, SharableString>) { return EvaluateSharableString(program_index); }
    else if constexpr(std::is_same_v<T, std::string>)    { return EvaluateString(program_index); }
    else                                                 { return static_cast<T>(ExecuteInstruction(program_index)); }
}


template<typename T>
std::optional<T> LogicInterpreter::EvaluateOptional(const int program_index)
{
    if( program_index != -1 )
        return Evaluate<T>(program_index);

    return std::nullopt;
}


template<typename T, typename DVT>
T LogicInterpreter::EvaluateOptional(int program_index, DVT&& default_value)
{
    if( program_index != -1 )
        return Evaluate<T>(program_index);

    return std::forward<DVT>(default_value);
}


template<typename T>
T LogicInterpreter::EvaluateOptionalOrConstruct(const int program_index)
{
    if( program_index != -1 )
        return Evaluate<T>(program_index);

    return T();
}


template<typename... Args>
void LogicInterpreter::IssueMessage(const MessageType message_type, const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes<char>(args...);
#endif

    IssueMessageWorker(message_type, message_number, args...);
}


template<typename... Args>
std::string LogicInterpreter::GetFormattedMessage(const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes<char>(args...);
#endif

    return GetFormattedMessageWorker(message_number, args...);
}


template<typename T>
double LogicInterpreter::AssignString(T&& value)
{
    if constexpr(cs::is_optional<T>::value)
    {
        return value.has_value() ? AssignString(std::move(*value)) :
                                   AssignStringNull();
    }

    else
    {
        m_workingStrings.emplace_back(std::forward<T>(value));
        return static_cast<double>(m_workingStrings.size() - 1);
    }
}


template<typename SymbolT /*= Symbol* */, typename SymbolReferenceT>
auto LogicInterpreter::GetFromSymbolOrEngineItem(const SymbolReference<SymbolReferenceT>& symbol_reference, const bool use_exceptions/* = false*/)
{
    if constexpr(std::is_same_v<SymbolT, std::shared_ptr<Symbol>>)
    {
        // INTERPRETER_DLL_TODO restore the original version:
        // return GetFromSymbolOrEngineItemWorker<SymbolReferenceT>(symbol_reference, use_exceptions);
        return GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(symbol_reference, use_exceptions);
    }

    else
    {
        // INTERPRETER_DLL_TODO restore the original version:
        // return assert_nullable_cast<SymbolT>(GetFromSymbolOrEngineItemWorker<SymbolReferenceT>(symbol_reference, use_exceptions));
        return assert_nullable_cast<SymbolT>(GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(symbol_reference, use_exceptions));
    }
}


template<typename SymbolT /*= Symbol* */>
auto LogicInterpreter::GetFromSymbolOrEngineItem(const int symbol_index, const int subscript_compilation, const bool use_exceptions/* = false*/)
{
    using SymbolReferenceT = typename std::conditional<std::is_same_v<SymbolT, std::shared_ptr<Symbol>>, SymbolT, Symbol*>::type;
    return GetFromSymbolOrEngineItem<SymbolT, SymbolReferenceT>(EvaluateSymbolReference<SymbolReferenceT>(symbol_index, subscript_compilation), use_exceptions);
}


template<typename SymbolT /* = Symbol */>
SymbolT& LogicInterpreter::GetFromSymbolOrEngineItemForStaticFunction(const int symbol_index, const int subscript_compilation)
{
    // this function is for the "this" symbol of a static dot-notation function;
    // even though the subscript does not need to be evaluated to run the static function, it will be evaluated:
    //     - for functions that use FunctionDetails::StaticType::StaticWhenNecessary
    //     - so that something like SEX(MyFunc()).getLabel() results in MyFunc() being called, which people would expect
    Symbol* symbol = nullptr;

    if( subscript_compilation != -1 )
    {
        try
        {
            symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_index, subscript_compilation, true);
        }
        catch(...) { }
    }

    if( symbol == nullptr )
        symbol = &NPT_Ref(symbol_index);

    return assert_cast<SymbolT&>(*symbol);
}
