#include "stdafx.h"
#include "IncludesRT.h"
#include "Image.h"
#include "Nodes/Barcode.h"
#include <zMultimediaO/Image.h>
#include <zMultimediaO/QRCode.h>
#include <zParadataO/Logger.h>


double LogicInterpreter::ex_Barcode_read(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int& message_text_expression = va_node.arguments[0];
    const SharableString message_text = ( message_text_expression >= 0 ) ? EvaluateSharableString(message_text_expression).MakeTrim() :
                                                                           SharableString();
    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::BarcodeRead);

    SharableString barcode = m_applicationInterface->BarcodeRead(*message_text);

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, barcode, true);
        RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::move(operator_selection_event));
    }

    return AssignString(std::move(barcode));
}


double LogicInterpreter::ex_Barcode_createQRCode(const int program_index)
{
    const auto& create_qr_code_node = GetNode<Nodes::CreateQRCode>(program_index);

    try
    {
        Multimedia::QRCode qr_code;

        // process any options
        if( create_qr_code_node.options_node_index != -1 )
        {
            const auto& create_qr_code_options_node = GetNode<Nodes::CreateQRCodeOptions>(create_qr_code_node.options_node_index);

            if( create_qr_code_options_node.error_correction_expression != -1 )
                qr_code.SetErrorCorrectionLevel(*EvaluateSharableString(create_qr_code_options_node.error_correction_expression));

            if( create_qr_code_options_node.scale_expression != -1 )
                qr_code.SetScale(Evaluate<int>(create_qr_code_options_node.scale_expression));

            if( create_qr_code_options_node.quiet_zone_expression != -1 )
                qr_code.SetQuietZone(Evaluate<int>(create_qr_code_options_node.quiet_zone_expression));

            auto evaluate_portable_color = [&](const int expression)
            {
                const SharableString color_text = EvaluateSharableString(expression);
                const std::optional<PortableColor> color = PortableColor::FromString(*color_text);

                if( !color.has_value() )
                    throw CSProException(GetFormattedMessage(MGF::color_invalid_2036, color_text->c_str()));

                return *color;
            };

            if( create_qr_code_options_node.dark_color_expression != -1 )
                qr_code.SetDarkColor(evaluate_portable_color(create_qr_code_options_node.dark_color_expression));

            if( create_qr_code_options_node.light_color_expression != -1 )
                qr_code.SetLightColor(evaluate_portable_color(create_qr_code_options_node.light_color_expression));
        }


        // create the QR code
        SharableString text = EvaluateSharableString(create_qr_code_node.value_data_type, create_qr_code_node.value_expression);
        qr_code.Create(*text);

        std::unique_ptr<Multimedia::Image> qr_code_bitmap = qr_code.GetImage();
        ASSERT(qr_code_bitmap != nullptr);

        // move the QR code image to a Image object...
        if( create_qr_code_node.symbol_index_or_filename_expression < 0 )
        {
            LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(-1 * create_qr_code_node.symbol_index_or_filename_expression, create_qr_code_node.subscript_compilation);

            if( logic_image == nullptr )
                return 0;

            logic_image->Load(std::move(qr_code_bitmap), "qr-code.bmp");

            BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadata();
            binary_data_metadata.SetProperty("label", text.Release());
            binary_data_metadata.SetProperty("source", "Image.createQRCode");
            binary_data_metadata.SetProperty("timestamp", GetTimestamp());
        }

        // ... or save it to disk
        else
        {
            const std::string file_path = EvaluatePath(create_qr_code_node.symbol_index_or_filename_expression);
            qr_code_bitmap->ToFile(file_path);
        }
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, 100331, exception.what());
        return 0;
    }

    return 1;
}
