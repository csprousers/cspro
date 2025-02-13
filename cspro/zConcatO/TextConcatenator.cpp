#include "stdafx.h"
#include "TextConcatenator.h"
#include "ConcatenatorHelpers.h"
#include "ConcatenatorReporter.h"
#include <zToolsO/Utf8Convert.h>
#include <zDataO/DataRepositoryDefines.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr uint64_t BufferSize = 65536;
}


void TextConcatenator::Run(ConcatenatorReporter& concatenator_reporter,
                           const std::vector<ConnectionString>& input_connection_strings,
                           const ConnectionString& output_connection_string)
{
    if( output_connection_string.GetType() != DataRepositoryType::Text )
    {
        throw CSProException(_T("The output of text concatenation must be to a text file, not to a file of type '%s'."),
                             ToString(output_connection_string.GetType()));
    }


    // calculate the size of each file
    std::vector<std::optional<uint64_t>> file_sizes;
    uint64_t total_file_size;
    std::tie(file_sizes, total_file_size) = ConcatenatorHelpers::CalculateFileSizes(input_connection_strings);

#ifdef WIN32
    // on Windows, see if there is enough room for this file
    const std::wstring volume = PathGetVolume(output_connection_string.GetFilename());

    if( volume.length() == 3 && volume[1] == ':' )
    {
        ASSERT(volume[2] == '\\');

        ULONG sectors_per_cluster;
        ULONG bytes_per_second;
        ULONG free_clusters;
        ULONG total_clusters;
        GetDiskFreeSpace(volume.c_str(), &sectors_per_cluster, &bytes_per_second, &free_clusters, &total_clusters);

        const uint64_t free_size = static_cast<uint64_t>(sectors_per_cluster) *
                                   static_cast<uint64_t>(bytes_per_second) *
                                   static_cast<uint64_t>(free_clusters);

        if( free_size < total_file_size )
        {
            throw CSProException(_T("The concatenated file requires at least %s bytes but only %s bytes are available."),
                                 IntToString(total_file_size).GetString(), IntToString(free_size).GetString());
        }
    }
#endif


    // create the output file using a temporary filename and write the UTF-8 BOM
    const ConnectionString temporary_output_connection_string = ConcatenatorHelpers::GetTemporaryOutputConnectionString(output_connection_string);

    CFile output_file;

    if( !output_file.Open(temporary_output_connection_string.GetFilename().c_str(), CFile::modeWrite | CFile::modeCreate) )
    {
        throw CSProException(_T("There was an error creating the output file: %s"),
                            temporary_output_connection_string.GetFilename().c_str());
    }

    output_file.Write(Utf8BOM_sv.data(), Utf8BOM_sv.length());


    // concatenate each file
    std::vector<std::byte> read_buffer;
    std::vector<std::byte> conversion_buffer;
    uint64_t processed_bytes = 0;

    try
    {
        for( size_t i = 0; i < input_connection_strings.size(); ++i )
        {
            if( concatenator_reporter.IsCanceled() )
                throw UserCanceledException();

            const ConnectionString& input_connection_string = input_connection_strings[i];
            const std::optional<uint64_t>& file_size = file_sizes[i];

            // skip files that don't exist
            if( !file_sizes[i].has_value() )
            {
                concatenator_reporter.ErrorFileOpenFailed(input_connection_string);
                continue;
            }

            // only text files can be concatenated
            if( input_connection_string.GetType() != DataRepositoryType::Text )
            {
                concatenator_reporter.ErrorInvalidEncoding(input_connection_string);
                processed_bytes += *file_size;
                continue;
            }

            // check the file's encoding
            Encoding encoding = Encoding::Invalid;

            if( *file_size == 0 )
            {
                encoding = Encoding::Ansi;
            }

            else
            {
                GetFileBOM(input_connection_string.GetFilename(), encoding);

                if( encoding != Encoding::Ansi && encoding != Encoding::Utf8 && encoding != Encoding::Utf16LE )
                {
                    concatenator_reporter.ErrorInvalidEncoding(input_connection_string);
                    processed_bytes += *file_size;
                    continue;
                }
            }

            // determine the reading and conversion parameters
            static_assert(BufferSize % sizeof(wchar_t) == 0);
            static_assert(sizeof(wchar_t) == ( OnAndroidOrWasm() ? 4 : 2 ));

            size_t read_buffer_size_needed = BufferSize;
            size_t conversion_buffer_size_needed;
            size_t bom_size;

            if( encoding == Encoding::Ansi )
            {
                // every byte in an ANSI file will map to a wide character and then back to at most two UTF-8 bytes
                read_buffer_size_needed = BufferSize * 2;
                conversion_buffer_size_needed = BufferSize * sizeof(wchar_t);
                bom_size = 0;
            }

            else if( encoding == Encoding::Utf8 )
            {
                conversion_buffer_size_needed = 0;
                bom_size = Utf8BOM_sv.length();
            }

            else // if( encoding == Encoding::Utf16LE )
            {
                // every UTF-16 character will map to at most four UTF-8 bytes
                conversion_buffer_size_needed = BufferSize * 4;
                bom_size = 2;
            }

            if( read_buffer.size() < read_buffer_size_needed )
                read_buffer.resize(read_buffer_size_needed);

            if( conversion_buffer.size() < conversion_buffer_size_needed )
                conversion_buffer.resize(conversion_buffer_size_needed);

            processed_bytes += bom_size;

            // only process non-empty files
            if( *file_size != bom_size )
            {
                CFile input_file;

                if( !input_file.Open(input_connection_string.GetFilename().c_str(), CFile::modeRead) )
                {
                    concatenator_reporter.ErrorFileOpenFailed(input_connection_string);
                    processed_bytes += *file_size - bom_size;
                    continue;
                }

                // do the concatenation
                concatenator_reporter.SetSource(ConcatenatorHelpers::GetReporterSourceText(input_connection_strings, i));

                // skip past the BOM
                if( bom_size != 0 )
                    input_file.Seek(bom_size, CFile::begin);

                // read the data in blocks
                uint64_t remaining_bytes = *file_size - bom_size;
                bool trim_trailing_characters = false;

                while( remaining_bytes > 0 )
                {
                    // read the text
                    UINT bytes_read;

                    if( remaining_bytes > BufferSize )
                    {
                        bytes_read = input_file.Read(read_buffer.data(), BufferSize);
                        ASSERT(bytes_read == BufferSize);
                    }

                    else
                    {
                        bytes_read = input_file.Read(read_buffer.data(), static_cast<UINT>(remaining_bytes));
                        ASSERT(bytes_read == remaining_bytes);
                        trim_trailing_characters = true;
                    }

                    remaining_bytes -= bytes_read;
                    processed_bytes += bytes_read;

                    // convert the text
                    std::byte* write_buffer;
                    UINT bytes_to_write;

                    if( encoding == Encoding::Ansi )
                    {
                        // ANSI data must be converted to wide characters and then to UTF-8
                        const size_t wide_characters = UTF8Convert::EncodedCharsBufferToWideBuffer(Encoding::Ansi,
                                                                                                   reinterpret_cast<const char*>(read_buffer.data()), bytes_read,
                                                                                                   reinterpret_cast<wchar_t*>(conversion_buffer.data()), conversion_buffer.size() / sizeof(wchar_t));

                        bytes_to_write = UTF8Convert::WideBufferToUTF8Buffer(reinterpret_cast<const wchar_t*>(conversion_buffer.data()), wide_characters,
                                                                             reinterpret_cast<char*>(read_buffer.data()), read_buffer.size());
                        write_buffer = read_buffer.data();
                    }

                    else if( encoding == Encoding::Utf8 )
                    {
                        // UTF-8 characters can be copied directly
                        bytes_to_write = bytes_read;
                        write_buffer = read_buffer.data();
                    }

                    else // if( encoding == Encoding::Utf16LE )
                    {
#ifdef WIN32
                        // wide characters must be converted to UTF-8
                        bytes_to_write = UTF8Convert::WideBufferToUTF8Buffer(reinterpret_cast<const wchar_t*>(read_buffer.data()), bytes_read / sizeof(wchar_t),
                                                                             reinterpret_cast<char*>(conversion_buffer.data()), conversion_buffer.size());
#else
                        // on Android/WASM, two-byte characters need to be preprocessed because wchar_t is four bytes
                        static_assert(sizeof(uint16_t) == 2);
                        const std::wstring wide_string = TwoByteCharToWide(reinterpret_cast<const uint16_t*>(read_buffer.data()), bytes_read / 2);
                        ASSERT(wide_string.length() == ( bytes_read / 2));

                        bytes_to_write = UTF8Convert::WideBufferToUTF8Buffer(wide_string.c_str(), wide_string.length(),
                                                                             reinterpret_cast<char*>(conversion_buffer.data()), conversion_buffer.size());
#endif
                        write_buffer = conversion_buffer.data();
                    }

                    // remove any trailing non-printable characters
                    if( trim_trailing_characters )
                    {
                        static_assert(sizeof(char) == sizeof(std::byte));

                        for( const std::byte* itr = write_buffer + bytes_to_write - 1;
                             itr >= write_buffer && static_cast<char>(*itr) < ' ';
                             --itr )
                        {
                            --bytes_to_write;
                        }
                    }

                    // write the text
                    output_file.Write(write_buffer, bytes_to_write);


                    // check for cancelation and update the progress bar
                    if( concatenator_reporter.IsCanceled() )
                        throw UserCanceledException();

                    concatenator_reporter.GetProcessSummary().IncrementAttributesRead(bytes_to_write);

                    concatenator_reporter.GetProcessSummary().SetPercentSourceRead(CreatePercent(processed_bytes, total_file_size));
                }

                // add a newline after each file
                output_file.Write("\r\n", 2);

                input_file.Close();
            }

            concatenator_reporter.AddSuccessfullyProcessedFile(input_connection_string);
        }

        ASSERT(processed_bytes == total_file_size);
    }

    catch(...)
    {
        output_file.Close();
        PortableFunctions::FileDelete(temporary_output_connection_string.GetFilename());
        throw;
    }


    // after a success concatenation, rename the temporary file to the output filename
    output_file.Close();

    DataRepositoryHelpers::RenameRepository(temporary_output_connection_string, output_connection_string);
}
