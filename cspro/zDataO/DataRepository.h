#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepositoryDefines.h>
#include <zDataO/DataRepositoryException.h>
#include <zToolsO/UniqueId.h>
#include <zUtilO/ConnectionString.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseKey.h>

class CaseIterator;
class CaseIteratorSettings;
class CaseSummary;
class DataRepositoryUniqueCaseIdentifer;
class ISyncableDataRepository;
class WriteCaseParameter;


class ZDATAO_API DataRepository
{
protected:
    DataRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);

    bool IsReadOnly() const { return ( m_accessType == DataRepositoryAccess::BatchInput || m_accessType == DataRepositoryAccess::ReadOnly ); }

    virtual void Open(DataRepositoryOpenFlag open_flag) = 0;

public:
    virtual ~DataRepository() { }

    // Creates a new repository based on the type that comes from a connection string.
    static std::unique_ptr<DataRepository> Create(std::shared_ptr<const CaseAccess> case_access,
                                                  const ConnectionString& connection_string,
                                                  DataRepositoryAccess access_type);

    // Creates and opens a new repository based on the type that comes from a connection string.
    static std::unique_ptr<DataRepository> CreateAndOpen(std::shared_ptr<const CaseAccess> case_access,
                                                         const ConnectionString& connection_string,
                                                         DataRepositoryAccess access_type, DataRepositoryOpenFlag open_flag);

    // Returns a unique ID that identifies this instance of a data repository object.
    const UniqueId& GetRepositoryId() { return m_repositoryId; }

    // Returns the repository type.
    DataRepositoryType GetRepositoryType() const { return m_type; }

    // Returns the repository access.
    DataRepositoryAccess GetRepositoryAccess() const { return m_accessType; }

    // Returns the current connection string.
    const ConnectionString& GetConnectionString() const { return m_connectionString; }

    // Returns the case access associated with repository.
    const CaseAccess& GetCaseAccess() const                       { return *m_caseAccess; }
    std::shared_ptr<const CaseAccess> GetSharedCaseAccess() const { return m_caseAccess; }

    // Returns the real underlying repository, not a wrapper repository like ParadataWrapperRepository.
    virtual DataRepository& GetRealRepository() { return *this; }

    // Returns the repository, or null if the repository does not support data synchronization.
    virtual ISyncableDataRepository* GetSyncableDataRepository() { return nullptr; }

    // Opens the source using the access parameters previously specified when creating the repository.
    // If open_flag is CreateNew, any existing cases will be removed from the repository.
    void Open(const ConnectionString& connection_string, DataRepositoryOpenFlag open_flag);

    // Returns a name that combines the repository type as well as information about the source.
    // This name can be used when printing out information about the repository in listing files.
    std::string GetName(DataRepositoryNameType name_type) const;

    // Modifies the case access used by the repository. The dictionary will never change
    // for the repository but the other access parameters may.
    virtual void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) = 0;

    // Toggles the access from DataRepositoryAccess::ReadOnly to DataRepositoryAccess::ReadWrite,
    // or vice versa, after a repository has been opened in one of those modes. The case access is
    // left unchanged. If the mode cannot be toggled, an exception is thrown and the only expected
    // behavior is that Close and the destructor will be executed, so the repository can be left in
    // a bad state.
    virtual void ToggleReadWriteMode() = 0;

    // Closes the repository. The repository will not be opened again after a Close and
    // the only expected behavior is that the destructor will be executed.
    virtual void Close() = 0;

    // Deletes the repository. If the repository is disk-based, it will be removed from the disk.
    virtual void DeleteRepository() = 0;

    // Returns whether or not a non-deleted case with the given key exists in the repository.
    virtual bool ContainsCase(const std::string& key) = 0;

    // Searches for a case using one of three (generally unique) identifiers and then populates the
    // other identifiers. If the key is not empty, it is used in the search; otherwise, if the UUID is not empty,
    // it is used in the search; if both string values are empty, then the position in the repository is used.
    // When searching using the key, deleted cases will not be processed, but the other search types will look at
    // deleted cases. If the case does not exist, DataRepositoryException::CaseNotFound will be thrown.
    virtual void PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository) = 0;

    // Returns an identifier that uniquely identifies a case in instances when the repository changes positions.
    virtual DataRepositoryUniqueCaseIdentifer GetUniqueCaseIdentifer(const CaseKey& case_key) = 0;

    // Searches for a case key using the search rules. If no key is found, std::nullopt is returned.
    virtual std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                               const CaseIteratorParameters* start_parameters = nullptr) = 0;

    // Reads the non-deleted case with the given key. If no case with the given key is in the repository,
    // DataRepositoryException::CaseNotFound will be thrown.
    virtual void ReadCase(Case& data_case, const std::string& key) = 0;

    // Reads the case at the given position in the repository. The position is a number returned by
    // Case::GetPositionInRepository. If no case at the given position is in the repository,
    // DataRepositoryException::CaseNotFound will be thrown.
    virtual void ReadCase(Case& data_case, double position_in_repository) = 0;

    // Reads the case at the given UUID. If no case with the given UUID is in the repository,
    // DataRepositoryException::CaseNotFound will be thrown.
    virtual void ReadCaseByUuid(Case& data_case, const std::string& uuid) = 0;

    // Writes the case using rules based on the access parameters previously specified when creating the repository.
    // The case's deleted status may be set; if so, the case is written as deleted (when applicable).
    virtual void WriteCase(Case& data_case, const WriteCaseParameter* write_case_parameter = nullptr) = 0;

    // Modifies the case's deleted status. If the case does not exist,
    // DataRepositoryException::CaseNotFound will be thrown.
    virtual void DeleteCase(double position_in_repository, bool deleted = true) = 0;

    // Deletes the case. If the case does not exist,
    // DataRepositoryException::CaseNotFound will be thrown.
    // The default implementation uses PopulateCaseIdentifiers to determine the position and then calls the position-based DeleteCase.
    virtual void DeleteCase(const std::string& key);

    // Returns the number of non-deleted cases in the repository.
    virtual size_t GetNumberCases() = 0;

    // Gets the number of cases matching the specified parameters.
    virtual size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) = 0;

    // Returns an iterator that can be used to process all of the cases in the repository matching the
    // specified parameters. The iteration is optimized for the specified iteration content, but can
    // be used to read any of the applicable objects.
    virtual std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content,
                                                         const CaseIteratorSettings& iterator_settings,
                                                         size_t offset = 0, size_t limit = SIZE_MAX) = 0;

    // Returns an iterator that can be used to process all of the cases in the repository.
    std::unique_ptr<CaseIterator> CreateCaseIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order);

    // Returns an iterator that can be used to process all of the case keys in the repository.
    std::unique_ptr<CaseIterator> CreateCaseKeyIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order);

    // Starts a transaction in the repository. A transaction does not need to be started to modify
    // the repository, but wrapping writes or deletes in a transaction may speed any such modifications.
    virtual void StartTransaction() { }

    // Ends a transaction in the repository.
    virtual void EndTransaction() { }


protected:
    UniqueId m_repositoryId;
    const DataRepositoryType m_type;
    std::shared_ptr<const CaseAccess> m_caseAccess; // non-null
    DataRepositoryAccess m_accessType;
    ConnectionString m_connectionString;


    // --------------------------------------------------------------------------
    // CR_TODO remove all below...
    // --------------------------------------------------------------------------
public:
    void ReadCasetainer(Case& casetainer, const std::string& key);
    void ReadCasetainer(Case& casetainer, double position_in_repository);
    void WriteCasetainer(Case& casetainer, const WriteCaseParameter* write_case_parameter = nullptr);
    static bool NextCasetainer(CaseIterator& case_iterator, Case& data_case);
};
