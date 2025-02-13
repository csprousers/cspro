#include "stdafx.h"
#include "BinarySymbolData.h"
#include "BinarySymbol.h"


BinarySymbolData& BinarySymbolData::operator=(const BinarySymbolData& binary_symbol_data)
{
    if( binary_symbol_data.IsDefined() )
    {
        m_binaryDataAccessor->SetBinaryData(binary_symbol_data.m_binaryDataAccessor->GetBinaryData());
        m_path = binary_symbol_data.m_path;
    }

    else
    {
        Reset();
    }

    return *this;
}


template<typename T> 
void BinarySymbolData::SetBinaryData(T&& content_or_callback, std::wstring path_or_filename)
{
    BinaryDataMetadata binary_data_metadata;

    m_path.clear();

    if( !path_or_filename.empty() )
    {
        binary_data_metadata.SetFilename(path_or_filename);

        // only set the path if there is directory information
        if( !PortableFunctions::PathGetDirectory(path_or_filename).empty() )
            m_path = std::move(path_or_filename);
    }

    SetBinaryData(std::forward<T>(content_or_callback), std::move(binary_data_metadata));
}

template void BinarySymbolData::SetBinaryData(BinaryData::ContentCallbackType&& content_or_callback, std::wstring path_or_filename);
template void BinarySymbolData::SetBinaryData(std::unique_ptr<std::vector<std::byte>>&& content_or_callback, std::wstring path_or_filename);
template void BinarySymbolData::SetBinaryData(std::shared_ptr<const std::vector<std::byte>>&& content_or_callback, std::wstring path_or_filename);


template<typename T> 
void BinarySymbolData::SetBinaryData(T&& content_or_callback, std::wstring path_or_filename, std::wstring mime_type)
{
    SetBinaryData(std::forward<T>(content_or_callback), std::move(path_or_filename));

    if( !mime_type.empty() )
        m_binaryDataAccessor->GetBinaryDataMetadataForModification().SetMimeType(std::move(mime_type));
}

template void BinarySymbolData::SetBinaryData(BinaryData::ContentCallbackType&& content_or_callback, std::wstring path_or_filename, std::wstring mime_type);
template void BinarySymbolData::SetBinaryData(std::unique_ptr<std::vector<std::byte>>&& content_or_callback, std::wstring path_or_filename, std::wstring mime_type);
template void BinarySymbolData::SetBinaryData(std::vector<std::byte>&& content_or_callback, std::wstring path_or_filename, std::wstring mime_type);


void BinarySymbolData::SetPath(std::wstring path)
{
    ASSERT(!path.empty());

    GetMetadataForModification().SetFilename(path);
    m_path = std::move(path);
}


std::wstring BinarySymbolData::GetFilenameOnly() const
{
    std::optional<std::wstring> filename = GetMetadata().GetFilename();
    ASSERT(!filename.has_value() || m_path.empty() || PortableFunctions::PathGetFilename(m_path) == *filename);

    return ValueOrDefault(std::move(filename));
}


std::wstring BinarySymbolData::CreateFilenameBasedOnMimeType(const Symbol* symbol) const
{
    std::wstring filename = GetFilenameOnly();

    if( filename.empty() )
    {
        const std::optional<std::wstring> mime_type = GetMetadata().GetMimeType();

        if( mime_type.has_value() )
        {
            const TCHAR* const extension = MimeType::GetFileExtensionFromType(*mime_type);

            if( extension != nullptr )
            {
                const TCHAR* fake_filename = ( symbol != nullptr ) ? symbol->GetName().c_str() : _T("g");
                return PortableFunctions::PathAppendFileExtension<std::wstring>(fake_filename, extension);
            }
        }
    }

    return filename;
}


void BinarySymbolData::WriteSymbolValueToJson(const BinarySymbol& binary_symbol, JsonWriter& json_writer,
                                              const std::function<void()>* content_writer_override/* = nullptr*/) const
{
    if( !IsDefined() )
    {
        json_writer.WriteNull();
        return;
    }

    json_writer.BeginObject();

    const BinaryDataMetadata& binary_data_metadata = GetMetadata();

    // write the metadata
    {
        json_writer.BeginObject(JK::metadata);

        if( !m_path.empty() )
            json_writer.WritePath(JK::path, m_path);

        binary_data_metadata.WriteJson(json_writer, false);

        json_writer.EndObject();
    }

    // write the content
    {
        json_writer.BeginObject(JK::content);

        SymbolSerializerHelper* symbol_serializer_helper = json_writer.GetSerializerHelper().Get<SymbolSerializerHelper>();
        const JsonProperties::BinaryDataFormat binary_data_format =
            ( symbol_serializer_helper != nullptr ) ? symbol_serializer_helper->GetJsonProperties().GetBinaryDataFormat() :
                                                      JsonProperties::DefaultBinaryDataFormat;

        // write out the content as a Localhost URL...
        if( symbol_serializer_helper != nullptr && binary_data_format == JsonProperties::BinaryDataFormat::LocalhostUrl )
        {
            json_writer.Write(JK::url, symbol_serializer_helper->LocalhostCreateMappingForBinarySymbol(binary_symbol));
        }

        // ...using an override
        else if( content_writer_override != nullptr )
        {
            (*content_writer_override)();
        }

        // ...or as a data URL
        else
        {
            ASSERT(binary_data_format == JsonProperties::BinaryDataFormat::DataUrl || symbol_serializer_helper == nullptr);

            json_writer.Write(JK::url, Encoders::ToDataUrl(GetContent(),
                                                           ValueOrDefault(binary_data_metadata.GetEvaluatedMimeType())));
        }

        json_writer.EndObject();
    }

    json_writer.EndObject();
}


void BinarySymbolData::UpdateSymbolValueFromJson(BinarySymbol& binary_symbol, const JsonNode<wchar_t>& json_node, BinarySymbolDataContentValidator* const content_validator/* = nullptr*/,
                                                 const std::function<BinaryData::ContentCallbackType(const JsonNode<wchar_t>&)>* const non_url_content_reader/* = nullptr*/)
{
    if( json_node.IsNull() || !json_node.Contains(JK::content) )
    {
        binary_symbol.Reset();
        return;
    }

    // parse the metadata
    BinaryDataMetadata binary_data_metadata = json_node.Contains(JK::metadata) ? json_node.Get<BinaryDataMetadata>(JK::metadata) :
                                                                                 BinaryDataMetadata();

    // parse the content...
    const auto& content_node = json_node.Get(JK::content);

    // ...as a data URL
    if( content_node.Contains(JK::url) )
    {
        UpdateSymbolValueFromDataUrl(binary_symbol, content_node.Get<wstring_view>(JK::url), std::move(binary_data_metadata), content_validator);
        return;
    }

    std::optional<std::wstring> path;

    // ...as a path
    if( content_node.Contains(JK::path) )
    {
        path = content_node.GetAbsolutePath(JK::path);
        std::shared_ptr<std::vector<std::byte>> content = FileIO::Read(*path);

        if( content_validator != nullptr && !content_validator->ValidateContent(content) )
            throw CSProException(_T("The binary content for '%s' is not valid."), binary_symbol.GetName().c_str());

        SetBinaryData(std::move(content), std::move(binary_data_metadata));
    }

    // ...or using a different reader
    else if( non_url_content_reader != nullptr )
    {
        SetBinaryData((*non_url_content_reader)(content_node), std::move(binary_data_metadata));
    }

    else
    {
        throw CSProException(_T("The binary content for '%s' is not defined."), binary_symbol.GetName().c_str());
    }

    // the path is only maintained when it was used to retrieve the content
    if( path.has_value() )
    {
        SetPath(std::move(*path));
    }

    else
    {
        ClearPath();
    }
}


void BinarySymbolData::UpdateSymbolValueFromDataUrl(BinarySymbol& binary_symbol, const wstring_view data_url_sv, BinaryDataMetadata binary_data_metadata/* = BinaryDataMetadata()*/,
                                                    BinarySymbolDataContentValidator* const content_validator/* = nullptr*/)
{
    std::shared_ptr<const std::vector<std::byte>> content;
    std::wstring mediatype;
    std::tie(content, mediatype) = Encoders::FromDataUrl(data_url_sv);

    if( ( content == nullptr ) ||
        ( content_validator != nullptr && !content_validator->ValidateContent(content) ) )
    {
        throw CSProException(_T("The binary content for '%s' is not a valid data URL."), binary_symbol.GetName().c_str());
    }

    // use the MIME type from the data URL when possible
    if( !mediatype.empty() )
        binary_data_metadata.SetMimeType(std::move(mediatype));

    SetBinaryData(std::move(content), std::move(binary_data_metadata));

    ClearPath();
}
