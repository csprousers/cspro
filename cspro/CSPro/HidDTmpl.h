#pragma once


class CHiddenDocTemplate : public CMultiDocTemplate
{
    DECLARE_DYNAMIC(CHiddenDocTemplate)

public:
    CHiddenDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

    CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible) override;
    CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bAddToMRU, BOOL bMakeVisible) override;

    CDocTemplate::Confidence CHiddenDocTemplate::MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch) override;
};
