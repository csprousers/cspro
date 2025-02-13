#include "stdafx.h"
#include "IncludesCC.h"
#include "Image.h"
#include "ValueSet.h"


LogicImage* LogicCompiler::CompileLogicImageDeclaration()
{
    std::wstring image_name = CompileNewSymbolName();

    auto logic_image = std::make_shared<LogicImage>(std::move(image_name));

    m_engineData->AddSymbol(logic_image);

    return logic_image.get();
}


int LogicCompiler::CompileLogicImageDeclarations()
{
    ASSERT(Tkn == TOKKWIMAGE);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicImage* logic_image = CompileLogicImageDeclaration();

        int& initialize_value = AddSymbolResetNode(symbol_reset_node, *logic_image);

        NextToken();

        // allow assignments as part of the declaration
        if( Tkn == TOKEQOP )
            initialize_value = CompileLogicImageComputeInstruction(logic_image);

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicImageComputeInstruction(const LogicImage* logic_image_from_declaration/* = nullptr*/)
{
    const Symbol* lhs_symbol = logic_image_from_declaration;
    const int lhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    if( IsGlobalCompilation() )
        IssueError(MGF::symbol_assignment_not_allowed_in_proc_global_687, ToString(SymbolType::Image));

    if( lhs_symbol == nullptr )
    {
        ASSERT(Tkn == TOKIMAGE);
        lhs_symbol = &NPT_Ref(Tokstindex);
        ASSERT(lhs_symbol->IsA(SymbolType::Image) || ( lhs_symbol->IsA(SymbolType::Item) && lhs_subscript_compilation != -1 ));

        NextToken();

        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Image_invalid_assignment_47241);
    }

    ASSERT(Tkn == TOKEQOP);

    NextToken();

    // an Image can be assigned another Image or a Document object
    if( Tkn != TOKIMAGE && Tkn != TOKDOCUMENT )
        IssueError(MGF::Image_invalid_assignment_47241);

    auto& symbol_compute_with_subscript_node = CreateNode<Nodes::SymbolComputeWithSubscript>(FunctionCode::IMAGEFN_COMPUTE_CODE);

    symbol_compute_with_subscript_node.next_st = -1;
    symbol_compute_with_subscript_node.lhs_symbol_index = lhs_symbol->GetSymbolIndex();
    symbol_compute_with_subscript_node.lhs_subscript_compilation = lhs_subscript_compilation;
    symbol_compute_with_subscript_node.rhs_symbol_index = Tokstindex;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = CurrentToken.symbol_subscript_compilation;

    NextToken();

    if( logic_image_from_declaration == nullptr )
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetProgramIndex(symbol_compute_with_subscript_node);
}


int LogicCompiler::CompileLogicImageFunctions()
{
    // compiling image_name.captureSignature([message][, showExisting := b]);
    //           image_name.clear();
    //           image_name.height();
    //           image_name.load(filename);  ||  image_name.load(valueset_name, from_value);
    //           image_name.resample(width, height);
    //           image_name.save(filename[, quality := i]);
    //           image_name.takePhoto([message][, showExisting := b]);
    //           image_name.view([viewer options]);
    //           image_name.width();
    const FunctionCode function_code = CurrentToken.function_details->code;

    ASSERT(CurrentToken.symbol != nullptr && CurrentToken.symbol->IsOneOf(SymbolType::Image, SymbolType::Item));
    const Symbol& symbol = *CurrentToken.symbol;

    Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node =
        CreateSymbolVariableArgumentsWithSubscriptNode(function_code, symbol, CurrentToken.symbol_subscript_compilation,
                                                       CurrentToken.function_details->number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    const bool is_captureSignature_takePhoto = ( function_code == FunctionCode::IMAGEFN_CAPTURESIGNATURE_CODE ||
                                                 function_code == FunctionCode::IMAGEFN_TAKEPHOTO_CODE );
    const bool is_captureSignature_takePhoto_with_only_named_arguments = ( is_captureSignature_takePhoto &&
                                                                           IsNextTokenNamedArgument() );

    if( !is_captureSignature_takePhoto_with_only_named_arguments &&
        function_code != FunctionCode::IMAGEFN_RESAMPLE_CODE &&
        function_code != FunctionCode::IMAGEFN_VIEW_CODE )
    {
        NextToken();
    }

    auto check_comma_and_read_next_token = [&]()
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
        NextToken();
    };

    // captureSignature, load, and takePhoto can take a string argument; save must take a string argument
    if( ( function_code == FunctionCode::IMAGEFN_SAVE_CODE ) ||
        ( function_code == FunctionCode::IMAGEFN_LOAD_CODE && IsCurrentTokenString() ) ||
        ( is_captureSignature_takePhoto && !is_captureSignature_takePhoto_with_only_named_arguments && IsCurrentTokenString() ) )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileStringExpression();

        // save can take an optional named argument for the JPEG quality
        if( function_code == FunctionCode::IMAGEFN_SAVE_CODE )
        {
            OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
            optional_named_arguments_compiler.AddArgument(_T("quality"), symbol_va_with_subscript_node.arguments[1], DataType::Numeric);
            optional_named_arguments_compiler.Compile();
        }
    }


    // load can take a value set and numeric/string argument (based on the value set type)
    else if( function_code == FunctionCode::IMAGEFN_LOAD_CODE )
    {
        IssueErrorOnTokenMismatch(TOKVALUESET, MGF::Image_load_invalid_argument_47242);

        symbol_va_with_subscript_node.arguments[0] = Tokstindex;

        const ValueSet& value_set = GetSymbolValueSet(symbol_va_with_subscript_node.arguments[0]);

        NextToken();
        check_comma_and_read_next_token();

        symbol_va_with_subscript_node.arguments[1] = CompileExpression(value_set.GetDataType());
    }


    // resample takes a variety of named arguments, or two numeric arguments (width and height)
    else if( function_code == FunctionCode::IMAGEFN_RESAMPLE_CODE )
    {
        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
        optional_named_arguments_compiler.AddArgument(_T("width"), symbol_va_with_subscript_node.arguments[0], DataType::Numeric);
        optional_named_arguments_compiler.AddArgument(_T("height"), symbol_va_with_subscript_node.arguments[1], DataType::Numeric);
        optional_named_arguments_compiler.AddArgument(_T("maxWidth"), symbol_va_with_subscript_node.arguments[2], DataType::Numeric);
        optional_named_arguments_compiler.AddArgument(_T("maxHeight"), symbol_va_with_subscript_node.arguments[3], DataType::Numeric);

        // the named arguments version
        if( optional_named_arguments_compiler.Compile(true) > 0 )
        {
            bool width_or_height_specified = ( symbol_va_with_subscript_node.arguments[0] != -1 ) || ( symbol_va_with_subscript_node.arguments[1] != -1 );
            bool max_width_or_height_specified = ( symbol_va_with_subscript_node.arguments[2] != -1 ) || ( symbol_va_with_subscript_node.arguments[3] != -1 );

            if( width_or_height_specified == max_width_or_height_specified )
                IssueError(MGF::Image_resample_invalid_arguments_47243);
        }

        // the width and height version
        else
        {
            NextToken();

            symbol_va_with_subscript_node.arguments[0] = exprlog();

            check_comma_and_read_next_token();

            symbol_va_with_subscript_node.arguments[1] = exprlog();
        }
    }


    // view can take optional viewer options
    else if( function_code == FunctionCode::IMAGEFN_VIEW_CODE )
    {
        symbol_va_with_subscript_node.arguments[0] = CompileViewerOptions(true);

        if( symbol_va_with_subscript_node.arguments[0] == -1 )
            NextToken();
    }


    // captureSignature and takePhoto can take an optional named argument to indicate if the existing image should be shown
    if( is_captureSignature_takePhoto || is_captureSignature_takePhoto_with_only_named_arguments )
    {
        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
        optional_named_arguments_compiler.AddArgument(_T("showExisting"), symbol_va_with_subscript_node.arguments[1], DataType::Numeric);
        optional_named_arguments_compiler.Compile(is_captureSignature_takePhoto_with_only_named_arguments);
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_with_subscript_node);
}
