#include "StdAfx.h"
#include "cLinkUnt.h"
#include "cLinkSub.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CLinkUnit::CLinkUnit() {
    Init();
}

CLinkUnit::~CLinkUnit() {
}

void CLinkUnit::Copy(CLinkUnit& rLinkUnit) {
    m_aSubTables.RemoveAll();

    for( int i = 0; i < rLinkUnit.GetNumSubTable(); i++ ) {
        CLinkSubTable& rLinSubTable=rLinkUnit.GetSubTable(i);

        m_aSubTables.Add( rLinSubTable );
    }

    m_csRepeatingKeyword = rLinkUnit.GetRepeatingKeyword();
    m_csRepeatingName = rLinkUnit.GetRepeatingName();
    m_csSelectExpr = rLinkUnit.GetSelectExpr();
    m_csWeightExpr = rLinkUnit.GetWeightExpr();
    m_csTablogicExpr = rLinkUnit.GetTablogicExpr();
}

CLinkUnit::CLinkUnit( CLinkUnit& rLinkUnit) {
    Copy( rLinkUnit );
}

void CLinkUnit::operator=(CLinkUnit& rLinkUnit) {
    Copy ( rLinkUnit );
}

void CLinkUnit::Init() {
    m_aSubTables.RemoveAll();
    m_csRepeatingKeyword.Empty();
    m_csRepeatingName.Empty();
    m_csWeightExpr.Empty();
    m_csSelectExpr.Empty();
    m_csTablogicExpr.Empty();
}

void CLinkUnit::AddSubTable( CLinkSubTable& rLinkSubTable ) {
    m_aSubTables.Add( rLinkSubTable );
}

int CLinkUnit::GetNumSubTable() {
    return m_aSubTables.GetSize();
}

CLinkSubTable&  CLinkUnit::GetSubTable( int i ) {
    return m_aSubTables.ElementAt(i);
}

void CLinkUnit::SetRepeatingKeyword( CString csRepeatingKeyword ) {
    m_csRepeatingKeyword = csRepeatingKeyword;
}

void CLinkUnit::SetRepeatingName( CString csRepeatingName ) {
    m_csRepeatingName = csRepeatingName;
}

void CLinkUnit::SetSelectExpr( CString csSelectExpr ) {
    m_csSelectExpr = csSelectExpr;
}

void CLinkUnit::SetWeightExpr( CString csWeightExpr ) {
    m_csWeightExpr = csWeightExpr;
}

void CLinkUnit::SetTablogicExpr( CString csTablogicExpr ) {
    m_csTablogicExpr = csTablogicExpr;
}


CString CLinkUnit::GetRepeatingKeyword() {
    return m_csRepeatingKeyword;
}

CString CLinkUnit::GetRepeatingName() {
    return m_csRepeatingName;
}

CString CLinkUnit::GetSelectExpr(){
    return m_csSelectExpr;
}

CString CLinkUnit::GetWeightExpr(){
    return m_csWeightExpr;
}

CString CLinkUnit::GetTablogicExpr(){
    return m_csTablogicExpr;
}


