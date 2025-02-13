#include "stdafx.h"
#include "IncludesCC.h"
#include "AllSymbols.h"
#include "CommonStoreCompilerHelper.h"
#include "PersistentVariableProcessor.h"
#include "PreinitializedVariable.h"
#include "Nodes/Encryption.h"
#include <zLogicO/KeywordTable.h>


// --------------------------------------------------------------------------
// a routine for compiling symbol names, which will allow the option of
// nameless symbol declarations when compiling function pointers
// --------------------------------------------------------------------------

std::wstring LogicCompiler::CompileNewSymbolName()
{
    // if no symbol name compiler exists, add the default one
    if( m_symbolCompilerModifier.name_compiler.empty() )
    {
        m_symbolCompilerModifier.name_compiler.push(
            [&]()
            {
                NextTokenOrNewSymbolName();

                if( Tkn != TOKNEWSYMBOL )
                {
                    // if not a new symbol, run the token through the name checks
                    // so that a more accurate message occurs is issued...
                    CheckIfValidNewSymbolName(Tokstr);

                    // ...or default to a generic error
                    IssueError(MGF::symbol_name_in_use_102, Tokstr.c_str());
                }

                return Tokstr;
            });
    }

    return (m_symbolCompilerModifier.name_compiler.top())();
}



// --------------------------------------------------------------------------
// routines for keeping track of modifiers used when declaring symbols
// and processing the modifiers
// --------------------------------------------------------------------------

int LogicCompiler::CompileSymbolWithModifiers()
{
    static const std::vector<TokenCode> ConfigTokens =
    {
        TOKNUMERIC,
        TOKSTRING,
    };

    static const std::vector<TokenCode> PersistentTokens =
    {
        TOKNUMERIC,
        TOKSTRING, TOKALPHA,
        TOKKWARRAY, 
        TOKKWAUDIO,
        TOKKWDOCUMENT,
        TOKKWGEOMETRY,
        TOKKWHASHMAP,
        TOKKWIMAGE,
        TOKKWLIST,
        TOKKWSYSTEMAPP,
        TOKKWVALUESET,
    };

    std::vector<RAII::SetValueAndRestoreOnDestruction<bool>> raii_modifiers;

    // read all modifiers
    while( Tkn == TOKCONFIG || Tkn == TOKPERSISTENT )
    {
        bool& compilation_flag = ( Tkn == TOKCONFIG ) ? m_symbolCompilerModifier.config_variable :
                                                        m_symbolCompilerModifier.persistent_variable;

        // multiple modifiers of the same type are not allowed
        if( compilation_flag )
            IssueError(MGF::variable_modifier_duplicated_94100, Logic::KeywordTable::GetKeywordName(Tkn));

        raii_modifiers.emplace_back(compilation_flag, true);

        // prior to 8.0, config automatically meant a string variable, so allow that but with a deprecation warning
        if( m_symbolCompilerModifier.config_variable && raii_modifiers.size() == 1 )
        {
            const Logic::BasicToken* next_basic_token = PeekNextBasicToken();

            if( next_basic_token != nullptr && !Logic::KeywordTable::IsKeyword(next_basic_token->GetTextSV()) )
            {
                IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_config_without_type_95030);
                return CompileLogicStrings();
            }
        }

        NextToken();
    }

    ASSERT(!raii_modifiers.empty());

    // check that the modifier is valid for the token
    auto check_modifier_validity = [&](bool compilation_flag, TokenCode modifier_token, const std::vector<TokenCode>& valid_tokens)
    {
        if( compilation_flag && std::find(valid_tokens.cbegin(), valid_tokens.cend(), Tkn) == valid_tokens.cend() )
            IssueError(MGF::variable_modifier_invalid_94101, Logic::KeywordTable::GetKeywordName(modifier_token), Logic::KeywordTable::GetKeywordName(Tkn));
    };

    check_modifier_validity(m_symbolCompilerModifier.config_variable, TOKCONFIG, ConfigTokens);
    check_modifier_validity(m_symbolCompilerModifier.persistent_variable, TOKPERSISTENT, PersistentTokens);


    // compile the symbol declaration
    size_t initial_number_symbols = m_symbolTable.GetTableSize();
    int program_index = CompileSymbolRouter();


    // config
    // ------
    // the processing of config variables is done in the symbol's compilation function


    // persistent
    // ----------
    if( m_symbolCompilerModifier.persistent_variable )
    {
        PersistentVariableProcessor& persistent_variable_processor = m_engineData->runtime_events_processor.GetOrCreateEvent<PersistentVariableProcessor>();

        for( size_t i = initial_number_symbols; i < m_symbolTable.GetTableSize(); ++i )
        {
            Symbol& symbol = NPT_Ref(i);

            // prevent the use of multiple persistent variables with the same name
            if( persistent_variable_processor.HasSymbolWithSameName(symbol) )
                IssueError(MGF::variable_modifier_persistent_duplicate_names_94105, symbol.GetName().c_str());

            persistent_variable_processor.AddSymbol(symbol);
        }
    }

    return program_index;
}



// --------------------------------------------------------------------------
// route the declaration to the proper symbol compilation routine
// --------------------------------------------------------------------------

int LogicCompiler::CompileSymbolRouter()
{
    using CompilationFunction = int(LogicCompiler::*)();

    static const std::map<TokenCode, CompilationFunction> SymbolCompilationFunctionMap =
    {
        { TOKNUMERIC,       &LogicCompiler::CompileWorkVariables },
        { TOKALPHA,         &LogicCompiler::CompileLogicStrings },
        { TOKSTRING,        &LogicCompiler::CompileLogicStrings },

        { TOKKWARRAY,       &LogicCompiler::CompileLogicArrayDeclaration },
        { TOKKWAUDIO,       &LogicCompiler::CompileLogicAudioDeclarations },
        { TOKKWCASE,        &LogicCompiler::CompileEngineCases },
        { TOKKWDATASOURCE,  &LogicCompiler::CompileEngineDataRepositories },
        { TOKKWDOCUMENT,    &LogicCompiler::CompileLogicDocumentDeclarations },
        { TOKKWFILE,        &LogicCompiler::CompileLogicFiles },
        { TOKKWFREQ,        &LogicCompiler::CompileFrequencyDeclaration },
        { TOKKWGEOMETRY,    &LogicCompiler::CompileLogicGeometryDeclarations },
        { TOKKWHASHMAP,     &LogicCompiler::CompileLogicHashMapDeclarations },
        { TOKKWIMAGE,       &LogicCompiler::CompileLogicImageDeclarations },
        { TOKKWLIST,        &LogicCompiler::CompileLogicListDeclarations },
        { TOKKWMAP,         &LogicCompiler::CompileLogicMapDeclarations },
        { TOKKWPFF,         &LogicCompiler::CompileLogicPffDeclarations },
        { TOKKWSYSTEMAPP,   &LogicCompiler::CompileSystemAppDeclarations },
        { TOKKWVALUESET,    &LogicCompiler::CompileDynamicValueSetDeclarations },
    };

    const auto& compilation_function_lookup = SymbolCompilationFunctionMap.find(Tkn);

    // invalid declaration
    if( compilation_function_lookup == SymbolCompilationFunctionMap.cend() )
        IssueError(MGF::declaration_invalid_92);

    return (this->*compilation_function_lookup->second)();
}



// --------------------------------------------------------------------------
// some helper routines
// --------------------------------------------------------------------------

int& LogicCompiler::AddSymbolResetNode(Nodes::SymbolReset*& symbol_reset_node, const Symbol& symbol)
{
    // symbol reset nodes are not used for global variables
    if( IsGlobalCompilation() )
    {
        static int dummy_initialize_value;
        dummy_initialize_value = -1;
        return dummy_initialize_value;
    }

    // create the symbol reset node
    FunctionCode function_code = m_symbolCompilerModifier.persistent_variable ? FunctionCode::PERSISTENT_SYMBOL_RESET_CODE :
                                                                                FunctionCode::SYMBOL_RESET_CODE;

    auto& new_symbol_reset_node = CreateNode<Nodes::SymbolReset>(function_code);

    new_symbol_reset_node.next_st = -1;
    new_symbol_reset_node.symbol_index = symbol.GetSymbolIndex();
    new_symbol_reset_node.initialize_value = -1;

    // link any symbol reset nodes already added as part of this declaration
    if( symbol_reset_node != nullptr )
        symbol_reset_node->next_st = GetProgramIndex(new_symbol_reset_node);

    symbol_reset_node = &new_symbol_reset_node;

    return new_symbol_reset_node.initialize_value;
}


unsigned LogicCompiler::CompileAlphaLength()
{
    constexpr unsigned DefaultAlphaLength = 16;
    constexpr unsigned MaxAlphaLength     = 8192;

    // if no length is specified, issue a warning and return the default length
    if( !NextKeywordIf(TOKLPAREN) )
    {
        IssueWarning(MGF::alpha_specificy_length_8203, static_cast<int>(DefaultAlphaLength));
        return DefaultAlphaLength;
    }

    // otherwise the length must be a positive integer (no greater than 8192) specified in parentheses
    NextToken();

    if( Tkn == TOKCTE && IsNumericConstantInteger() )
    {
        unsigned string_length = (unsigned)Tokvalue;

        if( string_length > 0 && string_length <= MaxAlphaLength )
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_19);

            // a deprecation warning will appear with valid alpha variables
            IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, MGF::deprecation_use_string_not_alpha_95004);

            return string_length;
        }
    }

    // an invalid length specification
    IssueError(MGF::alpha_length_too_long_8204, static_cast<int>(MaxAlphaLength));
}


int LogicCompiler::CompileSymbolInitialAssignment(const Symbol& symbol)
{
    bool numeric = IsNumeric(symbol);
    size_t exact_string_length = 0;

    if( symbol.IsA(SymbolType::WorkString) )
    {
        if( symbol.GetSubType() == SymbolSubType::WorkAlpha )
            exact_string_length = assert_cast<const WorkAlpha&>(symbol).GetLength();
    }

    else if( symbol.IsA(SymbolType::Array) )
    {
        exact_string_length = assert_cast<const LogicArray&>(symbol).GetPaddingStringLength();
    }

    else
    {
        ASSERT(symbol.IsOneOf(SymbolType::List,
                              SymbolType::WorkVariable));
    }

    if( IsGlobalCompilation() )
    {
        int conserver_index = 0;

        // numeric variables (in PROC GLOBAL)
        if( numeric )
        {
            bool negative = false;

            if( Tkn == TOKMINUS )
            {
                negative = true;
                NextToken();
            }

            // only constants can be used in PROC GLOBAL
            IssueErrorOnTokenMismatch(TOKCTE, MGF::numeric_constant_expected_8217);

            double value = ( negative && !IsSpecial(Tokvalue) ) ? ( -1 * Tokvalue ) :
                                                                  Tokvalue;

            conserver_index = ConserveConstant(value);
        }

        // string variables (in PROC GLOBAL)
        else
        {
            // only string literals are allowed
            IssueErrorOnTokenMismatch(TOKSCTE, MGF::string_literal_expected_8218);

            std::wstring value = Tokstr;

            // ensure that the value is the correct length for alpha variables
            if( exact_string_length != 0 )
            {
                if( value.size() > exact_string_length )
                    IssueWarning(MGF::string_literal_will_be_truncated_8216);

                SO::MakeExactLength(value, exact_string_length);
            }

            conserver_index = ConserveConstant(std::move(value));
        }

        // read the next token (to match what happens in the below functions)
        NextToken();

        return conserver_index;
    }

    // if not in PROC GLOBAL, use the standard compilation functions
    else
    {
        return CompileExpression(numeric ? DataType::Numeric : DataType::String);
    }
}



// --------------------------------------------------------------------------
//  Working Variables (numeric)
// --------------------------------------------------------------------------

WorkVariable* LogicCompiler::CompileWorkVariableDeclaration()
{
    std::wstring numeric_name = CompileNewSymbolName();

    auto work_variable = std::make_shared<WorkVariable>(std::move(numeric_name));

    m_engineData->AddSymbol(work_variable);

    return work_variable.get();
}


int LogicCompiler::CompileWorkVariables()
{
    ASSERT(Tkn == TOKNUMERIC);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        WorkVariable* work_variable = CompileWorkVariableDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *work_variable);

        NextToken();

        // allow direct variable assignment
        if( Tkn == TOKEQOP )
        {
            NextToken();

            initialize_value = CompileSymbolInitialAssignment(*work_variable);
        }


        // if a config variable, the value can come from common store
        if( m_symbolCompilerModifier.config_variable )
        {
            bool value_already_provided = ( initialize_value != -1 );

            std::optional<double> config_value =
                GetCompilerHelper<CommonStoreCompilerHelper>().GetConfigValue<double>(
                work_variable->GetName(), !value_already_provided);

            // default to NOTAPPL if there is no value
            if( !config_value.has_value() && !value_already_provided )
                config_value = NOTAPPL;

            if( config_value.has_value() )
            {
                initialize_value = IsGlobalCompilation() ? ConserveConstant(*config_value) :
                                                           CreateNumericConstantNode(*config_value);
            }
        }

        // if in PROC GLOBAL, add to the preinitialized variables
        if( IsGlobalCompilation() && initialize_value >= 0 )
        {
            // actually set the value (so that it can be used to define the dimensions of an array)
            work_variable->SetValue(GetNumericConstant(initialize_value));

            m_engineData->runtime_events_processor.AddEvent(std::make_unique<PreinitializedVariable>(
                *m_engineData,
                *work_variable,
                std::vector { initialize_value }));
        }

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}



// --------------------------------------------------------------------------
// Logic Strings (string, alpha)
// --------------------------------------------------------------------------

WorkString* LogicCompiler::CompileLogicStringDeclaration(TokenCode token_code, const WorkString* work_string_to_copy_attributes/* = nullptr*/)
{
    std::optional<unsigned> string_length;

    // the string length can be specified for the first alpha variable
    if( token_code == TOKALPHA )
    {
        if( work_string_to_copy_attributes != nullptr )
        {
            string_length = assert_cast<const WorkAlpha*>(work_string_to_copy_attributes)->GetLength();
        }

        else
        {
            string_length = CompileAlphaLength();
        }
    }

    // add the variable
    std::wstring string_name = CompileNewSymbolName();

    std::shared_ptr<WorkString> work_string;

    if( string_length.has_value() )
    {
        auto work_alpha = std::make_unique<WorkAlpha>(std::move(string_name));
        work_alpha->SetLength(*string_length);
        work_string = std::move(work_alpha);
    }

    else
    {
        work_string = std::make_shared<WorkString>(std::move(string_name));
    }

    m_engineData->AddSymbol(work_string);

    return work_string.get();
}


int LogicCompiler::CompileLogicStrings()
{
    ASSERT(Tkn == TOKALPHA || Tkn == TOKSTRING || m_symbolCompilerModifier.config_variable);
    TokenCode token_code = m_symbolCompilerModifier.config_variable ? TOKSTRING : Tkn;
    Nodes::SymbolReset* symbol_reset_node = nullptr;
    WorkString* last_work_string_compiled = nullptr;

    do
    {
        last_work_string_compiled = CompileLogicStringDeclaration(token_code, last_work_string_compiled);

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *last_work_string_compiled);

        NextToken();

        // allow direct variable assignment
        if( Tkn == TOKEQOP )
        {
            NextToken();

            initialize_value = CompileSymbolInitialAssignment(*last_work_string_compiled);
        }


        // if a config variable, get the value from the common store
        bool value_processed = false;

        if( m_symbolCompilerModifier.config_variable )
        {
            bool value_already_provided = ( initialize_value != -1 );

            std::optional<std::wstring> config_value =
                GetCompilerHelper<CommonStoreCompilerHelper>().GetConfigValue<std::wstring>(
                last_work_string_compiled->GetName(), !value_already_provided);

            if( config_value.has_value() )
            {
                // encrypt the value
                const Encryptor::Type encryption_type = PreinitializedVariable::GetEncryptionType();
                std::wstring encrypted_value = Encryptor(encryption_type).Encrypt(*config_value);

                if( IsGlobalCompilation() )
                {
                    initialize_value = ConserveConstant(std::move(encrypted_value));
                    m_engineData->runtime_events_processor.AddEvent(std::make_unique<PreinitializedVariable>(
                        *m_engineData,
                        *last_work_string_compiled,
                        std::vector { initialize_value },
                        PreinitializedVariable::SpecialProcessing::EncryptedString));
                }

                // config variables (not in PROC GLOBAL) need to point to a encryption node to decrypt the value
                else
                {
                    auto& encryption_node = CreateNode<Nodes::Encryption>(FunctionCode::DECRYPT_STRING_CODE);

                    encryption_node.string_expression = CreateStringLiteralNode(std::move(encrypted_value));
                    encryption_node.encryption_type = encryption_type;

                    initialize_value = GetProgramIndex(encryption_node);
                }

                value_processed = true;
            }
        }

        // for non-config variables (or config variables with a direct assignment that was not overriden),
		// add to the preinitialized variables if in PROC GLOBAL
        if( IsGlobalCompilation() && initialize_value >= 0 && !value_processed )
        {
            m_engineData->runtime_events_processor.AddEvent(std::make_unique<PreinitializedVariable>(
                *m_engineData,
                *last_work_string_compiled,
                std::vector { initialize_value }));
        }

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}



// --------------------------------------------------------------------------
// Aliases (alias)
// --------------------------------------------------------------------------

void LogicCompiler::CompileAlias()
{
    ASSERT(Tkn == TOKALIAS);
    ASSERT(IsGlobalCompilation()); // aliases are only allowed in PROC GLOBAL

    do
    {
        std::wstring alias_name = CompileNewSymbolName();

        NextToken();
        IssueErrorOnTokenMismatch(TOKCOLON, MGF::alias_invalid_8206);

        // reset Tokstindex
        Tokstindex = -1;

        NextToken();

        if( Tokstindex < 1 ) // invalid symbol
            IssueError(MGF::alias_invalid_8206);

        // add the alias
        m_symbolTable.AddAlias(std::move(alias_name), NPT_Ref(Tokstindex));

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);
}



// --------------------------------------------------------------------------
// Ensure (ensure)
// --------------------------------------------------------------------------

void LogicCompiler::CompileEnsure()
{
    ASSERT(Tkn == TOKENSURE);
    ASSERT(IsGlobalCompilation()); // ensure variables are only allowed in PROC GLOBAL

    NextTokenOrNewSymbolName();
    IssueErrorOnTokenMismatch(TOKNUMERIC, MGF::ensure_type_error_8207);

    while( true )
    {
        NextTokenOrNewSymbolName();

        // add a new variable
        if( Tkn == TOKNEWSYMBOL )
        {
            m_engineData->AddSymbol(std::make_unique<WorkVariable>(Tokstr));
        }

        // or check that the item is a valid numeric item
        else if( Tkn != TOKVAR || !IsNumeric(NPT_Ref(Tokstindex)) )
        {
            IssueError(MGF::ensure_type_error_8207);
        }

        NextToken();

        // given an error if they try to initialize the ensure variable
        if( Tkn == TOKEQOP )
        {
            IssueError(MGF::ensure_assignment_invalid_8208);
        }

        else if( Tkn == TOKCOMMA )
        {
            continue;
        }

        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);
        return;
    }
}



// --------------------------------------------------------------------------
// Symbol functions
// --------------------------------------------------------------------------

int LogicCompiler::CompileSymbolFunctions()
{
    // compiling:
    //      [all symbols].getLabel([language := s])
    //      [all symbols].getJson([serializationOptions := s]);
    //      [all symbols].getName()
    //      [some symbols].getValueJson([serializationOptions := s]);
    //      [some symbols].updateValueFromJson(json);
    ASSERT(CurrentToken.symbol != nullptr);

    const Logic::FunctionDetails* function_details = CurrentToken.function_details;
    const Symbol& symbol = *CurrentToken.symbol;
    const int symbol_subscript_compilation = CurrentToken.symbol_subscript_compilation;
    const bool symbol_subscript_used = ( symbol_subscript_compilation != -1 );

    // read the left parenthesis but nothing more (in case any compilers use named arguments as the first argument)
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    Nodes::SymbolVariableArgumentsWithSubscript* symbol_va_with_subscript_node = nullptr;

    auto instantiate_node = [&]()
    {
        if( symbol_va_with_subscript_node == nullptr )
            symbol_va_with_subscript_node = &CreateSymbolVariableArgumentsWithSubscriptNode(function_details->code, symbol, symbol_subscript_compilation, function_details->number_arguments);
    };


    // [all symbols].getName()
    // [all symbols].getLabel([language := s])
    // --------------------------------------------------------------------------
    if( function_details->code == FunctionCode::SYMBOLFN_GETNAME_CODE ||
        function_details->code == FunctionCode::SYMBOLFN_GETLABEL_CODE )
    {
        // we can calculate the name/label immediately if:
        // - a subscript has not been used
        // - this is not a function parameter
        // - a language has not been specified (for getLabel)
        // - multiple languages don't exist (for getLabel)
        bool can_evaluate_immediately = ( !symbol_subscript_used && !IsFunctionParameterSymbol(symbol) );

        if( function_details->code == FunctionCode::SYMBOLFN_GETNAME_CODE )
        {
            NextToken();

            if( can_evaluate_immediately )
            {
                IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

                NextToken();

                return CreateStringLiteralNode(symbol.GetName());
            }

            instantiate_node();
        }

        // for getLabel, evaluate the optional language named argument
        else
        {
            OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
            int language_expression = -1;
            optional_named_arguments_compiler.AddArgument(_T("language"), language_expression, DataType::String);

            if( optional_named_arguments_compiler.Compile(true) != 0 )
            {
                can_evaluate_immediately = false;
            }

            else
            {
                NextToken();

                if( can_evaluate_immediately )
                    can_evaluate_immediately = !SymbolCalculator::DoMultipleLabelsExist(symbol);
            }

            if( can_evaluate_immediately )
            {
                IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

                NextToken();

                return CreateStringLiteralNode(SymbolCalculator::GetLabel(symbol));
            }

            instantiate_node();
            symbol_va_with_subscript_node->arguments[0] = language_expression;
        }
    }


    // [some symbols].updateValueFromJson(json);
    // --------------------------------------------------------------------------
    else if( function_details->code == FunctionCode::SYMBOLFN_UPDATEVALUEFROMJSON_CODE )
    {
        instantiate_node();

        NextToken();
        symbol_va_with_subscript_node->arguments[0] = CompileJsonText();
    }


    // [all symbols].getJson([serializationOptions := s]);
    // [some symbols].getValueJson([serializationOptions := s]);
    // --------------------------------------------------------------------------
    else
    {
        ASSERT(function_details->code == FunctionCode::SYMBOLFN_GETJSON_CODE ||
               function_details->code == FunctionCode::SYMBOLFN_GETVALUEJSON_CODE);

        instantiate_node();
        symbol_va_with_subscript_node->arguments[0] = -1;

        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

        optional_named_arguments_compiler.AddArgumentJsonText(JK::serializationOptions, symbol_va_with_subscript_node->arguments[0],
            [&](const JsonNode<wchar_t>& json_node)
            {
                JsonProperties::CreateFromJson(json_node);
            });

        if( optional_named_arguments_compiler.Compile(true) == 0 )
            NextToken();
    }


    ASSERT(symbol_va_with_subscript_node != nullptr);

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(*symbol_va_with_subscript_node);
}
