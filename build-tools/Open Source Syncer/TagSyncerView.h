#pragma once

#include <zUtilF/SortListCtrl.h>


class TagSyncerView : public CFormView
{
    DECLARE_DYNCREATE(TagSyncerView)

protected:
    TagSyncerView();

public:
    static std::string ExtractCommitSHAFromTagMessage(std::string_view tag_message_sv);

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnUpdateUI(WPARAM wParam, LPARAM lParam);

    void OnCreateOpenSourceTag();

    void OnViewOpenSourceHistoryLog();
    void OnCreatePrivateHistoryLog();

private:
    void OnRefreshTags();
    void RefreshTags(Controller& controller);

    void OnUpdateTagsUI();

    struct Tag;
    void RunOperationWithTag(bool validate_private_commit, bool validate_open_source_commit,
                             std::function<void(Controller&, const Tag&)> callback_function);

    void DisplayHistoryMarkdown(const std::string& history_md);

private:
    Controller& m_controller;

    CSortListCtrl m_tagsListCtrl;

    std::shared_ptr<const std::vector<Tag>> m_tags;

    std::vector<std::unique_ptr<const SharableString>> m_historyHtml;
};
