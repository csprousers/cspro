#include "StdAfx.h"
#include "Encryption.h"
#include "base64.h"
#include "Hash.h"

extern "C"
{
#include "rijndael/rijndael-alg-fst.h"
}


class Encryptor::Impl
{
public:
    virtual ~Impl() { }

    virtual std::wstring Encrypt(wstring_view text_sv) = 0;
    virtual std::wstring Decrypt(wstring_view encrypted_text_sv) = 0;

    virtual std::vector<std::byte> Encrypt(cs::span<const std::byte> buffer) = 0;
    virtual std::vector<std::byte> Decrypt(cs::span<const std::byte> encrypted_buffer) = 0;
};


// --------------------------------------------------------------------------
// RijndaelEncryptor
// --------------------------------------------------------------------------

class RijndaelEncryptor : public Encryptor::Impl
{
public:
    RijndaelEncryptor(wstring_view key_sv, bool use_base64);

    std::wstring Encrypt(wstring_view text_sv) override;
    std::wstring Decrypt(wstring_view encrypted_text_sv) override;

    std::vector<std::byte> Encrypt(cs::span<const std::byte> buffer) override;
    std::vector<std::byte> Decrypt(cs::span<const std::byte> encrypted_buffer) override;

private:
    using BufferSizeStorage = uint64_t;

    template<bool AddBufferSize>
    std::vector<std::byte> EncryptWorker(cs::span<const std::byte> buffer);

    std::vector<std::byte> DecryptWorker(cs::span<const std::byte> buffer);

    static bool HexStringLengthIsValid(const std::wstring& hex_string)
    {
        return ( hex_string.length() % ( BlockSize * 2 ) == 0 );
    }

private:
    const bool m_useBase64;
    int m_rounds;
    static constexpr size_t KeySize = 4 * ( RIJNDAEL_MAXNR + 1 );
    static constexpr size_t BlockSize = 16;
    uint32_t m_rijndaelEncryptionKey[KeySize]; 
    uint32_t m_rijndaelDecryptionKey[KeySize];
};



RijndaelEncryptor::RijndaelEncryptor(const wstring_view key_sv, const bool use_base64)
    :   m_useBase64(use_base64)
{
    // generate a 256-bit hash from the key
    constexpr size_t HashSizeBits = 256;
    const std::string utf8_key = UTF8Convert::WideToUTF8(key_sv);
    const std::vector<std::byte> key_hash = Hash::Hash(reinterpret_cast<const std::byte*>(utf8_key.data()), utf8_key.length(),
                                                       nullptr, 0, HashSizeBits / 8);
    ASSERT(key_hash.size() == 32);

    // create the Rijndael encryption and decryption keys
    m_rounds = rijndaelKeySetupEnc(m_rijndaelEncryptionKey, reinterpret_cast<const uint8_t*>(key_hash.data()), HashSizeBits);
    ASSERT(m_rounds == RIJNDAEL_MAXNR);

    const int dec_rounds = rijndaelKeySetupDec(m_rijndaelDecryptionKey, reinterpret_cast<const uint8_t*>(key_hash.data()), HashSizeBits);
    ASSERT(m_rounds == dec_rounds);            
}


std::wstring RijndaelEncryptor::Encrypt(const wstring_view text_sv)
{
    // get the text as UTF-8 and add a null terminator
    std::vector<std::byte> utf8_buffer = UTF8Convert::WideToUTF8Buffer(text_sv);
    utf8_buffer.emplace_back(static_cast<std::byte>(0));

    const std::vector<std::byte> encrypted_buffer = EncryptWorker<false>(utf8_buffer);

    // Base64
    if( m_useBase64 )
    {
        return Base64::Encode<std::wstring>(encrypted_buffer);
    }

    // hex
    else
    {
        std::wstring encrypted_text = Hash::BytesToHexString(encrypted_buffer.data(), encrypted_buffer.size());
        ASSERT(HexStringLengthIsValid(encrypted_text));
        return encrypted_text;
    }
}


std::wstring RijndaelEncryptor::Decrypt(const wstring_view encrypted_text_sv)
{
    std::optional<std::vector<std::byte>> utf8_buffer;

    // Base64
    if( m_useBase64 )
    {
        utf8_buffer.emplace(Base64::Decode<wstring_view, std::vector<std::byte>>(encrypted_text_sv));
    }

    // hex (which must be of the correct length)
    else if( HexStringLengthIsValid(encrypted_text_sv) )
    {
        utf8_buffer.emplace(Hash::HexStringToBytes(encrypted_text_sv, false));
    }

    if( utf8_buffer.has_value() )
    {
        const std::vector<std::byte> decrypted_buffer = DecryptWorker(*utf8_buffer);

        // a correctly decrypted string will be null terminated
        if( !decrypted_buffer.empty() && decrypted_buffer.back() == static_cast<std::byte>(0) )
            return UTF8Convert::UTF8ToWide(reinterpret_cast<const char*>(decrypted_buffer.data()));
    }

    return std::wstring();
}


std::vector<std::byte> RijndaelEncryptor::Encrypt(const cs::span<const std::byte> buffer)
{
    return EncryptWorker<true>(buffer);
}


std::vector<std::byte> RijndaelEncryptor::Decrypt(const cs::span<const std::byte> encrypted_buffer)
{
    std::vector<std::byte> decrypted_buffer = DecryptWorker(encrypted_buffer);
    const size_t buffer_size_without_buffer_size_value = decrypted_buffer.size() - sizeof(BufferSizeStorage);

    if( buffer_size_without_buffer_size_value < decrypted_buffer.size() )
    {
        // the buffer size is at the end of the decrypted buffer
        const BufferSizeStorage* const buffer_size = reinterpret_cast<const BufferSizeStorage*>(decrypted_buffer.data() + buffer_size_without_buffer_size_value);

        if( *buffer_size <= buffer_size_without_buffer_size_value )
        {
            decrypted_buffer.resize(static_cast<size_t>(*buffer_size));
            return decrypted_buffer;
        }
    }

    // the decrypted buffer is invalid
    return std::vector<std::byte>();
}


template<bool AddBufferSize>
std::vector<std::byte> RijndaelEncryptor::EncryptWorker(const cs::span<const std::byte> buffer)
{
    size_t real_data_bytes = buffer.size();

    if constexpr(AddBufferSize)
        real_data_bytes += sizeof(BufferSizeStorage);

    // calculate the padding
    size_t padding_required = BlockSize - ( real_data_bytes % BlockSize );

    if( padding_required == BlockSize )
        padding_required = 0;

    const size_t total_bytes = real_data_bytes + padding_required;
    ASSERT(total_bytes % BlockSize == 0);

    std::vector<std::byte> output_buffer(total_bytes);
    std::byte* output_buffer_itr = reinterpret_cast<std::byte*>(output_buffer.data());

    auto encrypt = [&](const std::byte* const input_buffer)
    {
        ASSERT(( output_buffer_itr - output_buffer.data() + BlockSize ) <= total_bytes);
        rijndaelEncrypt(m_rijndaelEncryptionKey, m_rounds, reinterpret_cast<const uint8_t*>(input_buffer), reinterpret_cast<uint8_t*>(output_buffer_itr));
        output_buffer_itr += BlockSize;
    };

    // process the full blocks in the input buffer
    const std::byte* buffer_itr = buffer.data();
    const std::byte* buffer_full_block_end = buffer_itr + ( buffer.size() / BlockSize ) * BlockSize;

    while( buffer_itr != buffer_full_block_end )
    {
        encrypt(buffer_itr);
        buffer_itr += BlockSize;
    }

    const size_t remaining_bytes_from_buffer = buffer.cend() - buffer_full_block_end;
    ASSERT(remaining_bytes_from_buffer == 0 || ( AddBufferSize || padding_required > 0 ));

    const size_t remaining_data_bytes = total_bytes - buffer.size() + remaining_bytes_from_buffer;
    ASSERT(remaining_data_bytes % BlockSize == 0);

    // process the remaining partial block, or the padding, or the buffer size
    if( remaining_data_bytes > 0 )
    {
        // construct the remaining block(s)
        auto remaining_data = std::make_unique<std::byte[]>(remaining_data_bytes);
        std::byte* remaining_data_itr = remaining_data.get();

        if( remaining_bytes_from_buffer > 0 )
        {
            memcpy(remaining_data_itr, buffer_itr, remaining_bytes_from_buffer);
            remaining_data_itr += remaining_bytes_from_buffer;
        }

        if( padding_required > 0 )
        {
            memset(remaining_data_itr, 0, padding_required);
            remaining_data_itr += padding_required;
        }

        if constexpr(AddBufferSize)
        {
            const BufferSizeStorage buffer_size = buffer.size();
            memcpy(remaining_data_itr, &buffer_size, sizeof(BufferSizeStorage));
            remaining_data_itr += sizeof(BufferSizeStorage);
        }

        const std::byte* const remaining_data_end = remaining_data_itr;

        remaining_data_itr = remaining_data.get();
        ASSERT(remaining_data_end == ( remaining_data_itr + remaining_data_bytes ));

        while( remaining_data_itr != remaining_data_end )
        {
            encrypt(remaining_data_itr);
            remaining_data_itr += BlockSize;
        }
    }   

    return output_buffer;
}


std::vector<std::byte> RijndaelEncryptor::DecryptWorker(const cs::span<const std::byte> buffer)
{
    // return if the size of the buffer is not valid
    if( buffer.size() % BlockSize != 0 )
        return std::vector<std::byte>();

    std::vector<std::byte> output_buffer(buffer.size());

    const uint8_t* input_buffer = reinterpret_cast<const uint8_t*>(buffer.data());
    uint8_t* output_buffer_itr = reinterpret_cast<uint8_t*>(output_buffer.data());

    for( size_t num_blocks = buffer.size() / BlockSize; num_blocks > 0; --num_blocks )
    {
        rijndaelDecrypt(m_rijndaelDecryptionKey, m_rounds, input_buffer, output_buffer_itr);
        input_buffer += BlockSize;
        output_buffer_itr += BlockSize;
    }

    return output_buffer;
}



// --------------------------------------------------------------------------
// Encryptor
// --------------------------------------------------------------------------

Encryptor::Encryptor(const Type type, const wstring_view key_sv/* = wstring_view()*/)
{
    switch( type )
    {
        case Type::RijndaelHex:
            m_encryptor = std::make_unique<RijndaelEncryptor>(key_sv, false);
            break;

        case Type::RijndaelBase64:
            m_encryptor = std::make_unique<RijndaelEncryptor>(key_sv, true);
            break;

        default:
            throw ProgrammingErrorException();
    }
}


Encryptor::~Encryptor()
{
}


std::wstring Encryptor::Encrypt(const wstring_view text_sv)
{
    return m_encryptor->Encrypt(text_sv);
}


std::wstring Encryptor::Decrypt(const wstring_view encrypted_text_sv)
{
    return m_encryptor->Decrypt(encrypted_text_sv);
}


std::vector<std::byte> Encryptor::Encrypt(const cs::span<const std::byte> buffer)
{
    return m_encryptor->Encrypt(buffer);
}


std::vector<std::byte> Encryptor::Decrypt(const cs::span<const std::byte> encrypted_buffer)
{
    return m_encryptor->Decrypt(encrypted_buffer);
}
