#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Image.h>
#include <zEngineO/Nodes/Barcode.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Utf8Convert.h>
#include <zParadataO/Logger.h>
#include <zMultimediaO/Image.h>
#include <zMultimediaO/QRCode.h>


double CIntDriver::exBarcode_read(int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int& message_text_expression = va_node.arguments[0];
    const std::wstring message_text = ( message_text_expression >= 0 ) ? std::wstring(SO::Trim(EvalAlphaExpr(message_text_expression))) :
                                                                         std::wstring();
    std::wstring barcode;

    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

#ifndef WIN_DESKTOP
    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::BarcodeRead);
    
    barcode = PlatformInterface::GetInstance()->GetApplicationInterface()->BarcodeRead(message_text);
#endif

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, barcode, true);
        m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    return AssignAlphaValue(std::move(barcode));
}


double CIntDriver::exBarcode_createQRCode(int program_index)
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
                qr_code.SetErrorCorrectionLevel(EvalAlphaExpr(create_qr_code_options_node.error_correction_expression));

            if( create_qr_code_options_node.scale_expression != -1 )
                qr_code.SetScale(static_cast<int>(evalexpr(create_qr_code_options_node.scale_expression)));

            if( create_qr_code_options_node.quiet_zone_expression != -1 )
                qr_code.SetQuietZone(static_cast<int>(evalexpr(create_qr_code_options_node.quiet_zone_expression)));

            auto evaluate_portable_color = [&](int expression)
            {
                const std::wstring color_text = EvalAlphaExpr(expression);
                std::optional<PortableColor> color = PortableColor::FromString(color_text);

                if( !color.has_value() )
                    throw CSProException(m_pEngineDriver->GetSystemMessageIssuer().GetFormattedMessage(2036, color_text.c_str()));

                return *color;
            };

            if( create_qr_code_options_node.dark_color_expression != -1 )
                qr_code.SetDarkColor(evaluate_portable_color(create_qr_code_options_node.dark_color_expression));

            if( create_qr_code_options_node.light_color_expression != -1 )
                qr_code.SetLightColor(evaluate_portable_color(create_qr_code_options_node.light_color_expression));
        }


        // create the QR code
        const std::wstring text = EvaluateExpressionAsString(create_qr_code_node.value_data_type, create_qr_code_node.value_expression);
        qr_code.Create(UTF8Convert::WideToUTF8(text));

        std::unique_ptr<Multimedia::Image> qr_code_bitmap = qr_code.GetImage();
        ASSERT(qr_code_bitmap != nullptr);

        // move the QR code image to a Image object...
        if( create_qr_code_node.symbol_index_or_filename_expression < 0 )
        {
            LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(-1 * create_qr_code_node.symbol_index_or_filename_expression, create_qr_code_node.subscript_compilation);

            if( logic_image == nullptr )
                return 0;

            logic_image->Load(std::move(qr_code_bitmap), _T("qr-code.bmp"));

            BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadataForModification();
            binary_data_metadata.SetProperty(_T("label"), text);
            binary_data_metadata.SetProperty(_T("source"), _T("Image.createQRCode"));
            binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());
        }

        // ... or save it to disk
        else
        {
            const std::wstring filename = EvalFullPathFileName(create_qr_code_node.symbol_index_or_filename_expression);
            qr_code_bitmap->ToFile(filename);            
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100331, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}
