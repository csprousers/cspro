#pragma once

#include <CSDocument/TextEditDoc.h>
#include <zDocument/DocSetSpec.h>

class DocSetSpecFrame;


class DocSetSpecDoc : public TextEditDoc
{
    DECLARE_DYNCREATE(DocSetSpecDoc)

protected:
    DocSetSpecDoc() { }  // create from serialization only

public:
    static void CreateNewDocument();

    DocSetSpec& GetDocSetSpec() { ASSERT(m_docSetSpec != nullptr); return *m_docSetSpec; }

    // TextEditDoc overrides
    int GetLexerLanguage() const override { return SCLEX_JSON; }

protected:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
};
