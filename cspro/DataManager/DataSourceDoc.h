#pragma once

#include <DataManager/CaseHoldingDoc.h>
#include <zDataO/DataRepository.h>

class CaseListingView;
class DataSourceFrame;
class DataSourceSettings;
class DictionarySource;
class ViewableCaseIteratorSettings;


class DataSourceDoc : public CaseHoldingDoc
{
    DECLARE_DYNCREATE(DataSourceDoc)

protected:
    DataSourceDoc();

public:
    ~DataSourceDoc();

    DataSourceFrame& GetFrame();
    CaseListingView& GetCaseListingView();

    const DataRepository& GetDataRepository() const           { return *m_dataRepository; }
    DataRepository& GetDataRepository()                       { return *m_dataRepository; }
    std::shared_ptr<DataRepository> GetSharedDataRepository() { return m_dataRepository; }

    void SetCurrentCase(std::shared_ptr<const Case> data_case) { m_currentCase = std::move(data_case); }

    bool IsDocumentCaseOnly() const override { return false; }

    UINT GetDefaultPageCommandId() override;

protected:
    DECLARE_MESSAGE_MAP()

    void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    bool LoadDictionary();

private:
    std::shared_ptr<DataSourceSettings> m_dataSourceSettings;
    std::unique_ptr<DictionarySource> m_dictionarySource;
    std::shared_ptr<DataRepository> m_dataRepository;
};
