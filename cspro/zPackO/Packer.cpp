#include "stdafx.h"
#include "Packer.h"
#include "PackSpec.h"
#include <zToolsO/File.h>
#include <zZip/ZipFile.h>


namespace
{
    constexpr const char* ListingDivider = "--------------------------------------------------------------------------------";
}


// --------------------------------------------------------------------------
//
// PackerImpl
//
// --------------------------------------------------------------------------

class PackerImpl
{
public:
    PackerImpl(const PFF* pff, const PackSpec& pack_spec);

    void Run();

private:
    void WriteLogHeader();
    void WriteLogFilesToPack();
    void CloseLog(bool run_success);

private:
    const PFF* m_pff;
    const PackSpec& m_packSpec;

    std::string m_zipFilePath;
    std::vector<std::string> m_extraFilePaths;
    std::vector<std::string> m_allFilePaths;

    std::unique_ptr<FileIO::TextFile> m_log;
};


PackerImpl::PackerImpl(const PFF* const pff, const PackSpec& pack_spec)
    :   m_pff(pff),
        m_packSpec(pack_spec),
        m_zipFilePath(m_packSpec.GetZipFilePath())
{
    if( m_pff != nullptr )
    {
        ASSERT(m_pff->GetAppType() == APPTYPE::PACK_TYPE);

        // the output filename can be overridden in the PFF
        if( !m_pff->GetPackOutputFName().IsEmpty() )
            m_zipFilePath = UTF8_TODO::GetUtf8(m_pff->GetPackOutputFName());

        // open the optional log file
        if( !m_pff->GetListingFName().IsEmpty() )
        {
            m_log = std::make_unique<FileIO::TextFile>();
            m_log->OpenForTextWritingCreate(m_pff->GetListingFName());
        }
    }
}


void PackerImpl::Run()
{
    try
    {
        // write out the log header
        if( m_log != nullptr )
            WriteLogHeader();

        if( m_zipFilePath.empty() )
            throw CSProException("You must specify a ZIP filename for the packed files.");

        if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(m_zipFilePath), FileExtensions::Zip) )
        {
            throw CSProException("Files can only be packed to ZIP format, so the output file must end in .zip, not '%s'.",
                                 PortableFunctions::PathGetFileExtension(m_zipFilePath, true).c_str());
        }

        // get the extra files specified in the PFF
        if( m_pff != nullptr )
        {
            for( const CString& file_path : m_pff->GetPackExtraFiles() )
                m_extraFilePaths.emplace_back(UTF8_TODO::GetUtf8(file_path));

            VectorHelpers::RemoveDuplicateStringsNoCase(m_extraFilePaths);
        }

        // get all the files
        m_allFilePaths = VectorHelpers::Concatenate(m_packSpec.GetFilePathsForPack(), m_extraFilePaths);
        VectorHelpers::RemoveDuplicateStringsNoCase(m_allFilePaths);

        // write out the expected files to pack
        if( m_log != nullptr )
        {
            WriteLogFilesToPack();
            m_log->WriteLine();
        }

        // make sure all files exist
        int64_t total_input_size = 0;

        for( const std::string& file_path : m_allFilePaths )
        {
            const int64_t file_size = PortableFunctions::FileSize(file_path);

            if( file_size < 0 )
                throw FileIO::Exception::FileNotFound(file_path);

            total_input_size += file_size;
        }

        // create the zip file
        {
            ZipCreator zip_creator(m_zipFilePath);
            zip_creator.AddFiles(m_allFilePaths);
        }

        // write out the listing footer
        if( m_log != nullptr )
        {
            const int64_t compressed_size = PortableFunctions::FileSize(m_zipFilePath);
            ASSERT(compressed_size > 0);

            m_log->WriteFormattedLine("ZIP file created successfully (%d file%s compressed with a space saving of %d%%).",
                                      static_cast<int>(m_allFilePaths.size()), PluralizeWord(m_allFilePaths.size()),
                                      std::max(0, 100 - CreatePercent<int>(compressed_size, total_input_size)));

            CloseLog(true);
        }
    }

    catch( const CSProException& exception )
    {
        if( m_log != nullptr )
        {
            m_log->WriteFormattedLine("*** There was an error packing the files to '%s':", PortableFunctions::PathGetFilename(m_zipFilePath).c_str());
            m_log->WriteLine("***");
            m_log->WriteFormattedLine("*** %s", exception.what());

            CloseLog(false);
        }

        throw exception;
    }
}


void PackerImpl::WriteLogHeader()
{
    ASSERT(m_log != nullptr && m_pff != nullptr);
    bool file_path_written = false;

    auto write_header_file_path = [&](const char* file_type, const cs::string_sz file_path)
    {
        m_log->WriteFormattedLine("%-20s%s", file_type, file_path.c_str());
        file_path_written = true;
    };

    if( !m_pff->GetAppFName().IsEmpty() )
        write_header_file_path(PackSpec::IsPffUsingPackSpec(*m_pff) ? "Pack Specification:" : "Application:", UTF8_TODO::GetUtf8(m_pff->GetAppFName()));

    if( !m_zipFilePath.empty() )
        write_header_file_path("ZIP File:", m_zipFilePath);

    if( file_path_written )
    {
        m_log->WriteLine();
        m_log->WriteLine(ListingDivider);
    }
}


void PackerImpl::WriteLogFilesToPack()
{
    ASSERT(m_log != nullptr);

    constexpr const char* FormatterLevel1 = u8"    • %s";
    constexpr const char* FormatterLevel2 = u8"        • %s";

    m_log->WriteLine();
    m_log->WriteLine("The following inputs are included, along with any dependent files:");

    for( const PackEntry& pack_entry : m_packSpec.GetEntries() )
    {
        m_log->WriteLine();
        m_log->WriteFormattedLine(FormatterLevel1, pack_entry.GetPath().c_str());

        // see what files come as part of this pack entry
        const std::vector<std::tuple<std::string, std::string>> filenames_for_display = pack_entry.GetFilenamesForDisplay();

        if( !filenames_for_display.empty() )
        {
            m_log->WriteLine();

            for( const auto& [path, filename_for_display] : filenames_for_display )
                m_log->WriteFormattedLine(FormatterLevel2, filename_for_display.c_str());
        }
    }

    if( !m_extraFilePaths.empty() )
    {
        m_log->WriteLine();
        m_log->WriteLine("The following additional files are included:");
        m_log->WriteLine();

        for( const std::string& file_path : m_extraFilePaths )
            m_log->WriteFormattedLine(FormatterLevel1, file_path.c_str());
    }

    m_log->WriteLine();
    m_log->WriteLine(ListingDivider);
}


void PackerImpl::CloseLog(const bool run_success)
{
    ASSERT(m_log != nullptr && m_pff != nullptr);

    // close the log and potentially view the listing
    m_log->Close();

    if( m_pff->GetViewListing() == ALWAYS || ( !run_success && m_pff->GetViewListing() == ONERROR ) )
        m_pff->ViewListing();
}



// --------------------------------------------------------------------------
//
// Packer
//
// --------------------------------------------------------------------------

void Packer::Run(const PFF* const pff, const PackSpec& pack_spec)
{
    PackerImpl(pff, pack_spec).Run();
}


void Packer::Run(const PFF& pff, const bool silent)
{
    PackSpec pack_spec = PackSpec::CreateFromPff(pff, silent, true);
    PackerImpl(&pff, pack_spec).Run();
}
