#pragma once

#include <CSDocument/DocSetSpec.h>
#include <CSDocument/TextEditDoc.h>


class DocSetComponentDoc : public TextEditDoc
{
    DECLARE_DYNCREATE(DocSetComponentDoc)

protected:
    DocSetComponentDoc() { }  // create from serialization only

public:
    DocSetSpec& GetDocSetSpec() { ASSERT(m_docSetSpec != nullptr); return *m_docSetSpec; }

    DocSetComponent::Type GetDocSetComponentType() const { ASSERT(m_docSetComponentType.has_value()); return *m_docSetComponentType; }

    // TextEditDoc overrides
    int GetLexerLanguage() const override { return ( m_docSetComponentType == DocSetComponent::Type::ContextIds ) ? SCLEX_NULL : SCLEX_JSON; }

protected:
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

private:
    std::optional<DocSetComponent::Type> m_docSetComponentType;
};
