#include "stdafx.h"
#include "ExportWriterRepository.h"
#include "NullRepositoryIterators.h"
#include <zExportO/CSProExportWriter.h>
#include <zExportO/DelimitedTextExportWriter.h>
#include <zExportO/ExcelExportWriter.h>
#include <zExportO/RExportWriter.h>
#include <zExportO/SasExportWriter.h>
#include <zExportO/SpssExportWriter.h>
#include <zExportO/StataExportWriter.h>


ExportWriterRepository::ExportWriterRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
    :   DataRepository(type, std::move(case_access), access_type)
{
}


ExportWriterRepository::~ExportWriterRepository()
{
    // ignore any exceptions that may occur while closing the repository
    try
    {
        Close();
    }

    catch( const CSProException& ) { }
}


void ExportWriterRepository::LogInvalidAccess(const TCHAR* access_message, const Case* data_case/* = nullptr*/) const
{
    CaseConstructionReporter* case_construction_reporter = ( data_case != nullptr ) ? data_case->GetCaseConstructionReporter() :
                                                                                      nullptr;

    if( case_construction_reporter == nullptr )
        case_construction_reporter = m_caseAccess->GetCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->IssueMessage(MessageType::Warning, 31103, ToString(m_type), access_message);
}


void ExportWriterRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    ASSERT(false);
    m_caseAccess = std::move(case_access);
}


void ExportWriterRepository::Open(DataRepositoryOpenFlag open_flag)
{
    ASSERT(DataRepositoryHelpers::IsTypeExportWriter(m_type));

    if( open_flag == DataRepositoryOpenFlag::OpenMustExist )
    {
        throw DataRepositoryException::IOError(FormatText(_T("Files of type '%s' cannot be opened as input files"), ToString(m_type)));
    }

    if( m_accessType != DataRepositoryAccess::BatchOutput &&
        m_accessType != DataRepositoryAccess::BatchOutputAppend &&
        m_accessType != DataRepositoryAccess::ReadWrite )
    {
        throw DataRepositoryException::IOError(FormatText(_T("Files of type '%s' can only be opened as files that will receive output"), ToString(m_type)));
    }

    try
    {
        if( m_type == DataRepositoryType::CommaDelimited ||
            m_type == DataRepositoryType::SemicolonDelimited ||
            m_type == DataRepositoryType::TabDelimited )
        {
            m_exportWriter = std::make_unique<DelimitedTextExportWriter>(m_type, m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::Excel )
        {
            m_exportWriter = std::make_unique<ExcelExportWriter>(m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::CSProExport )
        {
            m_exportWriter = std::make_unique<CSProExportWriter>(m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::R )
        {
            m_exportWriter = std::make_unique<RExportWriter>(m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::SAS )
        {
            m_exportWriter = std::make_unique<SasExportWriter>(m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::SPSS )
        {
            m_exportWriter = std::make_unique<SpssExportWriter>(m_caseAccess, m_connectionString);
        }

        else if( m_type == DataRepositoryType::Stata )
        {
            m_exportWriter = std::make_unique<StataExportWriter>(m_caseAccess, m_connectionString);
        }

        else
        {
            throw ProgrammingErrorException();
        }
    }

    catch( const CSProException& exception )
    {
        // rethrow as a DataRepositoryException
        throw DataRepositoryException::IOError(exception.GetErrorMessage());
    }
}


void ExportWriterRepository::Close()
{
    if( m_exportWriter != nullptr )
    {
        m_exportWriter->Close();
        m_exportWriter.reset();
    }
}


void ExportWriterRepository::DeleteRepository()
{
    Close();

    for( const std::wstring& filename : GetExportFilenames(m_connectionString) )
        PortableFunctions::FileDelete(filename);
}


void ExportWriterRepository::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    std::vector<std::wstring> old_filenames = GetExportFilenames(old_connection_string);
    std::vector<std::wstring> new_filenames = GetExportFilenames(new_connection_string);
    ASSERT(old_filenames.size() == new_filenames.size());

    // this routine is not perfect (because the newly renamed SAS syntax file will have a reference to the old
    // transport filename), but at the moment this function is not used in any context where that would matter
    for( size_t i = 0; i < old_filenames.size(); ++i )
    {
        if( ( PortableFunctions::FileIsRegular(new_filenames[i]) && !PortableFunctions::FileDelete(new_filenames[i]) ) ||
            ( PortableFunctions::FileIsRegular(old_filenames[i]) && !PortableFunctions::FileRename(old_filenames[i], new_filenames[i]) ) )
        {
            throw DataRepositoryException::RenameRepositoryError();
        }
    }
}


std::vector<std::wstring> ExportWriterRepository::GetAssociatedFileList(const ConnectionString& connection_string)
{
    return GetExportFilenames(connection_string);
}


bool ExportWriterRepository::ContainsCase(const CString& /*key*/) const
{
    LogInvalidAccess(_T("search for cases"));
    return false;
}


void ExportWriterRepository::PopulateCaseIdentifiers(CString& /*key*/, CString& /*uuid*/, double& /*position_in_repository*/) const
{
    LogInvalidAccess(_T("search for cases"));
    throw DataRepositoryException::CaseNotFound();
}


std::optional<CaseKey> ExportWriterRepository::FindCaseKey(CaseIterationMethod /*iteration_method*/, CaseIterationOrder /*iteration_order*/,
    const CaseIteratorParameters* /*start_parameters = nullptr*/) const
{
    LogInvalidAccess(_T("search for cases"));
    return std::nullopt;
}


void ExportWriterRepository::ReadCase(Case& data_case, const CString& /*key*/)
{
    LogInvalidAccess(_T("load cases"), &data_case);
    throw DataRepositoryException::CaseNotFound();
}


void ExportWriterRepository::ReadCase(Case& data_case, double /*position_in_repository*/)
{
    LogInvalidAccess(_T("load cases"), &data_case);
    throw DataRepositoryException::CaseNotFound();
}


void ExportWriterRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    ASSERT(write_case_parameter == nullptr);

    m_exportWriter->WriteCase(data_case);
}


void ExportWriterRepository::DeleteCase(double /*position_in_repository*/, bool /*deleted = true*/)
{
    LogInvalidAccess(_T("delete cases"));
    throw DataRepositoryException::CaseNotFound();
}


size_t ExportWriterRepository::GetNumberCases() const
{
    LogInvalidAccess(_T("count cases"));
    return 0;
}


size_t ExportWriterRepository::GetNumberCases(CaseIterationCaseStatus /*case_status*/, const CaseIteratorParameters* /*start_parameters= nullptr*/) const
{
    LogInvalidAccess(_T("count cases"));
    return 0;
}


std::unique_ptr<CaseIterator> ExportWriterRepository::CreateIterator(CaseIterationContent /*iteration_content*/, CaseIterationCaseStatus /*case_status*/,
    std::optional<CaseIterationMethod> /*iteration_method*/, std::optional<CaseIterationOrder> /*iteration_order*/,
    const CaseIteratorParameters* /*start_parameters = nullptr*/, size_t /*offset = 0*/, size_t /*limit = SIZE_MAX*/)
{
    LogInvalidAccess(_T("iterate over cases"));
    return std::make_unique<NullRepositoryCaseIterator>();
}
