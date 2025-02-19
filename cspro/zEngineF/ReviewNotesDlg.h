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
        std::string group_label;
        std::string label;
        SharableString content;
        std::string sort_index;
    };

    ReviewNotesDlg();

    void SetGroupedReviewNotes(std::vector<std::vector<const ReviewNote*>> grouped_review_notes) { m_groupedReviewNotes = std::move(grouped_review_notes); }

    const std::set<const Note*>& GetDeletedNotes() const { return m_deletedNotes; }

    const Note* GetGotoNote() const { return m_gotoNote; }

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    std::vector<std::vector<const ReviewNote*>> m_groupedReviewNotes;
    std::map<uint64_t, const ReviewNote*> m_reviewNoteIndexMap;

    std::set<const Note*> m_deletedNotes;
    const Note* m_gotoNote;
};
