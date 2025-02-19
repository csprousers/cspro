#include "StdAfx.h"
#include "CaseContentCreatorBase.h"


CaseContentCreatorBase::CaseContentCreatorBase(CaseHoldingDoc& case_holding_doc)
    :   m_caseHoldingDoc(case_holding_doc)
{
}


std::string CaseContentCreatorBase::GetSaveSuggestedFilename() const
{
    ASSERT(m_dataCase != nullptr);
    return m_dataCase->GetSingleLineKey();
}


void CaseContentCreatorBase::UpdateCurrentCase()
{
    std::shared_ptr<const Case> data_case = m_caseHoldingDoc.GetSharedCurrentCase();

    // the "current case" will be null if the user clicks off the case in the case listing;
    // in that scenario, keep using the previously-selected case
    if( data_case != nullptr )
        m_dataCase = std::move(data_case);

    ASSERT(m_dataCase != nullptr);
}


SharableString CaseContentCreatorBase::GetTextContent()
{
    UpdateCurrentCase();
    return GetTextContentWorker();
}


SharableString CaseContentCreatorBase::GetTextContentWorker()
{
    return ContentCreator::GetTextContent();
}


SharableString CaseContentCreatorBase::GetHtmlContent(const bool embed_resources/* = false*/)
{
    UpdateCurrentCase();
    return GetHtmlContentWorker(embed_resources);
}


SharableString CaseContentCreatorBase::GetHtmlContentWorker(bool /*embed_resources*/)
{
    throw ProgrammingErrorException();
}
