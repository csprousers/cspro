#pragma once

#include <zDataO/CaseListingCtrl.h>


class DataSourceCaseListingCtrl : public CaseListingCtrl
{
protected:
    void OnCaseListingCaseSummariesQueried(std::variant<size_t, const char*> number_cases_or_exception) override;
    void OnCaseListingSelectionsChanged() override;
    void OnCaseListingDoubleClickAndReturn() override;
    void OnCaseListingContextMenu(CPoint point) override;
    bool OnCaseListingDeleteKey() override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnCopyKey();
    void OnViewCases();
    void OnRunTaskFromCaseListing(UINT nID);
    void OnPostCommand(UINT nID);

private:
    DataSourceFrame& GetDataSourceFrame();
};
