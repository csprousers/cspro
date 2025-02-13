#include "stdafx.h"
#include "Image.h"
#include "Document.h"
#include <zUtilF/ImageViewDlg.h>
#include <zMultimediaO/Image.h>


// --------------------------------------------------------------------------
// LogicImage
// --------------------------------------------------------------------------

LogicImage::LogicImage(std::wstring image_name)
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
            throw CSProException(_T("The Document '%s' has data that cannot be converted to an Image."), logic_document.GetName().c_str());
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


bool LogicImage::HasValidImage(bool parse_image_if_necessary) const noexcept
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


void LogicImage::Resample(int width, int height)
{
    ASSERT(HasValidImage(false));
    ASSERT(width > 0 && height > 0);

    m_image = m_image->GetResizedImage(width, height);

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromImageCallback());
    m_binarySymbolData.ClearPath();
}


void LogicImage::Load(std::wstring filename, bool filename_is_temporary/* = false*/)
{
    // read and validate the image
    std::unique_ptr<std::vector<std::byte>> content = FileIO::Read(filename);

    m_image = Multimedia::Image::FromBuffer(*content);

    if( filename_is_temporary )
    {
        m_binarySymbolData.SetBinaryData(std::move(content), std::wstring(),
                                         ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(filename))));
    }

    else
    {
        m_binarySymbolData.SetBinaryData(std::move(content), std::move(filename));
    }
}


void LogicImage::Load(std::unique_ptr<const Multimedia::Image> image, std::wstring filename)
{
    ASSERT(image != nullptr);

    m_image = std::move(image);

    // if saved, the content must be modified
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromImageCallback(), std::move(filename));
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


void LogicImage::LoadFromDataUrl(const wstring_view data_url_sv, BinaryDataMetadata binary_data_metadata/* = BinaryDataMetadata()*/)
{
    std::unique_ptr<BinarySymbolDataContentValidator> logic_image_content_validator = CreateContentValidator();

    m_binarySymbolData.UpdateSymbolValueFromDataUrl(*this, data_url_sv, std::move(binary_data_metadata), logic_image_content_validator.get());
    
    m_image.reset();
}


void LogicImage::Save(std::wstring filename, std::optional<int> jpeg_quality/* = std::nullopt*/)
{
    ASSERT(HasContent());
    ASSERT(!jpeg_quality.has_value() || ( *jpeg_quality >= 0 && *jpeg_quality <= 100 ));

    // if the contents of the image are already in the format requested, we can save the content directly
    if( m_image != nullptr && !jpeg_quality.has_value() &&
        m_image->GetDetails().image_type == MimeType::GetSupportedImageTypeFromFileExtension(PortableFunctions::PathGetFileExtension(filename)) )
    {
        FileIO::Write(filename, m_binarySymbolData.GetContent());
    }

    // otherwise we must save the file using stb_image
    else
    {
        GetParsedImage().ToFile(filename, jpeg_quality);
    }

    m_binarySymbolData.SetPath(std::move(filename));
}


void LogicImage::View(const ViewerOptions* viewer_options) const
{
    ASSERT(HasContent());

    View(m_binarySymbolData.GetContent(),
         ( m_image != nullptr ) ? std::make_optional<Multimedia::ImageDetails>(m_image->GetDetails()) : std::nullopt,
         m_binarySymbolData.CreateFilenameBasedOnMimeType(*this),
         viewer_options);
}


void LogicImage::View(const std::vector<std::byte>& image_content, std::optional<Multimedia::ImageDetails> image_details, const std::wstring& image_filename,
                      const ViewerOptions* viewer_options)
{
    // if the image has not been parsed, try to get the image type, width, height without fully decoding the buffer
    if( !image_details.has_value() )
        image_details = Multimedia::Image::GetDetailsFromBuffer(image_content);

    const std::optional<ImageType> image_type = image_details.has_value() ? image_details->image_type :
                                                                            MimeType::GetSupportedImageTypeFromFileExtension(PortableFunctions::PathGetFileExtension(image_filename));

    // view the image using a HTML dialog
    ImageViewDlg image_view_dlg(image_content,
                                image_type.has_value() ? MimeType::GetType(*image_type) : wstring_view(),
                                image_details.has_value() ? std::make_optional(std::make_tuple(image_details->width, image_details->height)) : std::nullopt,
                                image_filename);

    if( viewer_options == nullptr )
    {
        image_view_dlg.DoModalOnUIThread();
    }

    else
    {
        image_view_dlg.ShowDialogUsingViewer(*viewer_options);
    }
}


void LogicImage::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    std::unique_ptr<BinarySymbolDataContentValidator> logic_image_content_validator = CreateContentValidator();

    m_binarySymbolData.UpdateSymbolValueFromJson(*this, json_node, logic_image_content_validator.get());

    m_image.reset();
}
