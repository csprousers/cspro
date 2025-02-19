#pragma once

class Case;
class CaseIterator;
class CaseIteratorSettings;
class CaseSummary;
class DataRepository;


// --------------------------------------------------------------------------
// CaseProvider
// --------------------------------------------------------------------------

class CaseProvider
{
public:
    virtual ~CaseProvider() { }

    // Only SingleCaseProvider returns a pointer to its case.
    virtual const Case* GetCaseIfSingleCaseOperation() noexcept { return nullptr; }

    // Returns the number of cases.
    // Can throw exceptions.
    virtual size_t GetNumberCases() = 0;

    // Loads the next case if one exists, or returns false if at the end of the cases.
    // Can throw exceptions.
    virtual bool NextCase(Case& data_case) = 0;
};


// --------------------------------------------------------------------------
// SingleCaseProvider
// --------------------------------------------------------------------------

class SingleCaseProvider : public CaseProvider
{
public:
    SingleCaseProvider(std::shared_ptr<const Case> data_case);

    const Case* GetCaseIfSingleCaseOperation() noexcept override;
    size_t GetNumberCases() override;
    bool NextCase(Case& data_case) override;

private:
    std::shared_ptr<const Case> m_case;
    bool m_processedCase;
};


// --------------------------------------------------------------------------
// DataRepositoryCaseProvider
// --------------------------------------------------------------------------

class DataRepositoryCaseProvider : public CaseProvider
{
public:
    DataRepositoryCaseProvider(std::shared_ptr<DataRepository> data_repository,
                               std::shared_ptr<const CaseIteratorSettings> case_iterator_settings = nullptr);

    size_t GetNumberCases() override;
    bool NextCase(Case& data_case) override;

private:
    std::shared_ptr<DataRepository> m_dataRepository;
    std::shared_ptr<const CaseIteratorSettings> m_caseIteratorSettings;
    std::optional<size_t> m_numberCases;
    std::unique_ptr<CaseIterator> m_caseIterator;
};



// --------------------------------------------------------------------------
// SelectiveDataRepositoryCaseProvider
// --------------------------------------------------------------------------

class SelectiveDataRepositoryCaseProvider : public CaseProvider
{
public:
    SelectiveDataRepositoryCaseProvider(std::shared_ptr<DataRepository> data_repository,
                                        const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries);

    size_t GetNumberCases() override;
    bool NextCase(Case& data_case) override;

private:
    std::shared_ptr<DataRepository> m_dataRepository;
    std::vector<double> m_positionsInRepository;
    size_t m_caseCounter;
};
