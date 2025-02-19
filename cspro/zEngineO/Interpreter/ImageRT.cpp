#include "stdafx.h"
#include "IncludesRT.h"
#include "Document.h"
#include "Image.h"
#include "ValueSet.h"
#include <zHtml/VirtualFileMapping.h>
#include <zUtilF/ImageCaptureDlg.h>
#include <zEngineF/EngineUI.h>
#include <zDictO/ValueProcessor.h>


namespace ImageRT
{
    bool EnsureImageExists(LogicInterpreter& interpreter, const LogicImage& logic_image, const char* action_for_displayed_error_message);
    bool EnsureImageExistsAndIsValid(LogicInterpreter& interpreter, const LogicImage& logic_image, const char* action_for_displayed_error_message);
}


bool ImageRT::EnsureImageExists(LogicInterpreter& interpreter, const LogicImage& logic_image, const char* const action_for_displayed_error_message)
{
    if( !logic_image.HasContent() )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            interpreter.IssueMessage(MessageType::Error, MGF::Image_no_image_for_action_100320,
                                                         logic_image.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    return true;
}


bool ImageRT::EnsureImageExistsAndIsValid(LogicInterpreter& interpreter, const LogicImage& logic_image, const char* const action_for_displayed_error_message)
{
    if( !EnsureImageExists(interpreter, logic_image, action_for_displayed_error_message) )
    {
        return false;
    }

    else if( !logic_image.HasValidImage(true) )
    {
        if( action_for_displayed_error_message != nullptr )
        {
            interpreter.IssueMessage(MessageType::Error, MGF::Image_invalid_content_error_100321,
                                                         logic_image.GetName().c_str(), action_for_displayed_error_message);
        }

        return false;
    }

    return true;
}


double LogicInterpreter::ex_Image_compute(const int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);
    const Symbol* const rhs_symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

    if( rhs_symbol == nullptr )
        return 0;

    LogicImage* const lhs_logic_image = GetFromSymbolOrEngineItem<LogicImage*>(lhs_symbol_reference);

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
        IssueMessage(MessageType::Error, MGF::Image_assignment_error_100327,
                                         rhs_symbol->GetName().c_str(), lhs_logic_image->GetName().c_str(),
                                         exception.what());
    }

    return 0;
}


double LogicInterpreter::ex_Image_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    logic_image->Reset();

    return 1;
}


double LogicInterpreter::ex_Image_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    // the file path can come from...
    std::string file_path;

    // ...a string literal
    if( symbol_va_with_subscript_node.arguments[1] == -1 )
    {
        file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    }

    // ...or a value set
    else
    {
        const ValueSet& value_set = GetSymbolValueSet(symbol_va_with_subscript_node.arguments[0]);
        const ValueProcessor& value_processor = value_set.GetValueProcessor();
        const DictValue* const dict_value = value_set.IsNumeric() ?
            value_processor.GetDictValue(Evaluate(symbol_va_with_subscript_node.arguments[1])) :
            value_processor.GetDictValue(UTF8_TODO::GetCString(EvaluateSharableString(symbol_va_with_subscript_node.arguments[1]).GetString()));

        if( dict_value != nullptr )
            file_path = dict_value->GetImageFilePath();
    }

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    // load the image
    try
    {
        if( file_path.empty() )
            throw CSProException("No image was specified.");

        logic_image->Load(file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Image_load_error_100322,
                                         file_path.c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Image_resample(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    const bool specifying_maxes = ( symbol_va_with_subscript_node.arguments[0] == -1 && symbol_va_with_subscript_node.arguments[1] == -1 );
    const std::optional<double> specified_width = EvaluateOptional(symbol_va_with_subscript_node.arguments[specifying_maxes ? 2: 0]);
    const std::optional<double> specified_height = EvaluateOptional(symbol_va_with_subscript_node.arguments[specifying_maxes ? 3: 1]);

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !ImageRT::EnsureImageExistsAndIsValid(*this, *logic_image, "resample the image") )
        return DEFAULT;

    try
    {
        auto check_dimension = [&](const std::optional<double>& dimension, const char* const type) -> std::optional<int>
        {
            if( !dimension.has_value() || dimension == NOTAPPL )
                return std::nullopt;

            const int int_dimension = static_cast<int>(*dimension);

            if( int_dimension < 1 || IsSpecial(*dimension) )
                throw CSProException("The %s %s is invalid.", type, DoubleToString(*dimension).c_str());

            return int_dimension;
        };

        int new_width = logic_image->GetWidth();
        int new_height = logic_image->GetHeight();
        const double current_width_double = new_width;
        const double current_height_double = new_height;

        // the width or height was specified
        if( !specifying_maxes )
        {
            const std::optional<int> width = check_dimension(specified_width, "width");
            const std::optional<int> height = check_dimension(specified_height, "height");

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
            const std::optional<int> max_width = check_dimension(specified_width, "maxWidth");
            const std::optional<int> max_height = check_dimension(specified_height, "maxHeight");
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
        IssueMessage(MessageType::Error, MGF::Image_resample_error_100324,
                                         logic_image->GetName().c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Image_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    const std::optional<double> specified_jpeg_quality = EvaluateOptional(symbol_va_with_subscript_node.arguments[1]);

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !ImageRT::EnsureImageExists(*this, *logic_image, "save the image") )
        return DEFAULT;

    try
    {
        std::optional<int> jpeg_quality;

        if( specified_jpeg_quality.has_value() )
        {
            if( *specified_jpeg_quality < 0 || *specified_jpeg_quality > 100 )
            {
                throw CSProException("The quality '%s' is invalid; the value must be between 0 and 100.",
                                     DoubleToString(*specified_jpeg_quality).c_str());
            }

            jpeg_quality.emplace(static_cast<int>(*specified_jpeg_quality));
        }

        logic_image->Save(std::move(file_path), jpeg_quality);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Image_save_error_100323,
                                         logic_image->GetName().c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Image_captureSignature_takePhoto(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const bool capture_signature = ( symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_CAPTURESIGNATURE_CODE );
    ASSERT(capture_signature || symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_TAKEPHOTO_CODE);

#ifdef COMPONENTS_TODO_RESTORE_FOR_CSPRO81
    if constexpr(OnAndroid())
    {
        if( m_engineData->application != nullptr &&
            !m_engineData->application->GetApplicationProperties().GetUseHtmlComponentsInsteadOfNativeVersions() )
        {
            return capture_signature ? ex_Image_captureSignature_native(program_index) :
                                       ex_Image_takePhoto_native(program_index);
        }
    }

    const SharableString message = EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]);
    const bool show_existing_image = m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateOptionalConditional(symbol_va_with_subscript_node.arguments[1], true) :
                                                                                                                false;

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        // when applicable, display the current image
        std::unique_ptr<VirtualFileMappingHandler> image_virtual_file_mapping_handler;
        SharableString image_localhost_url;

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
        binary_data_metadata.SetProperty("label", capture_signature ? "Image (Signature)" : "Photo");
        binary_data_metadata.SetProperty("source", capture_signature ? "Image.captureSignature" : "Image.takePhoto");
        binary_data_metadata.SetProperty("timestamp", GetTimestamp());

        logic_image->LoadFromDataUrl(image_capture_dlg.GetImageDataUrl(), std::move(binary_data_metadata));
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Image_signature_error_100325, exception.what());
        return 0;
    }

    return 1;
#else
    return capture_signature ? ex_Image_captureSignature_native(program_index) :
                               ex_Image_takePhoto_native(program_index);
#endif
}


double LogicInterpreter::ex_Image_captureSignature_native(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    EngineUI::CaptureImageNode capture_image_node
    {
        EngineUI::CaptureImageNode::Action::CaptureSignature,
        EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]),
        std::string()
    };

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        if( SendEngineUIMessage(EngineUI::Type::CaptureImage, capture_image_node) != 1 )
            return 0;

        logic_image->Load(capture_image_node.output_file_path, true);

        BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadata();
        binary_data_metadata.SetProperty("label", "Image (Signature)");
        binary_data_metadata.SetProperty("source", "Image.captureSignature");
        binary_data_metadata.SetProperty("timestamp", GetTimestamp());

        PortableFunctions::FileDelete(capture_image_node.output_file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Image_signature_error_100325, exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Image_takePhoto_native(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);

    EngineUI::CaptureImageNode capture_image_node
    {
        EngineUI::CaptureImageNode::Action::TakePhoto,
        EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]),
        std::string()
    };

    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return 0;

    try
    {
        if( SendEngineUIMessage(EngineUI::Type::CaptureImage, capture_image_node) != 1 )
            return 0;

        logic_image->Load(capture_image_node.output_file_path, true);

        BinaryDataMetadata& binary_data_metadata = logic_image->GetMetadata();
        binary_data_metadata.SetProperty("label", "Photo");
        binary_data_metadata.SetProperty("source", "Image.takePhoto");
        binary_data_metadata.SetProperty("timestamp", GetTimestamp());

        PortableFunctions::FileDelete(capture_image_node.output_file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Image_photo_error_100326, exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Image_view(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr )
        return DEFAULT;

    const std::unique_ptr<const ViewerOptions> viewer_options = m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateViewerOptions(symbol_va_with_subscript_node.arguments[0]) :
                                                                                                                                           nullptr;

    return ex_Image_view(*logic_image, viewer_options.get());
}


double LogicInterpreter::ex_Image_view(const LogicImage& logic_image, const ViewerOptions* const viewer_options)
{
    if( !ImageRT::EnsureImageExists(*this, logic_image, "view the image") )
        return DEFAULT;

    logic_image.View(viewer_options);

    return 1;
}


double LogicInterpreter::ex_Image_width_height(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicImage* const logic_image = GetFromSymbolOrEngineItem<LogicImage*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_image == nullptr || !ImageRT::EnsureImageExistsAndIsValid(*this, *logic_image, nullptr) )
        return DEFAULT;

    return ( symbol_va_with_subscript_node.function_code == FunctionCode::IMAGEFN_WIDTH_CODE ) ? logic_image->GetWidth() :
                                                                                                 logic_image->GetHeight();
}
