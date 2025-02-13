#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Document.h>
#include <zEngineO/Image.h>
#include <zEngineO/ValueSet.h>
#include <zEngineO/Versioning.h>
#include <zEngineF/EngineUI.h>
#include <zHtml/VirtualFileMapping.h>
#include <zUtilF/ImageCaptureDlg.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDictO/ValueProcessor.h>


namespace
{
    bool EnsureImageExists(CIntDriver& interpreter, const LogicImage& logic_image, const TCHAR* action_for_displayed_error_message)
    {
        if( !logic_image.HasContent() )
        {
            if( action_for_displayed_error_message != nullptr )
                interpreter.issaerror(MessageType::Error, 100320, logic_image.GetName().c_str(), action_for_displayed_error_message);

            return false;
        }

        return true;
    }


    bool EnsureImageExistsAndIsValid(CIntDriver& interpreter, const LogicImage& logic_image, const TCHAR* action_for_displayed_error_message)
    {
        if( !EnsureImageExists(interpreter, logic_image, action_for_displayed_error_message) )
        {
            return false;
        }

        else if( !logic_image.HasValidImage(true) )
        {
            if( action_for_displayed_error_message != nullptr )
                interpreter.issaerror(MessageType::Error, 100321, logic_image.GetName().c_str(), action_for_displayed_error_message);

            return false;
        }

        return true;
    }
}


double CIntDriver::exImage_compute(int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicImage* lhs_logic_image = GetFromSymbolOrEngineItem<LogicImage*>(lhs_symbol_reference);

    if( lhs_logic_image == nullptr )
        return 0;

    try
    {
        if( rhs_symbol->IsA(SymbolType::Image) )
        {
            *lhs_logic_image = assert_cast<const LogicImage&>(*rhs_symbol);
        }

        else if( rhs_symbol->IsA(SymbolType::Document) )
        {
            *lhs_logic_image = assert_cast<const LogicDocument&>(*rhs_symbol);
        }

        else
        {
            ASSERT(false);
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100327, rhs_symbol->GetName().c_str(), lhs_logic_image->GetName().c_str(), exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exImage_clear(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    logic_image->Reset();

    return 1;
}


double CIntDriver::exImage_load(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    // the filename can come from...
    std::wstring filename;

    // ...a string literal
    if( symbol_va_with_subscript_node.arguments[1] == -1 )
    {
        filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    }

    // ...or a value set
    else
    {
        const ValueSet& value_set = GetSymbolValueSet(symbol_va_with_subscript_node.arguments[0]);
        const ValueProcessor& value_processor = value_set.GetValueProcessor();
        const DictValue* dict_value = value_set.IsNumeric() ?
            value_processor.GetDictValue(evalexpr(symbol_va_with_subscript_node.arguments[1])) :
            value_processor.GetDictValue(EvalAlphaExpr<CString>(symbol_va_with_subscript_node.arguments[1]));

        if( dict_value != nullptr )
            filename = dict_value->GetImageFilename();
    }

    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    // load the image
    try
    {
        if( filename.empty() )
            throw CSProException("No image was specified.");

        logic_image->Load(filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100322, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exImage_resample(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    const bool specifying_maxes = ( symbol_va_with_subscript_node.arguments[0] == -1 && symbol_va_with_subscript_node.arguments[1] == -1 );
    const std::optional<double> specified_width = EvaluateOptionalNumericExpression(symbol_va_with_subscript_node.arguments[specifying_maxes ? 2: 0]);
    const std::optional<double> specified_height = EvaluateOptionalNumericExpression(symbol_va_with_subscript_node.arguments[specifying_maxes ? 3: 1]);

    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !EnsureImageExistsAndIsValid(*this, *logic_image, _T("resample the image")) )
        return DEFAULT;

    try
    {
        auto check_dimension = [&](const std::optional<double>& dimension, const TCHAR* const type) -> std::optional<int>
        {
            if( !dimension.has_value() || dimension == NOTAPPL )
                return std::nullopt;

            const int int_dimension = static_cast<int>(*dimension);

            if( int_dimension < 1 || IsSpecial(*dimension) )
                throw CSProException(_T("The %s %s is invalid."), type, DoubleToString(*dimension).c_str());

            return int_dimension;
        };

        int new_width = logic_image->GetWidth();
        int new_height = logic_image->GetHeight();
        const double current_width_double = new_width;
        const double current_height_double = new_height;

        // the width or height was specified
        if( !specifying_maxes )
        {
            const std::optional<int> width = check_dimension(specified_width, _T("width"));
            const std::optional<int> height = check_dimension(specified_height, _T("height"));

            // resample on width/height or width only
            if( width.has_value() )
            {
                new_width = *width;
                new_height = height.has_value() ? *height :
                                                  static_cast<int>(new_width / current_width_double * current_height_double);
            }

            // resample on height only
            else if( height.has_value() )
            {
                new_height = *height;
                new_width = static_cast<int>(new_height / current_height_double * current_width_double);
            }
        }

        // calculate the dimensions based on a resample factor when a max width/height are provided
        else
        {
            const std::optional<int> max_width = check_dimension(specified_width, _T("maxWidth"));
            const std::optional<int> max_height = check_dimension(specified_height, _T("maxHeight"));
            double resample_factor = 1;

            if( max_width.has_value() && *max_width < new_width )
                resample_factor = *max_width / current_width_double;

            if( max_height.has_value() && *max_height < new_height )
                resample_factor = std::min(resample_factor, *max_height / current_height_double);

            if( resample_factor != 1 )
            {
                new_width = static_cast<int>(resample_factor * current_width_double);
                new_height = static_cast<int>(resample_factor * current_height_double);
            }
        }

        new_width = std::max(1, new_width);
        new_height = std::max(1, new_height);

        // resample the image
        if( new_width != logic_image->GetWidth() || new_height != logic_image->GetHeight() )
            logic_image->Resample(new_width, new_height);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100324, logic_image->GetName().c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exImage_save(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    const std::optional<double> specified_jpeg_quality = EvaluateOptionalNumericExpression(symbol_va_with_subscript_node.arguments[1]);

    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !EnsureImageExists(*this, *logic_image, _T("save the image")) )
        return DEFAULT;

    try
    {
        std::optional<int> jpeg_quality;

        if( specified_jpeg_quality.has_value() )
        {
            if( *specified_jpeg_quality < 0 || *specified_jpeg_quality > 100 )
                throw CSProException(_T("The quality %s is invalid; the value must be between 0 and 100."), DoubleToString(*specified_jpeg_quality).c_str());

            jpeg_quality.emplace(static_cast<int>(*specified_jpeg_quality));
        }

        logic_image->Save(std::move(filename), jpeg_quality);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100323, logic_image->GetName().c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exImage_captureSignature_takePhoto(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const bool capture_signature = ( symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_CAPTURESIGNATURE_CODE );
    ASSERT(capture_signature || symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_TAKEPHOTO_CODE);

#ifdef COMPONENTS_TODO_RESTORE_FOR_CSPRO81
    if constexpr(OnAndroid())
    {
        if( !m_pEngineDriver->GetApplication()->GetApplicationProperties().GetUseHtmlComponentsInsteadOfNativeVersions() )
        {
            return capture_signature ? exImage_captureSignature_native(program_index) :
                                       exImage_takePhoto_native(program_index);
        }
    }

    const std::optional<std::wstring> message = EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[0]);
    const bool show_existing_image =
        Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateOptionalConditionalExpression(symbol_va_with_subscript_node.arguments[1], true) :
                                                                                 false;

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        // when applicable, display the current image
        std::unique_ptr<VirtualFileMappingHandler> image_virtual_file_mapping_handler;
        std::optional<std::wstring> image_localhost_url;

        if( show_existing_image && logic_image->HasContent() )
        {
            image_virtual_file_mapping_handler = LocalhostCreateMappingForBinarySymbol<std::unique_ptr<VirtualFileMappingHandler>>(*logic_image);
            image_localhost_url = image_virtual_file_mapping_handler->GetUrl();
        }

        ImageCaptureDlg image_capture_dlg(capture_signature ? ImageCaptureDlg::ImageCaptureType::Signature : ImageCaptureDlg::ImageCaptureType::Photo,
                                          message, image_localhost_url);

        if( image_capture_dlg.DoModalOnUIThread() != IDOK )
            return 0;

        ASSERT(!image_capture_dlg.GetImageDataUrl().empty());

        BinaryDataMetadata binary_data_metadata;
        binary_data_metadata.SetProperty(_T("label"), capture_signature ? _T("Image (Signature)") : _T("Photo"));
        binary_data_metadata.SetProperty(_T("source"), capture_signature ? _T("Image.captureSignature") : _T("Image.takePhoto"));
        binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());

        logic_image->LoadFromDataUrl(image_capture_dlg.GetImageDataUrl(), std::move(binary_data_metadata));
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100325, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
#else
    return capture_signature ? exImage_captureSignature_native(program_index) :
                               exImage_takePhoto_native(program_index);
#endif
}


double CIntDriver::exImage_captureSignature_native(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    EngineUI::CaptureImageNode capture_image_node
    {
        EngineUI::CaptureImageNode::Action::CaptureSignature,
        EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[0]),
        std::wstring()
    };

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        if( SendEngineUIMessage(EngineUI::Type::CaptureImage, capture_image_node) != 1 )
            return 0;

        logic_image->Load(capture_image_node.output_filename, true);

        BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadataForModification();
        binary_data_metadata.SetProperty(_T("label"), _T("Image (Signature)"));
        binary_data_metadata.SetProperty(_T("source"), _T("Image.captureSignature"));
        binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());

        PortableFunctions::FileDelete(capture_image_node.output_filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100325, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exImage_takePhoto_native(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    EngineUI::CaptureImageNode capture_image_node
    {
        EngineUI::CaptureImageNode::Action::TakePhoto,
        EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[0]),
        std::wstring()
    };

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        if( SendEngineUIMessage(EngineUI::Type::CaptureImage, capture_image_node) != 1 )
            return 0;

        logic_image->Load(capture_image_node.output_filename, true);

        BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadataForModification();
        binary_data_metadata.SetProperty(_T("label"), _T("Photo"));
        binary_data_metadata.SetProperty(_T("source"), _T("Image.takePhoto"));
        binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());

        PortableFunctions::FileDelete(capture_image_node.output_filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100326, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exImage_view(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return DEFAULT;

    std::unique_ptr<const ViewerOptions> viewer_options = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateViewerOptions(symbol_va_with_subscript_node.arguments[0]) : 
                                                                                                                                   nullptr;

    return exImage_view(*logic_image, viewer_options.get());
}


double CIntDriver::exImage_view(const LogicImage& logic_image, const ViewerOptions* viewer_options)
{
    if( !EnsureImageExists(*this, logic_image, _T("view the image")) )
        return DEFAULT;

    logic_image.View(viewer_options);

    return 1;
}


double CIntDriver::exImage_width_height(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !EnsureImageExistsAndIsValid(*this, *logic_image, nullptr) )
        return DEFAULT;

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_WIDTH_CODE ) ? logic_image->GetWidth() :
                                                                                                 logic_image->GetHeight();
}
