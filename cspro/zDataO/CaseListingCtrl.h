#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/CaseIteratorSettings.h>


class ZDATAO_API CaseListingCtrl : public CListCtrl
{
public:
    CaseListingCtrl();
    virtual ~CaseListingCtrl();

    void Initialize(std::shared_ptr<DataRepository> data_depository,
                    std::shared_ptr<const ViewableCaseIteratorSettings> viewable_case_iterator_settings);

    void UpdateCaseListing(bool change_is_only_visual = false);

    // returns the number of cases to which the iterator's settings apply
    const std::optional<size_t>& GetNumberCases() const { return m_numberCases; }

    const std::vector<std::shared_ptr<const CaseSummary>>& GetSelectedCaseSummaries();

protected:
    // methods that subclasses can override
    virtual void OnCaseListingCaseSummariesQueried(std::variant<size_t, const char*> number_cases_or_exception);
    virtual void OnCaseListingSelectionsChanged();
    virtual void OnCaseListingDoubleClickAndReturn();
    virtual void OnCaseListingContextMenu(CPoint point);
    virtual bool OnCaseListingDeleteKey();

protected:
    DECLARE_MESSAGE_MAP()

    void PreSubclassWindow() override;

    BOOL PreTranslateMessage(MSG* pMsg) override;

    void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

    void OnCacheHint(NMHDR* pNMHDR, LRESULT* pResult);
    void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

    void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
    void OnRightClick(NMHDR* pNMHDR, LRESULT* pResult);

    void OnContextMenu();

    LRESULT OnAdjustColumnWidthAndInvalidate(WPARAM wParam, LPARAM lParam);
    LRESULT OnSelectionsChanged(WPARAM wParam, LPARAM lParam);
    LRESULT OnRequeryCaseSummaries(WPARAM wParam, LPARAM lParam);

private:
    void CreateImageList();
    static int GetIconIndex(const CaseSummary& case_summary);

    int& GetCurrentColumnWidth();

    struct CaseSummaryWithMeasuredText;
    CaseSummaryWithMeasuredText& GetCaseSummaryWithMeasuredText(int index);

private:
    CImageList m_imageList;
    DWORD m_textColors[2];       // selected = index 1
    DWORD m_backgroundColors[2]; // selected = index 1

    int m_columnWidths[2];       // viewing case keys = index 0
    bool m_adjustColumnWidthAndInvalidateMessagePosted;

    HACCEL m_hAccelerators;

    bool m_selectionsChangedMessagePosted;
    bool m_refreshSelectedCaseSummaries;
    std::vector<std::shared_ptr<const CaseSummary>> m_selectedCaseSummaries;

    std::shared_ptr<DataRepository> m_dataRepository;
    std::shared_ptr<const ViewableCaseIteratorSettings> m_viewableCaseIteratorSettings;

    struct CaseSummaryWithMeasuredText
    {
        std::shared_ptr<const CaseSummary> case_summary;
        std::optional<std::tuple<bool, std::shared_ptr<const std::wstring>, int>> last_displayed_text; // viewing case keys, wide text, text width
    };

    std::optional<size_t> m_numberCases;
    std::map<int, std::vector<CaseSummaryWithMeasuredText>> m_caseSummariesWithMeasuredTexts; // block start index -> case summaries
};
