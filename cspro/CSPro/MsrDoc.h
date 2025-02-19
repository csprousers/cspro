#pragma once

// MsrDoc.h : interface of the CCSProDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CCSProDoc : public CDocument
{
    DECLARE_DYNCREATE(CCSProDoc)

public:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

    bool IsFileOpen(std::string_view file_path_sv) const;
};
