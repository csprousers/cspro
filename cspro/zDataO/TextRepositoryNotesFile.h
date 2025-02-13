#pragma once

#include <zCaseO/CaseConstructionHelpers.h>

// an implementation of the .csnot notes file for storing information about field notes;
// data is written to disk whenever modified

class TextRepository;
class WriteCaseParameter;


class TextRepositoryNotesFile
{
    friend TextRepository;

private:
    TextRepositoryNotesFile(const TextRepository& repository, DataRepositoryOpenFlag open_flag);

    static std::wstring GetNotesFilename(const ConnectionString& connection_string);

public:
    ~TextRepositoryNotesFile();

    void CommitTransactions();

    void SetupCase(Case& data_case) const;
    void SetupCaseNote(CaseSummary& case_summary) const;

    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter);

    void DeleteCase(const CString& key);

private:
    void Load(const TextRepository& repository);
    void LoadOldFormat(const TextRepository& repository, NullTerminatedString filename);

    Note& AddNote(const CString& first_level_key, std::shared_ptr<NamedReference> named_reference, const CString& operator_id,
                  const time_t& modified_date_time, const CString& content);

    void Save(bool force_write_to_disk = false);

    const std::vector<Note>* LookupNotes(const CString& key) const;

    bool RemoveEntry(const CString& key);

private:
    const std::wstring m_filename;
    const CString m_dictionaryName;
    size_t m_firstLevelKeyLength;
    size_t m_allLevelsKeyLength;
	std::vector<size_t> m_secondaryLevelKeyLengths;
    std::unique_ptr<std::map<CString, std::vector<Note>>> m_notesMap;

    bool m_useTransactionManager;
    bool m_hasTransactionsToWrite;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void TextRepositoryNotesFile::SetupCase(Case& data_case) const
{
    const std::vector<Note>* notes = LookupNotes(data_case.GetKey());

    if( notes != nullptr )
    {
        data_case.SetNotes(*notes);
    }

    else
    {
        data_case.GetNotes().clear();
    }
}


inline void TextRepositoryNotesFile::SetupCaseNote(CaseSummary& case_summary) const
{
    const std::vector<Note>* notes = LookupNotes(case_summary.GetKey());
    case_summary.SetCaseNote(( notes != nullptr ) ? CaseConstructionHelpers::LookupCaseNote(m_dictionaryName, *notes) : CString());
}


inline const std::vector<Note>* TextRepositoryNotesFile::LookupNotes(const CString& key) const
{
    if( m_notesMap != nullptr )
    {
        const auto& notes_search = m_notesMap->find(key);

        if( notes_search != m_notesMap->end() )
            return &notes_search->second;
    }

    return nullptr;
}
