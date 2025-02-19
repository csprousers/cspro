#pragma once

#include <zJson/zJson.h>
#include <zJson/Json.h>
#include <zToolsO/FileIO.h>


namespace JsonSpecFile
{
    // --------------------------------------------------------------------------
    // writing spec files creation functions
    // --------------------------------------------------------------------------

    // Creates a JsonFileWriter, begins an object, and then calls WriteHeading.
    // - If there is an error writing to the file, a FileIO::Exception exception will be thrown.
    ZJSON_API std::unique_ptr<JsonFileWriter> CreateWriter(InterfaceString file_path, std::string_view file_type_sv);

    // Writes the heading (software, version, and fileType).
    ZJSON_API void WriteHeading(JsonWriter& json_writer, std::string_view file_type_sv);



    // --------------------------------------------------------------------------
    // reading spec files classes
    // --------------------------------------------------------------------------

    class Reader;

    // --------------------------------------------------------------------------
    // JsonSpecFile::ReaderMessageLogger
    // a class that can be used to log warnings (via JsonSpecFile::Reader) and to
    // display messages
    // --------------------------------------------------------------------------
    class ZJSON_API ReaderMessageLogger
    {
        friend class Reader;

    public:
        // Displays any warnings logged during the reading.
        void DisplayWarnings(bool silent = false) const;

        // Rethrows a CSProException with a more detailed message containing any warnings logged during the reading.
        [[noreturn]] void RethrowException(InterfaceString file_path, const CSProException& exception) const;

    private:
        void LogWarning(const std::string& file_path, std::string message);

        std::string GetErrorText(std::string initial_text) const;

    private:
        std::vector<std::tuple<std::string, std::vector<std::string>>> m_messageSets;
    };


    // --------------------------------------------------------------------------
    // JsonSpecFile::Reader
    // a subclass of JsonNode that has methods to help with reading JSON spec
    // files
    // --------------------------------------------------------------------------
    class ZJSON_API Reader : public JsonNode, public JsonReaderInterface
    {
    public:
        Reader(std::string_view json_text_sv, InterfaceString file_path, std::shared_ptr<ReaderMessageLogger> message_logger);

        const std::string& GetFilePath() const { return m_filePath; }

        const ReaderMessageLogger& GetMessageLogger() const           { return *m_messageLogger; }
        std::shared_ptr<ReaderMessageLogger> GetSharedMessageLogger() { return m_messageLogger; }

        // Adds a warning if the file version if greater than the current CSPro version (when the version is defined).
        double CheckVersion();

        // Throws a CSProException if the file type does not match (when the file type is defined).
        // Also throws an exception if the editable flag is set to false and the file is opened in the CSPro Designer.
        void CheckFileType(std::string_view file_type_sv);

    protected:
        // JsonReaderInterface overrides
        void OnLogWarning(std::string message) override
        {
            m_messageLogger->LogWarning(m_filePath, std::move(message));
        }

    private:
        const std::string m_filePath;
        const std::shared_ptr<ReaderMessageLogger> m_messageLogger;
    };



    // --------------------------------------------------------------------------
    // reading spec file creation functions
    // --------------------------------------------------------------------------

    // Creates a reader based on the string view:
    // - Errors interacting with JSON nodes will result in JsonParseException exceptions.
    // - If a message logger is not passed, one will be created.
    ZJSON_API std::unique_ptr<Reader> CreateReader(InterfaceString file_path,
                                                   std::string_view json_text_sv,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger = nullptr);

    // Reads and parse the contents of the file:
    // - If there is an error reading the file, a FileIO::Exception exception will be thrown.
    // - Errors in parsing the entire file will result in a CSProException exception.
    // - Errors interacting with JSON nodes will result in JsonParseException exceptions.
    // - If a message logger is not passed, one will be created.
    ZJSON_API std::unique_ptr<Reader> CreateReader(InterfaceString file_path,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger = nullptr);

    // Reads and parses the contents of the file (as above), but if the file is an old pre-8.0 spec file,
    // the callback function is executed to convert the old file into JSON text.
    ZJSON_API std::unique_ptr<Reader> CreateReader(InterfaceString file_path,
                                                   std::shared_ptr<ReaderMessageLogger> message_logger,
                                                   const std::function<std::string()>& pre_80_spec_file_converter);

    // The pre-8.0 spec file starting character:
    constexpr char Pre80SpecFileStartCharacter = '[';

    // Returns whether or not the file starts with '[' (accounting for a possible BOM),
    // returning false if there are any errors reading the file.
    // The first version assumes the file pointer is at position 0, and will not be
    // reset by the function after accessing the file.
    ZJSON_API bool IsPre80SpecFile(FileIO::FileAndSize& file_and_size);
    ZJSON_API bool IsPre80SpecFile(InterfaceString file_path);
}
