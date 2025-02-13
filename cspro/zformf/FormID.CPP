//***************************************************************************
//  File name: FormID.cpp
//
//  Description:
//       Forms tree control node implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "FormID.H"


IMPLEMENT_DYNAMIC(CFormID, CObject)

CFormID::CFormID()
    :   m_eNodeType(eINVALID_TYPE),
        m_pFormDoc(nullptr),
        m_iForm(0),
        m_iCol(NONE),
        m_iRosterField(NONE),
        m_pItem(nullptr)
{
}

CFormID::~CFormID()
{
}



// *********************************************************************

IMPLEMENT_DYNAMIC(CFormNodeID, CFormID)

CFormNodeID::CFormNodeID()
    :   m_iCount(0)
{
}

CFormNodeID::~CFormNodeID()
{
}



// *********************************************************************

IMPLEMENT_DYNAMIC(FormExternalCodeID, CFormID)

FormExternalCodeID::FormExternalCodeID(CFormDoc* pDoc, std::optional<CodeFile> code_file)
    :   m_codeFile(std::move(code_file))
{
    SetNodeType(eFFT_EXTERNALCODE);
    SetFormDoc(pDoc);
}


std::optional<CString> FormExternalCodeID::GetName() const
{
    // the heading for the external code nodes will simply be "Logic" because clicking on it will show the main application logic
    return m_codeFile.has_value() ? PortableFunctions::PathGetFilenameWithoutExtension<CString>(m_codeFile->GetFilename()) :
                                    _T("Logic");
}


std::optional<CString> FormExternalCodeID::GetLabel() const
{
    return GetName();
}


TextSource* FormExternalCodeID::GetTextSource()
{
    return m_codeFile.has_value() ? &m_codeFile->GetTextSource() :
                                    nullptr;
}



// *********************************************************************

IMPLEMENT_DYNAMIC(FormReportID, CFormID)

FormReportID::FormReportID(CFormDoc* pDoc, std::shared_ptr<NamedTextSource> named_text_source)
    :   m_namedTextSource(named_text_source)
{
    SetNodeType(eFFT_REPORT);
    SetFormDoc(pDoc);
}


std::optional<CString> FormReportID::GetName() const
{
    if( m_namedTextSource == nullptr )
    {
        return _T("Reports"); // the heading
    }

    else
    {
        return WS2CS(m_namedTextSource->name);
    }
}


std::optional<CString> FormReportID::GetLabel() const
{
    if( m_namedTextSource == nullptr )
    {
        return _T("Reports"); // the heading
    }

    else
    {
        return PortableFunctions::PathGetFilenameWithoutExtension<CString>(m_namedTextSource->text_source->GetFilename());
    }
}


TextSource* FormReportID::GetTextSource()
{
    if( m_namedTextSource == nullptr )
    {
        return nullptr;
    }

    else
    {
        return m_namedTextSource->text_source.get();
    }
}
