#pragma once

#include <zFreqO/TextFrequencyPrinter.h>
#include <zToolsO/TextFile.h>


class TextFileFrequencyPrinter : public TextFrequencyPrinter
{
public:
    TextFileFrequencyPrinter(FileIO::TextFile& text_file, const int listing_width)
        :   TextFrequencyPrinter(FormatType::UsePageLengthAddFormFeedBeforeFirstFrequency, listing_width),
            m_textFile(&text_file)
    {
    }

    TextFileFrequencyPrinter(const std::string& file_path, const int listing_width)
        :   TextFrequencyPrinter(FormatType::UsePageLength, listing_width),
            m_textFile(std::make_unique<FileIO::TextFile>())
    {
        SetupEnvironmentToCreateFile(file_path);

        m_textFile->OpenForTextWritingCreate(file_path); // TEXT_ENCODING_TODO refactor to use connection strings with properties
    }

protected:
    void WriteLine(const std::string_view line_sv) override
    {
        m_textFile->WriteLine(line_sv);
    }

private:
    cs::non_null_shared_or_raw_ptr<FileIO::TextFile> m_textFile;
};
