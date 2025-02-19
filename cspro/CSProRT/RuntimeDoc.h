#pragma once


class RuntimeDoc : public CDocument
{
    DECLARE_DYNCREATE(RuntimeDoc)

protected:
    RuntimeDoc() { } // create from serialization only

public:
    const std::string& GetPathFromCommandLine() const { return m_pathFromCommandLine; }

protected:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

private:
    std::string m_pathFromCommandLine;
};
