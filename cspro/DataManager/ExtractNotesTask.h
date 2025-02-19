#pragma once

#include <DataManager/CaseTask.h>
#include <DataManager/ExtractNotesSettings.h>


class ExtractNotesTask : public CaseTask
{
public:
    static constexpr const char* NamePrefix = "NOTES_";

public:
    ExtractNotesTask(ExtractNotesSettings settings);
    ~ExtractNotesTask();

protected:
    // Task and CaseTask overrides
    void Initialize() override;
    void ProcessCase(Case& data_case) override;
    void Finalize(Result result) override;

private:
    static std::unique_ptr<CDataDict> CreateNotesDictionary(const CDataDict& source_dictionary);

    struct IdLink;
    void SetUpIdLinks();

    struct NoteLinks;
    void SetUpNoteLinks();

    const CaseLevel* FindCaseLevel(Case& data_case, const std::string& level_key) const;

    void CopyNotesToCase(Case& data_case, const std::vector<const Note*>& notes);

private:
    ExtractNotesSettings m_settings;

    std::unique_ptr<const CDataDict> m_notesDictionary;
    std::shared_ptr<const CaseAccess> m_notesCaseAccess;
    std::shared_ptr<DataRepository> m_notesDataRepository;
    std::unique_ptr<Case> m_notesCase;

    std::unique_ptr<std::vector<IdLink>> m_idLinks;
    std::unique_ptr<NoteLinks> m_noteLinks;
    size_t m_casesWithNotes;
    size_t m_extractedNotes;
};
