#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineItem.h"
#include <engine/VarT.h>
#include <zLogicO/BaseCompilerSettings.h>
#include <zLogicO/FunctionTable.h>


void LogicCompiler::ProcessSymbol()
{
    BaseCompiler::ProcessSymbol();

    Symbol* symbol = CurrentToken.symbol;
    ASSERT(symbol != nullptr);

    Tokstindex = symbol->GetSymbolIndex();

    // special processing for items
    ASSERT(symbol->IsA(SymbolType::Item) || symbol->GetWrappedType() == SymbolType::None);

    if( symbol->IsA(SymbolType::Item) )
        ProcessSymbolEngineItem(assert_cast<EngineItem&>(*symbol));
}


void LogicCompiler::ProcessSymbolEngineItem(EngineItem& engine_item)
{
    ASSERT(CurrentToken.symbol == &engine_item && CurrentToken.symbol_subscript_compilation == -1);

    const bool subscript_supplied_explicity = NextKeywordIf(TOKLPAREN);
    int symbol_subscript_compilation;
    bool process_dot_notation_function;

    // when a subscript is supplied explicitly, we will compile it
    if( subscript_supplied_explicity )
    {
        symbol_subscript_compilation = CompileItemSubscriptExplicit(engine_item);

        process_dot_notation_function = ( Tkn == TOKPERIOD );
    }

    // otherwise we will compile an implicit subscript (if not a dot-notation function call);
    // if a dot-notation function call, we will compile the implicit subscript below only if needed
    else
    {
        const Logic::BasicToken* next_basic_token = PeekNextBasicToken();
        process_dot_notation_function = ( next_basic_token != nullptr &&
                                          next_basic_token->type == Logic::BasicToken::Type::Operator && next_basic_token->token_code == TokenCode::TOKPERIOD );

        symbol_subscript_compilation = process_dot_notation_function ? -1 :
                                                                       CompileItemSubscriptImplicit(engine_item, Logic::FunctionDetails::StaticType::NeverStatic);
    }
 
    // process a function if using dot notation
    if( process_dot_notation_function )
    {
        const Logic::BasicToken* next_basic_token = PeekNextBasicToken(( symbol_subscript_compilation == -1 ) ? 1 : 0);
        const Logic::FunctionDetails* function_details;

        // the function can be belong to the symbol, or to the symbol's wrapped type
        if( next_basic_token != nullptr && next_basic_token->type == Logic::BasicToken::Type::Text &&
            Logic::FunctionTable::IsFunction(next_basic_token->GetTextSV(), engine_item, &function_details) )
        {
            // if necessary, compile an implicit subscript for non-static functions
            if( symbol_subscript_compilation == -1 && function_details->static_type != Logic::FunctionDetails::StaticType::AlwaysStatic )
                CompileItemSubscriptImplicit(engine_item, function_details->static_type);

            GetCurrentToken().reset(TokenCode::TOKFUNCTION, next_basic_token->GetTextSV());

            GetCurrentToken().function_details = function_details;

            // move past the tokens to what should be the left parenthesis of the function call
            if( !subscript_supplied_explicity )
                NextBasicToken();

            NextBasicToken();
        }

        else
        {
            IssueError(93003, engine_item.GetName().c_str(), ( next_basic_token != nullptr ) ? next_basic_token->GetText().c_str() : _T(""));
        }
    }

    // otherwise process the symbol, changing the token to the wrapped type
    else
    {
        GetCurrentToken().reset(GetTokenCodeFromSymbolType(engine_item.GetWrappedType()), engine_item.GetName());
        ASSERT80(GetCurrentToken().code != GetTokenCodeFromSymbolType(engine_item.GetType()));

        Tokstindex = engine_item.GetSymbolIndex();
    }

    GetCurrentToken().symbol = &engine_item;
    GetCurrentToken().symbol_subscript_compilation = symbol_subscript_compilation;

    // when an explicit subscript is provided when this is not a function, move back one token
    // (because users of this token will do NextToken to move past the symbol token)
    if( subscript_supplied_explicity && !process_dot_notation_function )
        MoveNextBasicTokenIndex(-1);
}


void LogicCompiler::ProcessFunction()
{
    BaseCompiler::ProcessFunction();

    if( CurrentToken.symbol != nullptr && CurrentToken.symbol_subscript_compilation == -1 )
    {
        ASSERT(CurrentToken.function_details != nullptr && CurrentToken.function_details->function_domain != SymbolType::None);

        // if a dot-notation function is used with an object that requires a subcript and an explicit subscript
        // has not been defined, an implicit subscript must be calculated if the function is not static
        if( CurrentToken.function_details->static_type != Logic::FunctionDetails::StaticType::AlwaysStatic )
        {
            // the compilation of implicit subscripts for function calls will occur in ProcessSymbolWithWrappedType,
            // but this code is to handle cases that come from BaseCompiler::ProcessText
            if( CurrentToken.symbol->IsA(SymbolType::Item) )
            {
                GetCurrentToken().symbol_subscript_compilation = CompileItemSubscriptImplicit(assert_cast<const EngineItem&>(*CurrentToken.symbol),
                                                                                              CurrentToken.function_details->static_type);
            }
        }
    }
}


LogicCompiler::NextTokenHelperResult LogicCompiler::CheckNextTokenHelper(SymbolType preferred_symbol_type/* = SymbolType::None*/)
{
    NextTokenHelperResult result = NextTokenHelperResult::Unknown;

    MarkInputBufferToRestartLater();

    try
    {
        Logic::BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
        compiler_settings_modifier.SuppressErrorReporting();

        NextTokenWithPreference(preferred_symbol_type);

        TokenCode saved_token = Tkn;

        if( saved_token == TOKCTE || saved_token == TOKSCTE || saved_token == TOKWORKSTRING )
        {
            NextToken();

            if( Tkn == TOKCOMMA || Tkn == TOKRPAREN || Tkn == TOKSEMICOLON ) // prevent things like: "A" + "B"
            {
                result = ( saved_token == TOKCTE )  ? NextTokenHelperResult::NumericConstantNonNegative :
                         ( saved_token == TOKSCTE ) ? NextTokenHelperResult::StringLiteral :
                                                      NextTokenHelperResult::WorkString;
            }
        }

        else if( ( saved_token == TOKDICT_PRE80 ) ||
                 ( saved_token == TOKSECT ) ||
                 ( saved_token == TOKFORM ) ||
                 ( saved_token == TOKGROUP ) ||
                 ( saved_token == TOKBLOCK ) ||
                 ( saved_token == TOKVAR && NPT_Ref(Tokstindex).IsA(SymbolType::Variable) && VPT(Tokstindex)->GetOwnerDic() > 0 ) ||
                 ( saved_token == TOKARRAY ) ||
                 ( saved_token == TOKLIST ) )
        {
            NextToken();

            if( Tkn == TOKCOMMA || Tkn == TOKRPAREN || Tkn == TOKSEMICOLON )
            {
                result = ( saved_token == TOKARRAY ) ? NextTokenHelperResult::Array :
                         ( saved_token == TOKLIST )  ? NextTokenHelperResult::List :
                                                       NextTokenHelperResult::DictionaryRelatedSymbol;
            }
        }

        Tkn = saved_token;
    }

    catch( const Logic::ParserError& )
    {
    }

    RestartFromMarkedInputBuffer();

    return result;
}


std::optional<SymbolType> LogicCompiler::GetNextTokenSymbolType()
{
    // returns whether the next token is a symbol without being part of an expression; for example,
    // my_list would return SymbolType::List but my_list(3) would return std::nullopt
    std::optional<SymbolType> symbol_type;

    MarkInputBufferToRestartLater();

    try
    {
        Logic::BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
        compiler_settings_modifier.SuppressErrorReporting();

        NextToken();

        auto set_if_not_followed_by_left_parenthesis = [&](auto type)
        {
            if( !NextKeywordIf(TOKLPAREN) )
                symbol_type = type;
        };

        switch( Tkn )
        {
            case TOKARRAY:
                set_if_not_followed_by_left_parenthesis(SymbolType::Array);
                break;

            case TOKAUDIO:
                symbol_type = SymbolType::Audio;
                break;

            case TOKDICT:
                symbol_type = SymbolType::Dictionary;
                break;

            case TOKDICT_PRE80:
                symbol_type = SymbolType::Pre80Dictionary;
                break;

            case TOKDOCUMENT:
                symbol_type = SymbolType::Document;
                break;

            case TOKFILE:
                symbol_type = SymbolType::File;
                break;

            case TOKFREQ:
                set_if_not_followed_by_left_parenthesis(SymbolType::NamedFrequency);
                break;

            case TOKGEOMETRY:
                symbol_type = SymbolType::Geometry;
                break;

            case TOKHASHMAP:
                set_if_not_followed_by_left_parenthesis(SymbolType::HashMap);
                break;

            case TOKIMAGE:
                symbol_type = SymbolType::Image;
                break;

            case TOKLIST:
                set_if_not_followed_by_left_parenthesis(SymbolType::List);
                break;

            case TOKMAP:
                symbol_type = SymbolType::Map;
                break;

            case TOKPFF:
                symbol_type = SymbolType::Pff;
                break;

            case TOKREPORT:
                symbol_type = SymbolType::Report;
                break;

            case TOKSYSTEMAPP:
                symbol_type = SymbolType::SystemApp;
                break;

            case TOKUSERFUNCTION:
                set_if_not_followed_by_left_parenthesis(SymbolType::UserFunction);
                break;

            case TOKVALUESET:
                symbol_type = SymbolType::ValueSet;
                break;
        }
    }

    catch( const Logic::ParserError& )
    {
    }

    RestartFromMarkedInputBuffer();

    return symbol_type;
}
