#include "stdafx.h"
#include "Document.h"
#include "Audio.h"
#include "Geometry.h"
#include "Image.h"
#include <zUtilO/TemporaryFile.h>
#include <zMultimediaO/Image.h>


// --------------------------------------------------------------------------
// LogicDocument
// --------------------------------------------------------------------------

LogicDocument::LogicDocument(std::wstring document_name)
    :   BinarySymbol(std::move(document_name), SymbolType::Document)
{
}


LogicDocument::LogicDocument(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   BinarySymbol(engine_item, std::move(item_index), std::move(binary_data_accessor))
{
}


LogicDocument::LogicDocument(const LogicDocument& logic_document)
    :   BinarySymbol(logic_document)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


std::unique_ptr<Symbol> LogicDocument::CloneInInitialState() const
{
    return std::unique_ptr<LogicDocument>(new LogicDocument(*this));
}


LogicDocument& LogicDocument::operator=(const LogicDocument& logic_document)
{
    if( this != &logic_document )
        m_binarySymbolData = logic_document.m_binarySymbolData;

    return *this;
}


LogicDocument& LogicDocument::operator=(const LogicAudio& logic_audio)
{
    m_binarySymbolData = logic_audio.GetBinarySymbolData();
    return *this;
}


LogicDocument& LogicDocument::operator=(const LogicGeometry& logic_geometry)
{
    m_binarySymbolData = logic_geometry.GetBinarySymbolData();
    return *this;
}


LogicDocument& LogicDocument::operator=(const LogicImage& logic_image)
{
    m_binarySymbolData = logic_image.GetBinarySymbolData();
    return *this;
}


LogicDocument& LogicDocument::operator=(const std::wstring& document_text)
{
    m_binarySymbolData.SetBinaryData(UTF8Convert::WideToUTF8Buffer(document_text), std::wstring(), MimeType::Type::Text);

    BinaryDataMetadata& binary_data_metadata = m_binarySymbolData.GetMetadataForModification();
    binary_data_metadata.SetProperty(_T("label"), document_text);
    binary_data_metadata.SetProperty(_T("source"), _T("Document=string"));
    binary_data_metadata.SetProperty(_T("timestamp"), GetTimestamp());

    return *this;
}


void LogicDocument::Load(std::wstring filename)
{
    std::unique_ptr<std::vector<std::byte>> content = FileIO::Read(filename);
    m_binarySymbolData.SetBinaryData(std::move(content), std::move(filename));
}


void LogicDocument::Save(std::wstring filename)
{
    ASSERT(HasContent());

    FileIO::Write(filename, m_binarySymbolData.GetContent());

    m_binarySymbolData.SetPath(std::move(filename));
}


bool LogicDocument::View(const ViewerOptions* viewer_options) const
{
    ASSERT(HasContent());

    // if this is an image, display it as images are displayed
    std::optional<Multimedia::ImageDetails> image_details = Multimedia::Image::GetDetailsFromBuffer(m_binarySymbolData.GetContent());

    if( image_details.has_value() )
    {
        LogicImage::View(m_binarySymbolData.GetContent(),
                         std::move(image_details),
                         m_binarySymbolData.CreateFilenameBasedOnMimeType(*this),
                         viewer_options);
        return true;
    }

    // otherwise use the standard system viewer
    else
    {
        // if the file exists on the disk, use it
        std::wstring filename_to_view = m_binarySymbolData.GetPath();

        // otherwise save the file to a temporary file
        if( filename_to_view.empty() || !PortableFunctions::FileIsRegular(filename_to_view) )
        {
            filename_to_view = m_binarySymbolData.CreateFilenameBasedOnMimeType(*this);

            if( filename_to_view.empty() )
                return false;

            filename_to_view = GetUniqueTempFilename(filename_to_view);

            try
            {
                FileIO::Write(filename_to_view, m_binarySymbolData.GetContent());
            }

            catch(...)
            {
                return false;
            }

            TemporaryFile::RegisterFileForDeletion(filename_to_view);
        }

        ASSERT(PortableFunctions::FileIsRegular(filename_to_view));

        Viewer viewer;
        return viewer.UseEmbeddedViewer()
                     .SetOptions(viewer_options)
                     .ViewFile(filename_to_view);
    }
}
