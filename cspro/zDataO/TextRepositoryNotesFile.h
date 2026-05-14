#pragma once

#include <zCaseO/CaseConstructionHelpers.h>

// an implementation of the .csnot notes file for storing information about field notes;
// data is written to disk whenever modified

class TextRepository;
class WriteCaseParameter;


class TextRepositoryNotesFile
{
    friend TextRepository;

public:
    static constexpr size_t FieldLength        = 32;
    static constexpr size_t OperatorIdLength   = 32;
    static constexpr size_t ModifiedDateLength = 8;
    static constexpr size_t ModifiedTimeLength = 6;
    static constexpr size_t OccurrenceLength   = 5;

private:
    TextRepositoryNotesFile(const TextRepository& repository, DataRepositoryOpenFlag open_flag);

    static std::string GetNotesFilePath(const ConnectionString& connection_string);

public:
    ~TextRepositoryNotesFile();

    void CommitTransactions();

    void SetUpCase(Case& data_case) const;
    void SetUpCaseNote(CaseSummary& case_summary) const;

    void WriteCase(Case& data_case, const WriteCaseParameter* write_case_parameter);

    void DeleteCase(const std::string& key);

private:
    void Load();
    void LoadOldFormat(std::string file_path);
    std::string LoadAdjustLevelKey(std::string level_key) const;

    Note& AddNote(std::string first_level_key, std::shared_ptr<NamedReference> named_reference, std::string operator_id,
                  int64_t modified_date_time, SharableString content);

    void Save(bool force_write_to_disk = false);

    const std::vector<Note>* LookupNotes(const std::string& key) const;

    bool RemoveEntry(const std::string& key);

private:
    const TextRepository& m_repository;
    const std::string m_filePath;
    const std::string m_dictionaryName;
    size_t m_firstLevelKeyLength;
    size_t m_allLevelsKeyLength;
    std::vector<size_t> m_secondaryLevelKeyLengths;
    std::unique_ptr<std::map<std::string, std::vector<Note>>> m_notesMap;

    bool m_hasTransactionsToWrite;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void TextRepositoryNotesFile::SetUpCase(Case& data_case) const
{
    const std::vector<Note>* const notes = LookupNotes(data_case.GetKey());

    if( notes != nullptr )
    {
        data_case.SetNotes(*notes);
    }

    else
    {
        data_case.GetNotes().clear();
    }
}


inline void TextRepositoryNotesFile::SetUpCaseNote(CaseSummary& case_summary) const
{
    const std::string* const case_note = CaseConstructionHelpers::LookupCaseNote(m_dictionaryName, LookupNotes(case_summary.GetKey()));

    if( case_note == nullptr )
    {
        case_summary.ResetCaseNote();
    }

    else
    {
        case_summary.SetCaseNote(*case_note);
    }
}


inline const std::vector<Note>* TextRepositoryNotesFile::LookupNotes(const std::string& key) const
{
    if( m_notesMap != nullptr )
    {
        const auto& notes_search = m_notesMap->find(key);

        if( notes_search != m_notesMap->end() )
            return &notes_search->second;
    }

    return nullptr;
}
