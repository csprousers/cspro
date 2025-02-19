#pragma once

// an implementation of the .sts status file for storing information about verified cases and partial save statuses;
// data is written to disk whenever modified

class TextRepository;
class WriteCaseParameter;


class TextRepositoryStatusFile
{
    friend TextRepository;

private:
    TextRepositoryStatusFile(TextRepository& repository, DataRepositoryOpenFlag open_flag);

    static std::string GetStatusFilePath(const ConnectionString& connection_string);

public:
    ~TextRepositoryStatusFile();

    void CommitTransactions();

    bool IsPartial(const std::string& key) const;

    bool ContainsPartials() const;

    size_t GetNumberPartials() const;

    void SetUpCase(Case& data_case) const;
    void SetUpCaseSummary(CaseSummary& case_summary) const;

    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter);

    void DeleteCase(const std::string& key);

private:
    void Load();

    void Save(bool force_write_to_disk = false);

    struct Status;
    const Status& LookupStatus(const std::string& key) const;
    Status& GetOrCreateStatus(const std::string& key);

    bool RemoveEntry(const std::string& key);

    struct Status
    {
        std::string case_label;
        bool verified = false;
        PartialSaveMode partial_save_mode = PartialSaveMode::None;
        std::shared_ptr<CaseItemReference> partial_save_case_item_reference;
    };

private:
    TextRepository& m_repository;
    const std::string m_filePath;
    const std::string m_dictionaryName;

    std::unique_ptr<std::map<std::string, Status>> m_statuses;
    const Status m_defaultStatus;

    bool m_hasTransactionsToWrite;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool TextRepositoryStatusFile::IsPartial(const std::string& key) const
{
    const Status& status = LookupStatus(key);
    return ( status.partial_save_mode != PartialSaveMode::None );
}


inline bool TextRepositoryStatusFile::ContainsPartials() const
{
    if( m_statuses != nullptr )
    {
        for( const auto& [key, status] : *m_statuses )
        {
            if( status.partial_save_mode != PartialSaveMode::None )
                return true;
        }
    }

    return false;
}


inline size_t TextRepositoryStatusFile::GetNumberPartials() const
{
    size_t number_partials = 0;

    if( m_statuses != nullptr )
    {
        for( const auto& [key, status] : *m_statuses )
        {
            if( status.partial_save_mode != PartialSaveMode::None )
                ++number_partials;
        }
    }

    return number_partials;
}


inline void TextRepositoryStatusFile::SetUpCase(Case& data_case) const
{
    const Status& status = LookupStatus(data_case.GetKey());

    data_case.SetCaseLabel(status.case_label);
    data_case.SetVerified(status.verified);
    data_case.SetPartialSaveStatus(status.partial_save_mode, status.partial_save_case_item_reference);
}


inline void TextRepositoryStatusFile::SetUpCaseSummary(CaseSummary& case_summary) const
{
    const Status& status = LookupStatus(case_summary.GetKey());

    case_summary.SetCaseLabel(status.case_label);
    case_summary.SetVerified(status.verified);
    case_summary.SetPartialSaveMode(status.partial_save_mode);
}


inline const TextRepositoryStatusFile::Status& TextRepositoryStatusFile::LookupStatus(const std::string& key) const
{
    if( m_statuses != nullptr )
    {
        const auto& status_search = m_statuses->find(key);

        if( status_search != m_statuses->end() )
            return status_search->second;
    }

    return m_defaultStatus;        
}


inline TextRepositoryStatusFile::Status& TextRepositoryStatusFile::GetOrCreateStatus(const std::string& key)
{
    if( m_statuses == nullptr )
        m_statuses = std::make_unique<std::map<std::string, Status>>();

    const auto& status_search = m_statuses->find(key);

    if( status_search != m_statuses->end() )
    {
        return status_search->second;
    }

    else
    {
        return m_statuses->emplace(key, Status()).first->second;
    }
}
