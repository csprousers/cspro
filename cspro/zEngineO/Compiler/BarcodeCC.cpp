#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/Barcode.h"
#include <zMultimediaO/QRCode.h>


int LogicCompiler::CompileBarcodeFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    const Symbol* symbol = CurrentToken.symbol;
    int subscript_compilation = CurrentToken.symbol_subscript_compilation;
    int program_index;

    // createQRCode can apply to Image objects
    ASSERT(symbol == nullptr ||
           ( symbol->IsOneOf(SymbolType::Image, SymbolType::Item) && function_code == FunctionCode::BARCODEFN_CREATEQRCODE_CODE ));

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();


    // Barcode.read([message])
    if( function_code == FunctionCode::BARCODEFN_READ_CODE )
    {
        int message_text_expression = -1;

        if( Tkn != TOKRPAREN )
            message_text_expression = CompileStringExpression();

        auto& va_node = CreateVariableSizeNode<Nodes::VariableArguments>(function_code, 1);
        va_node.arguments[0] = message_text_expression;

        program_index = GetProgramIndex(va_node);
    }


    // Barcode.createQRCode("filename", "text" | number,
	//				        errorCorrection := "L" | "M" | "Q" | "H" | "low" | "medium" | "quartile" | "high",
    //					    scale := number,
    //					    quietZone := number,
    //					    darkColor := color,
    //					    lightColor := color);
    //
    // image_name.createQRCode (as above but without the first parameter)
    else if( function_code == FunctionCode::BARCODEFN_CREATEQRCODE_CODE )
    {
        auto& create_qr_code_node = CreateNode<Nodes::CreateQRCode>(function_code);

        // process the destination
        if( symbol != nullptr )
        {
            create_qr_code_node.symbol_index_or_filename_expression = -1 * symbol->GetSymbolIndex();
            create_qr_code_node.subscript_compilation = subscript_compilation;
        }

        else
        {
            create_qr_code_node.symbol_index_or_filename_expression = CompileStringExpression();

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
            NextToken();
        }

        // process the barcode text
        create_qr_code_node.value_data_type = GetCurrentTokenDataType();
        create_qr_code_node.value_expression = CompileExpression(create_qr_code_node.value_data_type);

        if( Tkn == TOKRPAREN )
        {
            create_qr_code_node.options_node_index = -1;
        }

        else
        {
            auto& create_qr_code_options_node = CreateNode<Nodes::CreateQRCodeOptions>();
            InitializeNode(create_qr_code_options_node, -1);

            create_qr_code_node.options_node_index = GetProgramIndex(create_qr_code_options_node);

            // process optional arguments
            OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

            optional_named_arguments_compiler.AddArgumentWithStringLiteralCheck(_T("errorCorrection"), create_qr_code_options_node.error_correction_expression,
                [&](const std::wstring& text)
                {
                    if( Multimedia::QRCode::GetErrorCorrectionLevelFromText(text) == std::nullopt )
                        IssueError(100330, text.c_str());
                });

            optional_named_arguments_compiler.AddArgumentInteger(_T("scale"), create_qr_code_options_node.scale_expression, Multimedia::QRCode::ScaleMin, std::nullopt);
            optional_named_arguments_compiler.AddArgumentInteger(_T("quietZone"), create_qr_code_options_node.quiet_zone_expression, Multimedia::QRCode::QuietZoneMin, std::nullopt);
            optional_named_arguments_compiler.AddArgumentPortableColorText(_T("darkColor"), create_qr_code_options_node.dark_color_expression);
            optional_named_arguments_compiler.AddArgumentPortableColorText(_T("lightColor"), create_qr_code_options_node.light_color_expression);

            optional_named_arguments_compiler.Compile();
        }

        program_index = GetProgramIndex(create_qr_code_node);
    }


    else
    {
        throw ProgrammingErrorException();
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return program_index;
}
