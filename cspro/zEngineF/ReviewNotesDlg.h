#pragma once

#include <zEngineF/zEngineF.h>
#include <zHtml/CSHtmlDlgRunner.h>
#include <zCaseO/Note.h>


class CLASS_DECL_ZENGINEF ReviewNotesDlg : public CSHtmlDlgRunner
{
public:
    struct ReviewNote
    {
        Note note;
        bool can_goto = false;
        int group_symbol_index = -1;
        std::wstring group_label;
        std::wstring label;
        std::wstring content;
        std::wstring sort_index;
    };

    ReviewNotesDlg();

    void SetGroupedReviewNotes(std::vector<std::vector<const ReviewNote*>> grouped_review_notes) { m_groupedReviewNotes = std::move(grouped_review_notes); }

    const std::set<const Note*>& GetDeletedNotes() const { return m_deletedNotes; }

    const Note* GetGotoNote() const { return m_gotoNote; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::vector<std::vector<const ReviewNote*>> m_groupedReviewNotes;
    std::map<uint64_t, const ReviewNote*> m_reviewNoteIndexMap;

    std::set<const Note*> m_deletedNotes;
    const Note* m_gotoNote;
};
