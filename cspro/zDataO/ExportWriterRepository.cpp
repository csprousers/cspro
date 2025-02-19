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


ExportWriterRepository::ExportWriterRepository(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
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


void ExportWriterRepository::LogInvalidAccess(const char* const access_message, const Case* const data_case/* = nullptr*/) const
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


void ExportWriterRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    ASSERT(DataRepositoryHelpers::IsTypeExportWriter(m_type));

    if( open_flag == DataRepositoryOpenFlag::OpenMustExist )
    {
        throw DataRepositoryException::IOError("Files of type '%s' cannot be opened as input files", ToString(m_type));
    }

    if( m_accessType != DataRepositoryAccess::BatchOutput &&
        m_accessType != DataRepositoryAccess::BatchOutputAppend &&
        m_accessType != DataRepositoryAccess::ReadWrite )
    {
        throw DataRepositoryException::IOError("Files of type '%s' can only be opened as files that will receive output", ToString(m_type));
    }

    try
    {
        switch( m_type )
        {
            case DataRepositoryType::CommaDelimited:
            case DataRepositoryType::SemicolonDelimited:
            case DataRepositoryType::TabDelimited:
                m_exportWriter = std::make_unique<DelimitedTextExportWriter>(m_type, m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::Excel:
                m_exportWriter = std::make_unique<ExcelExportWriter>(m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::CSProExport:
                m_exportWriter = std::make_unique<CSProExportWriter>(m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::R:
                m_exportWriter = std::make_unique<RExportWriter>(m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::SAS:
                m_exportWriter = std::make_unique<SasExportWriter>(m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::SPSS:
                m_exportWriter = std::make_unique<SpssExportWriter>(m_caseAccess, m_connectionString);
                break;

            case DataRepositoryType::Stata:
                m_exportWriter = std::make_unique<StataExportWriter>(m_caseAccess, m_connectionString);
                break;

            default:
                throw ProgrammingErrorException();
        }
    }

    catch( const CSProException& exception )
    {
        // rethrow as a DataRepositoryException
        throw DataRepositoryException::IOError(exception.what());
    }
}


void ExportWriterRepository::ToggleReadWriteMode()
{
    throw ProgrammingErrorException();
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

    for( const std::string& file_path : GetExportFilePaths(m_connectionString) )
        PortableFunctions::FileDelete(file_path);
}


void ExportWriterRepository::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    const std::vector<std::string> old_file_paths = GetExportFilePaths(old_connection_string);
    const std::vector<std::string> new_file_paths = GetExportFilePaths(new_connection_string);
    ASSERT(old_file_paths.size() == new_file_paths.size());

    // this routine is not perfect (because the newly renamed SAS syntax file will have a reference to the old
    // transport filename), but at the moment this function is not used in any context where that would matter
    for( size_t i = 0; i < old_file_paths.size(); ++i )
    {
        if( ( PortableFunctions::FileIsRegular(new_file_paths[i]) && !PortableFunctions::FileDelete(new_file_paths[i]) ) ||
            ( PortableFunctions::FileIsRegular(old_file_paths[i]) && !PortableFunctions::FileRename(old_file_paths[i], new_file_paths[i]) ) )
        {
            throw DataRepositoryException::RenameRepositoryError();
        }
    }
}


std::vector<std::string> ExportWriterRepository::GetAssociatedFileList(const ConnectionString& connection_string)
{
    return GetExportFilePaths(connection_string);
}


bool ExportWriterRepository::ContainsCase(const std::string& /*key*/)
{
    LogInvalidAccess("search for cases");
    return false;
}


void ExportWriterRepository::PopulateCaseIdentifiers(std::string& /*key*/, std::string& /*uuid*/, double& /*position_in_repository*/)
{
    LogInvalidAccess("search for cases");
    throw DataRepositoryException::CaseNotFound();
}


DataRepositoryUniqueCaseIdentifer ExportWriterRepository::GetUniqueCaseIdentifer(const CaseKey& /*case_key*/)
{
    LogInvalidAccess("search for cases");
    throw DataRepositoryException::CaseNotFound();
}


std::optional<CaseKey> ExportWriterRepository::FindCaseKey(CaseIterationMethod /*iteration_method*/, CaseIterationOrder /*iteration_order*/,
                                                           const CaseIteratorParameters* /*start_parameters = nullptr*/)
{
    LogInvalidAccess("search for cases");
    return std::nullopt;
}


void ExportWriterRepository::ReadCase(Case& data_case, const std::string& /*key*/)
{
    LogInvalidAccess("load cases", &data_case);
    throw DataRepositoryException::CaseNotFound();
}


void ExportWriterRepository::ReadCase(Case& data_case, double /*position_in_repository*/)
{
    LogInvalidAccess("load cases", &data_case);
    throw DataRepositoryException::CaseNotFound();
}


void ExportWriterRepository::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    ASSERT(write_case_parameter == nullptr);

    m_exportWriter->WriteCase(data_case);
}


void ExportWriterRepository::DeleteCase(double /*position_in_repository*/, bool /*deleted = true*/)
{
    LogInvalidAccess("delete cases");
    throw DataRepositoryException::CaseNotFound();
}


size_t ExportWriterRepository::GetNumberCases()
{
    LogInvalidAccess("count cases");
    return 0;
}


size_t ExportWriterRepository::GetNumberCases(CaseIterationCaseStatus /*case_status*/, const CaseIteratorParameters* /*start_parameters = nullptr*/)
{
    LogInvalidAccess("count cases");
    return 0;
}


std::unique_ptr<CaseIterator> ExportWriterRepository::CreateIterator(CaseIterationContent /*iteration_content*/, CaseIterationCaseStatus /*case_status*/,
                                                                     std::optional<CaseIterationMethod> /*iteration_method*/, std::optional<CaseIterationOrder> /*iteration_order*/,
                                                                     const CaseIteratorParameters* /*start_parameters = nullptr*/, size_t /*offset = 0*/, size_t /*limit = SIZE_MAX*/)
{
    LogInvalidAccess("iterate over cases");
    return std::make_unique<NullRepositoryCaseIterator>();
}
