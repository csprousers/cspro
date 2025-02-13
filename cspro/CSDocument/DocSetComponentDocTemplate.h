#pragma once

#include <CSDocument/CSDocument.h>


class DocSetComponentDocTemplate : public CMultiDocTemplate
{
public:
    using CMultiDocTemplate::CMultiDocTemplate;

    CDocTemplate::Confidence MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch) override
    {
        ASSERT(rpDocMatch == nullptr);

        // only match documents when parameters for the next open have been set
        CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());

        if( csdoc_app.HasDocSetParametersForNextOpen(lpszPathName) &&
            !CSDocumentApp::DocumentCanBeOpenedDirectly(lpszPathName) )
        {
            // check if the document is already open
            if( __super::MatchDocType(lpszPathName, rpDocMatch) == Confidence::yesAlreadyOpen )
            {
                ASSERT(rpDocMatch != nullptr);
                csdoc_app.ReleaseDocSetParametersForNextOpen();
                return Confidence::yesAlreadyOpen;
            }

            return Confidence::yesAttemptNative;
        }

        return Confidence::noAttempt;
    }
};
