#pragma once

#include <zFreqO/TextFrequencyPrinter.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/StdioFileUnicode.h>


class TextFileFrequencyPrinter : public TextFrequencyPrinter
{
public:
    TextFileFrequencyPrinter(CStdioFileUnicode& file, int listing_width)
        :   TextFrequencyPrinter(FormatType::UsePageLengthAddFormFeedBeforeFirstFrequency, listing_width),
            m_file(file)
    {
    }

    TextFileFrequencyPrinter(NullTerminatedString filename, int listing_width)
        :   TextFrequencyPrinter(FormatType::UsePageLength, listing_width),
            m_ownedFile(std::make_unique<CStdioFileUnicode>()),
            m_file(*m_ownedFile)
    {
        SetupEnvironmentToCreateFile(filename);

        if( !m_ownedFile->Open(filename.c_str(), CFile::modeWrite | CFile::modeCreate) )
            throw CSProException(_T("Could not create the frequency file: %s"), filename.c_str());
    }

    ~TextFileFrequencyPrinter()
    {
        if( m_ownedFile != nullptr )
            m_ownedFile->Close();
    }

protected:
    void WriteLine(NullTerminatedString line) override
    {
        m_file.WriteLine(line.c_str());
    }

private:
    std::unique_ptr<CStdioFileUnicode> m_ownedFile;
    CStdioFileUnicode& m_file;
};
