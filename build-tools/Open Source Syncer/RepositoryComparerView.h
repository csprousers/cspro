#pragma once


class RepositoryComparerView : public CFormView
{
    DECLARE_DYNCREATE(RepositoryComparerView)

protected:
    RepositoryComparerView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnCompare();

private:
    static void OnCompare(Controller& controller, const std::string& private_commit_sha,
                          const std::string& open_source_commit_sha);

private:
    Controller& m_controller;

    SharableString m_privateCommit;
    SharableString m_openSourceCommit;
};
