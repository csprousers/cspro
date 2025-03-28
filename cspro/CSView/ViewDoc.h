#pragma once

#include <zViewO/ViewInput.h>


class ViewDoc : public CDocument
{
    DECLARE_DYNCREATE(ViewDoc)

protected:
    ViewDoc(); // create from serialization only

public:
    std::string GetDescription() const;
    std::string GetUrl();

protected:
    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    void ProcessCloseDocument();

private:
    std::unique_ptr<ViewInput> m_viewInput;
};
