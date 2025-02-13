#include "stdafx.h"
#include "Packer.h"
#include "PackSpec.h"
#include <zToolsO/PointerClasses.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zZipo/IZip.h>


namespace
{
    constexpr const TCHAR* ListingDivider = _T("--------------------------------------------------------------------------------");
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
    void PackFiles();
    std::vector<std::wstring> GetRelativeFilenamesForZip() const;

    void WriteLogHeader();
    void WriteLogFilesToPack();
    void CloseLog(bool run_success);

private:
    const PFF* m_pff;
    const PackSpec& m_packSpec;

    std::wstring m_zipFilename;
    std::vector<std::wstring> m_extraFilenames;
    std::vector<std::wstring> m_allFiles;

    std::unique_ptr<CStdioFileUnicode> m_log;
};


PackerImpl::PackerImpl(const PFF* pff, const PackSpec& pack_spec)
    :   m_pff(pff),
        m_packSpec(pack_spec),
        m_zipFilename(m_packSpec.GetZipFilename())
{
    if( m_pff != nullptr )
    {
        ASSERT(m_pff->GetAppType() == APPTYPE::PACK_TYPE);

        // the output filename can be overriden in the PFF
        if( !m_pff->GetPackOutputFName().IsEmpty() )
            m_zipFilename = CS2WS(m_pff->GetPackOutputFName());

        // open the optional log file
        if( !m_pff->GetListingFName().IsEmpty() )
        {
            m_log = std::make_unique<CStdioFileUnicode>();

            if( !m_log->Open(m_pff->GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
                throw CSProException(_T("There was an error creating the listing file:\n\n%s"), m_pff->GetListingFName().GetString());
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

        if( m_zipFilename.empty() )
            throw CSProException("You must specify a ZIP filename for the packed files.");

        if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(m_zipFilename), _T("zip")) )
        {
            throw CSProException(_T("Files can only be packed to ZIP format, so the output file must end in .zip, not '%s'."),
                                 PortableFunctions::PathGetFileExtension(m_zipFilename, true).c_str());
        }

        // get the extra files specified in the PFF
        if( m_pff != nullptr )
        {
            for( const CString& filename : m_pff->GetPackExtraFiles() )
                m_extraFilenames.emplace_back(CS2WS(filename));

            VectorHelpers::RemoveDuplicateStringsNoCase(m_extraFilenames);
        }        

        // get all the files
        m_allFiles = VectorHelpers::Concatenate(m_packSpec.GetFilenamesForPack(), m_extraFilenames);
        VectorHelpers::RemoveDuplicateStringsNoCase(m_allFiles);

        // write out the expected files to pack
        if( m_log != nullptr )
        {
            WriteLogFilesToPack();
            m_log->WriteLine();
        }

        // make sure all files exist
        int64_t total_input_size = 0;

        for( const std::wstring& filename : m_allFiles )
        {
            const int64_t file_size = PortableFunctions::FileSize(filename.c_str());

            if( file_size < 0 )
                throw FileIO::Exception::FileNotFound(filename.c_str());

            total_input_size += file_size;
        }

        // create the zip file
        PackFiles();

        // write out the listing footer
        if( m_log != nullptr )
        {
            const int64_t compressed_size = PortableFunctions::FileSize(m_zipFilename.c_str());
            ASSERT(compressed_size > 0);

            m_log->WriteFormattedLine(_T("ZIP file created successfully (%d file%s compressed with a space saving of %d%%)."),
                                      static_cast<int>(m_allFiles.size()), PluralizeWord(m_allFiles.size()),
                                      100 - CreatePercent(compressed_size, total_input_size));

            CloseLog(true);
        }
    }

    catch( const CSProException& exception )
    {
        if( m_log != nullptr )
        {
            m_log->WriteFormattedLine(_T("*** There was an error packing the files to '%s':"), PortableFunctions::PathGetFilename(m_zipFilename.c_str()));
            m_log->WriteLine(_T("***"));
            m_log->WriteFormattedLine(_T("*** %s"), exception.GetErrorMessage().c_str());

            CloseLog(false);
        }

        throw exception;
    }
}


void PackerImpl::PackFiles()
{
    const std::vector<std::wstring> filenames_in_zip = GetRelativeFilenamesForZip();
    ASSERT(!filenames_in_zip.empty() && filenames_in_zip.size() == m_allFiles.size());

    // delete the zip file if it already exists
    if( PortableFunctions::FileIsRegular(m_zipFilename.c_str()) && !PortableFunctions::FileDelete(m_zipFilename.c_str()) )
        throw CSProException(_T("There was an error deleting the ZIP file: %s"), m_zipFilename.c_str());

    // if the filename has Unicode characters we have to zip to a temporary filename without those characters
    cs::non_null_shared_or_raw_ptr<const std::wstring> zip_filename_for_creation = &m_zipFilename;

    if( m_zipFilename.size() != UTF8Convert::WideToUTF8(m_zipFilename).size() )
        zip_filename_for_creation = std::make_unique<std::wstring>(GetUniqueTempFilename(_T("CSPack.zip")));

    // zip the files
    try
    {
        std::unique_ptr<IZip> zip(IZip::Create());

        zip->AddFiles(*zip_filename_for_creation, m_allFiles, filenames_in_zip, true);
    }

    catch( CZipError& exception )
    {
        PortableFunctions::FileDelete(zip_filename_for_creation->c_str());
        throw CSProException(exception.GetDetailedErrorMessage());
    }

    // if a non-Unicode filename was used, move the file
    if( zip_filename_for_creation.get() != &m_zipFilename )
    {
        if( !PortableFunctions::FileRename(zip_filename_for_creation->c_str(), m_zipFilename.c_str()) )
            throw CSProException(_T("There was an error renaming the temporary ZIP file: %s"), zip_filename_for_creation->c_str());
    }
}


std::vector<std::wstring> PackerImpl::GetRelativeFilenamesForZip() const
{
    // calculate the common root directory for all files
    std::wstring root_directory;

    for( const std::wstring& filename : m_allFiles )
    {
        const std::wstring directory = PortableFunctions::PathGetDirectory(filename);

        root_directory = root_directory.empty() ? directory :
                                                  PortableFunctions::PathGetCommonRoot(root_directory, directory);

        if( root_directory.empty() )
            throw CSProException("It is not possible to create a ZIP file when the files exist on two or more drives.");
    }

    // make sure that the root directory ends with a slash
    root_directory = PortableFunctions::PathEnsureTrailingSlash(root_directory);

    // create the relative filenames for the zip file
    std::vector<std::wstring> filenames_in_zip;

    for( const std::wstring& filename : m_allFiles )
    {
        filenames_in_zip.emplace_back(filename.substr(root_directory.size()));

        ASSERT(filenames_in_zip.back().find(_T(":\\")) == std::wstring::npos &&
               filenames_in_zip.back().find(_T("\\\\")) == std::wstring::npos);
    }

    return filenames_in_zip;
}


void PackerImpl::WriteLogHeader()
{
    ASSERT(m_log != nullptr && m_pff != nullptr);
    bool filename_written = false;

    auto write_header_filename = [&](const TCHAR* file_type, const TCHAR* filename)
    {
        m_log->WriteFormattedLine(_T("%-20s%s"), file_type, filename);
        filename_written = true;
    };

    if( !m_pff->GetAppFName().IsEmpty() )
        write_header_filename(PackSpec::IsPffUsingPackSpec(*m_pff) ? _T("Pack Specification:") : _T("Application:"), m_pff->GetAppFName());

    if( !m_zipFilename.empty() )
        write_header_filename(_T("ZIP File:"), m_zipFilename.c_str());

    if( filename_written )
    {
        m_log->WriteLine();
        m_log->WriteLine(ListingDivider);
    }
}


void PackerImpl::WriteLogFilesToPack()
{
    ASSERT(m_log != nullptr);

    constexpr const TCHAR* FormatterLevel1 = _T("    • %s");
    constexpr const TCHAR* FormatterLevel2 = _T("        • %s");

    m_log->WriteLine();
    m_log->WriteLine(_T("The following inputs are included, along with any dependent files:"));

    for( const PackEntry& pack_entry : m_packSpec.GetEntries() )
    {
        m_log->WriteLine();
        m_log->WriteFormattedLine(FormatterLevel1, pack_entry.GetPath().c_str());

        // see what files come as part of this pack entry
        std::vector<std::tuple<std::wstring, std::wstring>> filenames_for_display = pack_entry.GetFilenamesForDisplay();

        if( !filenames_for_display.empty() )
        {
            m_log->WriteLine();

            for( const auto& [path, filename_for_display] : filenames_for_display )
                m_log->WriteFormattedLine(FormatterLevel2, filename_for_display.c_str());
        }
    }

    if( !m_extraFilenames.empty() )
    {
        m_log->WriteLine();
        m_log->WriteLine(_T("The following additional files are included:"));
        m_log->WriteLine();

        for( const std::wstring& filename : m_extraFilenames )
            m_log->WriteFormattedLine(FormatterLevel1, filename.c_str());
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

void Packer::Run(const PFF* pff, const PackSpec& pack_spec)
{
    PackerImpl(pff, pack_spec).Run();
}


void Packer::Run(const PFF& pff, const bool silent)
{
    PackSpec pack_spec = PackSpec::CreateFromPff(pff, silent, true);
    PackerImpl(&pff, pack_spec).Run();
}
