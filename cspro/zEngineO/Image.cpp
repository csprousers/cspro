#include "stdafx.h"
#include "Image.h"
#include "Document.h"
#include <zUtilF/ImageViewDlg.h>
#include <zMultimediaO/Icon.h>
#include <zMultimediaO/Image.h>


// --------------------------------------------------------------------------
// LogicImage
// --------------------------------------------------------------------------

LogicImage::LogicImage(std::string image_name)
    :   BinarySymbol(std::move(image_name), SymbolType::Image)
{
}


LogicImage::LogicImage(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   BinarySymbol(engine_item, std::move(item_index), std::move(binary_data_accessor))
{
}


LogicImage::LogicImage(const LogicImage& logic_image)
    :   BinarySymbol(logic_image)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


std::unique_ptr<Symbol> LogicImage::CloneInInitialState() const
{
    return std::unique_ptr<LogicImage>(new LogicImage(*this));
}


LogicImage& LogicImage::operator=(const LogicImage& logic_image)
{
    if( this != &logic_image )
    {
        m_binarySymbolData = logic_image.m_binarySymbolData;
        m_image = logic_image.m_image;
    }

    return *this;
}


LogicImage& LogicImage::operator=(const LogicDocument& logic_document)
{
    const BinarySymbolData& document_binary_symbol_data = logic_document.GetBinarySymbolData();

    if( document_binary_symbol_data.IsDefined() )
    {
        const std::optional<Multimedia::ImageDetails> image_details = Multimedia::Image::GetDetailsFromBuffer(document_binary_symbol_data.GetContent());

        if( !image_details.has_value() )
            throw CSProException("The Document '%s' has data that cannot be converted to an Image.", logic_document.GetName().c_str());
    }

    m_binarySymbolData = document_binary_symbol_data;
    m_image.reset();

    return *this;
}


LogicImage& LogicImage::operator=(const BinarySymbolData& binary_symbol_data)
{
    m_binarySymbolData = binary_symbol_data;
    m_image.reset();

    return *this;
}


void LogicImage::Reset()
{
    BinarySymbol::Reset();
    m_image.reset();
}


const Multimedia::Image& LogicImage::GetParsedImage()
{
    ASSERT(HasContent());

    if( m_image == nullptr )
        m_image = Multimedia::Image::FromBuffer(m_binarySymbolData.GetContent());

    return *m_image;
}


BinaryData::ContentCallbackType LogicImage::CreateBinaryDataContentFromImageCallback() const
{
    ASSERT(m_image != nullptr);

    return
        [image = m_image]() -> std::shared_ptr<const std::vector<std::byte>>
        {
            ASSERT(image != nullptr);

            // default to saving the image as a PNG when the image type is unknown
            std::unique_ptr<std::vector<std::byte>> image_buffer = image->ToBuffer(image->GetDetails().image_type.value_or(ImageType::Png));

            if( image_buffer == nullptr )
            {
                ASSERT(false);
                image_buffer = std::make_unique<std::vector<std::byte>>();
            }

            return image_buffer;
        };
}


bool LogicImage::HasValidImage(const bool parse_image_if_necessary) const noexcept
{
    ASSERT(HasContent());

    if( m_image == nullptr )
    {
        if( !parse_image_if_necessary )
            return false;

        try
        {
            const_cast<LogicImage*>(this)->GetParsedImage();
        }

        catch(...)
        {
            return false;
        }
    }

    return true;
}


bool LogicImage::HasValidContent() const
{
    return HasContent() ? HasValidImage(true) :
                          false;
}


int LogicImage::GetWidth() const
{
    ASSERT(HasValidImage(false));

    return m_image->GetDetails().width;
}


int LogicImage::GetHeight() const
{
    ASSERT(HasValidImage(false));

    return m_image->GetDetails().height;
}


void LogicImage::Resample(const int width, const int height)
{
    ASSERT(HasValidImage(false));
    ASSERT(width > 0 && height > 0);

    m_image = m_image->GetResizedImage(width, height);

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromImageCallback());
    m_binarySymbolData.ClearPath();
}


void LogicImage::Load(std::string file_path, bool file_path_is_temporary/* = false*/)
{
    // read and validate the image
    std::unique_ptr<std::vector<std::byte>> content;
    std::optional<std::string> mime_type_override;

    if( Multimedia::Icon::IsExtensionIcon(file_path) )
    {
        content = Multimedia::Icon::LoadIconAsPng(file_path);

        // because the icon is returned as a PNG, don't store the file path
        file_path_is_temporary = true;
        mime_type_override = MimeType::GetType(ImageType::Png);
    }

    else
    {
        content = FileIO::Read(file_path);
    }

    m_image = Multimedia::Image::FromBuffer(*content);

    if( file_path_is_temporary )
    {
        if( !mime_type_override.has_value() )
            mime_type_override = MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path));

        m_binarySymbolData.SetBinaryData(std::move(content), std::string(), ValueOrDefault(std::move(mime_type_override)));
    }

    else
    {
        m_binarySymbolData.SetBinaryData(std::move(content), std::move(file_path));
    }
}


void LogicImage::Load(std::unique_ptr<const Multimedia::Image> image, std::string path_or_filename)
{
    ASSERT(image != nullptr);

    m_image = std::move(image);

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromImageCallback(), std::move(path_or_filename));
}


std::unique_ptr<BinarySymbolDataContentValidator> LogicImage::CreateContentValidator()
{
    class LogicImageContentValidator : public BinarySymbolDataContentValidator
    {
    public:
        bool ValidateContent(std::shared_ptr<const std::vector<std::byte>> content) override
        {
            const std::optional<Multimedia::ImageDetails> image_details = Multimedia::Image::GetDetailsFromBuffer(*content);
            return image_details.has_value();
        }
    };

    return std::make_unique<LogicImageContentValidator>();
}


void LogicImage::LoadFromDataUrl(const std::string_view data_url_sv, BinaryDataMetadata binary_data_metadata/* = BinaryDataMetadata()*/)
{
    const std::unique_ptr<BinarySymbolDataContentValidator> logic_image_content_validator = CreateContentValidator();

    m_binarySymbolData.SetSymbolValueFromDataUrl(*this, data_url_sv, std::move(binary_data_metadata), logic_image_content_validator.get());

    m_image.reset();
}


void LogicImage::Save(std::string file_path, const std::optional<int> jpeg_quality/* = std::nullopt*/)
{
    ASSERT(HasContent());
    ASSERT(!jpeg_quality.has_value() || ( *jpeg_quality >= 0 && *jpeg_quality <= 100 ));

    // if the contents of the image are already in the format requested, we can save the content directly
    if( m_image != nullptr && !jpeg_quality.has_value() &&
        m_image->GetDetails().image_type == MimeType::GetSupportedImageTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path)) )
    {
        FileIO::Write(file_path, m_binarySymbolData.GetContent());
    }

    // otherwise we must save the file using stb_image
    else
    {
        GetParsedImage().ToFile(file_path, jpeg_quality);
    }

    m_binarySymbolData.SetPath(std::move(file_path));
}


void LogicImage::View(const ViewerOptions* const viewer_options) const
{
    ASSERT(HasContent());

    View(m_binarySymbolData.GetContent(),
         ( m_image != nullptr ) ? std::make_optional<Multimedia::ImageDetails>(m_image->GetDetails()) : std::nullopt,
         m_binarySymbolData.CreateFilenameBasedOnMimeType(*this),
         viewer_options);
}


void LogicImage::View(const std::vector<std::byte>& image_content, std::optional<Multimedia::ImageDetails> image_details, const std::string& image_file_path,
                      const ViewerOptions* const viewer_options)
{
    // if the image has not been parsed, try to get the image type, width, height without fully decoding the buffer
    if( !image_details.has_value() )
        image_details = Multimedia::Image::GetDetailsFromBuffer(image_content);

    const std::optional<ImageType> image_type = image_details.has_value() ? image_details->image_type :
                                                                            MimeType::GetSupportedImageTypeFromFileExtension(PortableFunctions::PathGetFileExtension(image_file_path));

    // view the image using a HTML dialog
    ImageViewDlg image_view_dlg(image_content,
                                image_type.has_value() ? MimeType::GetType(*image_type) : std::string(),
                                image_details.has_value() ? std::make_optional(std::make_tuple(image_details->width, image_details->height)) : std::nullopt,
                                image_file_path);

    if( viewer_options == nullptr )
    {
        image_view_dlg.DoModalOnUIThread();
    }

    else
    {
        image_view_dlg.ShowDialogUsingViewer(*viewer_options);
    }
}


void LogicImage::SetValueFromJson(const JsonNode& json_node)
{
    std::unique_ptr<BinarySymbolDataContentValidator> logic_image_content_validator = CreateContentValidator();

    m_binarySymbolData.SetSymbolValueFromJson(*this, json_node, logic_image_content_validator.get());

    m_image.reset();
}
