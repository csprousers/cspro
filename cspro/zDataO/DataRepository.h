#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepositoryDefines.h>
#include <zDataO/DataRepositoryException.h>
#include <zUtilO/ConnectionString.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseKey.h>

class CaseIterator;
class CaseSummary;
class ISyncableDataRepository;
class WriteCaseParameter;


class ZDATAO_API DataRepository
{
protected:
    DataRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);

    bool IsReadOnly() const { return ( m_accessType == DataRepositoryAccess::BatchInput || m_accessType == DataRepositoryAccess::ReadOnly ); }

    virtual void Open(DataRepositoryOpenFlag open_flag) = 0;

public:
    virtual ~DataRepository();

    /// <summary>
    /// Create a new repository based on the type that comes from a connection string.
    /// </summary>
    static std::unique_ptr<DataRepository> Create(std::shared_ptr<const CaseAccess> case_access,
                                                  const ConnectionString& connection_string,
                                                  DataRepositoryAccess access_type);

    /// <summary>
    /// Create and open a new repository based on the type that comes from a connection string .
    /// </summary>
    static std::unique_ptr<DataRepository> CreateAndOpen(std::shared_ptr<const CaseAccess> case_access,
                                                         const ConnectionString& connection_string,
                                                         DataRepositoryAccess access_type, DataRepositoryOpenFlag open_flag);

    /// <summary>
    /// Returns the repository type.
    /// </summary>
    DataRepositoryType GetRepositoryType() const { return m_type; }

    /// <summary>
    /// Returns the current connection string.
    /// </summary>
    const ConnectionString& GetConnectionString() const { return m_connectionString; }

    /// <summary>
    /// Returns the case access associated with repository.
    /// </summary>
    const CaseAccess* GetCaseAccess() const                       { return m_caseAccess.get(); }
    std::shared_ptr<const CaseAccess> GetSharedCaseAccess() const { return m_caseAccess; }

    /// <summary>
    /// Returns the real underlying repository, not a wrapper repository like ParadataWrapperRepository.
    /// </summary>
    virtual const DataRepository& GetRealRepository() const { return *this; }
    virtual DataRepository& GetRealRepository()             { return *this; }

    /// <summary>
    /// Returns the repository, or nullptr if the repository does not support data synchronization.
    /// </summary>
    virtual const ISyncableDataRepository* GetSyncableDataRepository() const { return nullptr; }
    virtual ISyncableDataRepository* GetSyncableDataRepository()             { return nullptr; }

    /// <summary>
    /// Opens the source using the access parameters previously specified when creating the repository. If
    /// open_flag is CreateNew, any existing cases will be removed from the repository.
    /// </summary>
    void Open(const ConnectionString& connection_string, DataRepositoryOpenFlag open_flag);

    /// <summary>
    /// Returns a name that combines the repository type as well as information about the source. This
    /// name can be used when printing out information about the repository in listing files.
    /// </summary>
    CString GetName(DataRepositoryNameType name_type) const;

public:
    /// <summary>
    /// Modifies the case access used by the repository. The dictionary will never change
    /// for the repository but the other access parameters may.
    /// </summary>
    virtual void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) = 0;

    /// <summary>
    /// Closes the repository. The repository will not be opened again after a Close and the only expected
    /// behavior is that the destructor will be executed.
    /// </summary>
    virtual void Close() = 0;

    /// <summary>
    /// Delete the repository. If the repository is disk-based, it will be removed from the disk.
    /// </summary>
    virtual void DeleteRepository() = 0;

    /// <summary>
    /// Returns whether or not a case with the given key exists in the repository.
    /// </summary>
    virtual bool ContainsCase(const CString& key) const = 0;

    /// <summary>
    /// Searches for a case using one of three (generally unique) identifiers and then populates the
    /// other identifiers. If the key is not empty, it is used in the search; otherwise, if the UUID is not empty,
    /// it is used in the search; if both string values are empty, then the position in the repository is used.
    /// When searching using the key, deleted cases will not be processed, but the other search types will look at
    /// deleted cases. If the case does not exist, DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    virtual void PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const = 0;

    /// <summary>
    /// Searches for a case key using the search rules. If no key is found, std::nullopt is returned.
    /// </summary>
    virtual std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                               const CaseIteratorParameters* start_parameters = nullptr) const = 0;

    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                       const CaseIteratorParameters& start_parameters) const
    {
        return FindCaseKey(iteration_method, iteration_order, &start_parameters);
    }

    /// <summary>
    /// Reads the non-deleted case with the given key. If no case with the given key is in the repository,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    virtual void ReadCase(Case& data_case, const CString& key) = 0;

    /// <summary>
    /// Reads the case at the given position in the repository. The position is a number returned by
    /// Case::GetPositionInRepository. If no case at the given position is in the repository,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    virtual void ReadCase(Case& data_case, double position_in_repository) = 0;

    /// <summary>
    /// Writes the case using rules based on the access parameters previously specified when creating
    /// the repository.
    /// </summary>
    virtual void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) = 0;

    /// <summary>
    /// Modifies the case's deleted status. If the case does not exist,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    virtual void DeleteCase(double position_in_repository, bool deleted = true) = 0;

    /// <summary>
    /// Deletes the case. If the case does not exist,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    virtual void DeleteCase(const CString& key);

    /// <summary>
    /// Returns the number of non-deleted cases in the repository.
    /// </summary>
    virtual size_t GetNumberCases() const = 0;

    /// <summary>
    /// Gets the number of cases matching the specified parameters.
    /// </summary>
    virtual size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) const = 0;

    /// <summary>
    /// Returns an iterator that can be used to process all of the cases in the repository matching the
    /// specified parameters. The iteration is optimized for the specified iteration content, but can
    /// be used to read any of the applicable objects.
    /// </summary>
    virtual std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                         std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
                                                         const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) = 0;

    /// <summary>
    /// Returns an iterator that can be used to process all of the cases in the repository.
    /// </summary>
    std::unique_ptr<CaseIterator> CreateCaseIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order);

    /// <summary>
    /// Returns an iterator that can be used to process all of the case keys in the repository.
    /// </summary>
    std::unique_ptr<CaseIterator> CreateCaseKeyIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order);

    /// <summary>
    /// Starts a transaction in the repository. A transaction does not need to be started to modify
    /// the repository, but wrapping writes or deletes in a transaction may speed any such modifications.
    /// </summary>
    virtual void StartTransaction() { }

    /// <summary>
    /// Ends a transaction in the repository.
    /// </summary>
    virtual void EndTransaction() { }


protected:
    DataRepositoryType m_type;
    std::shared_ptr<const CaseAccess> m_caseAccess;
    DataRepositoryAccess m_accessType;
    ConnectionString m_connectionString;


    // CR_TODO remove all below...
public:
    void ReadCasetainer(Case* pCasetainer, CString key);
    void ReadCasetainer(Case& casetainer, CString key) { ReadCasetainer(&casetainer, key); }

    void ReadCasetainer(Case* pCasetainer, double position_in_repository);
    void ReadCasetainer(Case& casetainer, double position_in_repository) { ReadCasetainer(&casetainer, position_in_repository); }

    void WriteCasetainer(Case* pCasetainer, WriteCaseParameter* write_case_parameter = nullptr);
};
