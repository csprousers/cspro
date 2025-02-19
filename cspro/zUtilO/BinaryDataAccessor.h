#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/BinaryData.h>

class BinaryContentReader;


// --------------------------------------------------------------------------
// BinaryDataAccessor
//
// This class wraps what should evaluate to a BinaryData object but allows
// for the lazy loading of content by using a BinaryContentReader.
//
// The signature is the lowercase MD5 of the content (length 32). It is
// assumed that a signature, if not empty, is valid for any content.
//
// When using a binary content reader, the reader will be reset once the
// content has been queried or modified.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO BinaryDataAccessor
{
public:
    // Creates an object without defined binary data.
    BinaryDataAccessor() noexcept { }

    // Creates an object with defined binary data.
    BinaryDataAccessor(BinaryData binary_data);

    // Creates an object with defined binary data that will be accessed using a binary content reader.
    // The content will only be loaded on demand.
    BinaryDataAccessor(BinaryDataMetadata binary_data_metadata, std::string signature, std::shared_ptr<BinaryContentReader> binary_content_reader);

    BinaryDataAccessor(const BinaryDataAccessor&) = default;
    BinaryDataAccessor(BinaryDataAccessor&& rhs) noexcept = default;

    BinaryDataAccessor& operator=(const BinaryDataAccessor&) = default;
    BinaryDataAccessor& operator=(BinaryDataAccessor&&) noexcept = default;


    // Returns true if the binary data is defined (not empty).
    bool IsDefined() const noexcept { return m_data.has_value(); }

    // Returns true if the binary data is defined and the content has been loaded.
    // If a binary content reader was used, it indicates that the content has been retrieved.
    bool IsDefinedAndContentLoaded() const noexcept { return ( m_data.has_value() && std::holds_alternative<BinaryData>(*m_data) ); }

    // Clears the binary data, resetting the binary data to an undefined (empty) state.
    void Clear() noexcept { m_data.reset(); m_signature.clear(); }


    // Returns the binary data metadata.
    // An exception will only be thrown if the binary data is undefined (empty).
    const BinaryDataMetadata& GetBinaryDataMetadata() const;
    BinaryDataMetadata& GetBinaryDataMetadata();

    // Returns the binary content reader (if applicable).
    const BinaryContentReader* GetBinaryContentReader() const noexcept;
    BinaryContentReader* GetBinaryContentReader() noexcept;

    // Returns the size of the binary data, querying it from the binary content reader if necessary.
    // An exception can be thrown when using a binary content reader, or if the binary data is undefined (empty).
    uint64_t GetBinaryDataSize() const;

    // Returns the binary data, loading it using the binary content reader if necessary.
    // An exception can be thrown when using a binary content reader, or if the binary data is undefined (empty).
    const BinaryData& GetBinaryData() const;
    BinaryData& GetBinaryData();

    // Returns the signature of the content. If the content is not coming from a binary content reader,
    // it will be calculated (if not already set). The signature is blank for undefined (empty) data.
    const std::string& GetSignature() const;

    // Returns true if the signature is defined properly.
    static bool IsValidSignature(std::string_view signature_sv);

private:
    using ReaderData = std::tuple<BinaryDataMetadata, std::shared_ptr<BinaryContentReader>>;

    mutable std::optional<std::variant<BinaryData, ReaderData>> m_data;
    mutable std::string m_signature;
};
