#pragma once

#include <zJson/zJson.h>
#include <zJson/Json.h>
#include <zToolsO/FileIO.h>
    

namespace JsonSpecFile
{
    // --------------------------------------------------
    // writing spec files creation functions
    // --------------------------------------------------

    // creates a JsonFileWriter, begins an object, and then calls WriteHeading
    // - if there is an error writing to the file, a FileIO::Exception exception will be thrown
    ZJSON_API std::unique_ptr<JsonFileWriter> CreateWriter(NullTerminatedString filename, wstring_view file_type);

    // writes the heading (software, version, and fileType)
    ZJSON_API void WriteHeading(JsonWriter& json_writer, wstring_view file_type);



    // --------------------------------------------------
    // reading spec files classes
    // --------------------------------------------------

    class Reader;

    // JsonSpecFile::ReaderMessageLogger
    // a class that can be used to log warnings (via
    // JsonSpecFile::Reader) and to display messages
    // --------------------------------------------------
    class ZJSON_API ReaderMessageLogger
    {
        friend class Reader;

    public:
        // displays any warnings logged during the reading
        void DisplayWarnings(bool silent = false) const;

        // rethrows a CSProException with a more detailed message containing any warnings logged during the reading
        [[noreturn]] void RethrowException(NullTerminatedString filename, const CSProException& exception) const;

    private:
        void LogWarning(const std::wstring& filename, std::wstring message);

        std::wstring GetErrorText(std::wstring initial_text) const;

    private:
        std::vector<std::tuple<std::wstring, std::vector<std::wstring>>> m_messageSets;
    };


    // JsonSpecFile::Reader
    // a subclass of JsonNode that has methods to help
    // with reading JSON spec files
    // --------------------------------------------------
    class ZJSON_API Reader : public JsonNode<TCHAR>, public JsonReaderInterface
    {
    public:
        Reader(std::basic_string_view<TCHAR> json_text, std::wstring filename, std::shared_ptr<ReaderMessageLogger> message_logger);

        const std::wstring& GetFilename() const { return m_filename; }

        const ReaderMessageLogger& GetMessageLogger() const           { return *m_messageLogger; }
        std::shared_ptr<ReaderMessageLogger> GetSharedMessageLogger() { return m_messageLogger; }

        // adds a warning if the file version if greater than the current CSPro version (when the version is defined)
        double CheckVersion();

        // throws a CSProException if the file type does not match (when the file type is defined);
        // also throws an exception if the editable flag is set to false and the file is opened in the CSPro Designer
        void CheckFileType(wstring_view file_type);

    protected:
        // JsonReaderInterface overrides
        void OnLogWarning(std::wstring message) override
        {
            m_messageLogger->LogWarning(m_filename, std::move(message));
        }

    private:
        std::wstring m_filename;
        std::shared_ptr<ReaderMessageLogger> m_messageLogger;
    };        



    // --------------------------------------------------
    // reading spec file creation functions
    // --------------------------------------------------

    // creates a reader based on the string view:
    // - errors interacting with JSON nodes will result in JsonParseException exceptions
    // - if a message logger is not passed, one will be created
    ZJSON_API std::unique_ptr<Reader> CreateReader(NullTerminatedString filename,
                                                   wstring_view json_text,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger = nullptr);

    // reads and parse the contents of the file:
    // - if there is an error reading the file, a FileIO::Exception exception will be thrown
    // - errors in parsing the entire file will result in a CSProException exception
    // - errors interacting with JSON nodes will result in JsonParseException exceptions
    // - if a message logger is not passed, one will be created
    ZJSON_API std::unique_ptr<Reader> CreateReader(NullTerminatedString filename,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger = nullptr);

    // reads and parses the contents of the file (as above), but if the file is an old pre-8.0 spec file,
    // the callback function is executed to convert the old file into JSON text
    ZJSON_API std::unique_ptr<Reader> CreateReader(NullTerminatedString filename,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger,
                                                   const std::function<std::wstring()>& pre_80_spec_file_converter);

    // the pre-8.0 spec file starting character
    constexpr char Pre80SpecFileStartCharacter = '[';

    // returns whether or not the file starts with '[' (accounting for a possible BOM),
    // returning false if there are any errors reading the file;
    // the first version assumes the file pointer is at position 0, and will not be
    // reset by the function after accessing the file
    ZJSON_API bool IsPre80SpecFile(FileIO::FileAndSize& file_and_size);
    ZJSON_API bool IsPre80SpecFile(NullTerminatedString filename);
}
