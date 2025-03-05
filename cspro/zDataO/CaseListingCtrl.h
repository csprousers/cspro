#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/CaseIteratorSettings.h>


// --------------------------------------------------------------------------
// CaseListingReselection
//
// Reselection strategies used for CaseListingCtrl::UpdateCaseListingAsync.
// --------------------------------------------------------------------------

enum class CaseListingReselection
{
    // Clears any existing selections.
    None,

    // Clears any existing selections and then reselects the current
    // selections if they still appear in the case listing.
    SelectedCases,

    // Same as SelectedCases but if no current selections are available, the
    // case closest (by index) to one of the previously-selected cases if
    // selected.
    SelectedCasesOrClosestIndex
};



// --------------------------------------------------------------------------
// CaseListingCtrl
// --------------------------------------------------------------------------

class ZDATAO_API CaseListingCtrl : public CListCtrl
{
public:
    CaseListingCtrl();
    virtual ~CaseListingCtrl();

    void Initialize(std::shared_ptr<DataRepository> data_depository,
                    std::shared_ptr<const ViewableCaseIteratorSettings> viewable_case_iterator_settings);

    // Posts a message to invalidate the case listing. The case listing will not be requeried.
    // This method is useful for toggling between showing case keys and labels.
    void InvalidateCaseListingAsync();

    // Posts a message to requery the case listing.
    // Once complete, cases are reselected following the specified stategy.
    void UpdateCaseListingAsync(CaseListingReselection reselect_strategy = CaseListingReselection::SelectedCases);

    // Returns the number of cases to which the iterator's settings apply.
    const std::optional<size_t>& GetNumberCases() const { return m_numberCases; }

    // Returns the selected case summaries.
    const std::vector<std::shared_ptr<const CaseSummary>>& GetSelectedCaseSummaries();

    // The Find... methods return the index of the case summary that matches the given parameters.
    // If start_index is not 0 and nothing matches, the methods will restart the search from index 0.
    // The number of case summaries that are checked is capped at CaseSummariesFindLimit.
    // The methods return -1 if nothing matches.

    // Returns the index by searching the case key.
    int FindIndexByCaseKey(std::string_view key_sv, size_t start_index = 0);

    // Returns the index by searching the case position.
    int FindIndexByCasePosition(double position_in_repository, size_t start_index = 0);

    // Clears any selections and selects the given case summary (if found).
    bool SelectByCasePosition(double position_in_repository);

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

    void OnFindItem(NMHDR* pNMHDR, LRESULT* pResult);

    void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

    void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
    void OnRightClick(NMHDR* pNMHDR, LRESULT* pResult);

    void OnContextMenu();

    LRESULT OnAdjustColumnWidthAndInvalidate(WPARAM wParam, LPARAM lParam);
    LRESULT OnSelectionsChanged(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateCaseListing(WPARAM wParam, LPARAM lParam);

private:
    void CreateImageList();
    static int GetIconIndex(const CaseSummary& case_summary);

    int& GetCurrentColumnWidth();

    struct CaseSummaryWithMeasuredText;
    CaseSummaryWithMeasuredText& GetCaseSummaryWithMeasuredText(int index);

    template<typename CF>
    int FindIndexWorker(size_t start_index, size_t end_index, const CF& callback_function);

    template<typename CF>
    int FindIndexWorker(size_t start_index, const CF& callback_function);

    int FindIndexByCaseLabel(std::string_view text_sv, size_t start_index = 0);

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
    std::vector<int> m_selectedCaseSummaryIndices;

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
