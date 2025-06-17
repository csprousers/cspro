#include "StdAfx.h"
#include "cLinkSta.h"
#include "cLinkSub.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

CLinkStatVar::CLinkStatVar() {
    Init();
}

CLinkStatVar::~CLinkStatVar() {
}

void CLinkStatVar::Init() {
    m_csVarName.Empty();
    m_iSeqNumber = 0;
}

void CLinkStatVar::SetName( CString csVarName ) {
    m_csVarName = csVarName;
}

void CLinkStatVar::SetSeqNumber( int iSeqNumber ) {
    m_iSeqNumber = iSeqNumber;
}

CString CLinkStatVar::GetName() {
    return m_csVarName;
}

int CLinkStatVar::GetSeqNumber() {
    return m_iSeqNumber;
}


CLinkStat::CLinkStat() {
    Init();
}

CLinkStat::~CLinkStat() {
}

/*void CLinkStat::Copy( CLinkStat& rLinkStat ) {
    m_aSubTables.RemoveAll();
    m_aStatVars.RemoveAll();


    for( int i=0; i < rLinkStat.GetNumSubTable(); i++ ) {
        CLinkSubTable&  rLinkSubTable= rLinkStat.GetSubTable(i);

        m_aSubTables.Add( rLinkSubTable );
    }

    for( i=0; i < rLinkStat.GetNumStatVar(); i++ ) {
        CLinkStatVar& rLinkStatVar=rLinkStat.GetStatVar(i);

        m_aStatVars.Add( rLinkStatVar );
    }

     //Savy changed this to pointer m_aStatBase.RemoveAll();
    //Right not the pointer are not cloned in the copy . They are just referenced. This may potentially cause a
    RemoveAllStat();
    for( i=0; i < rLinkStat.GetNumStat(); i++ ) {
        //Savy changed to do the pointer stuff
        //CtStatBase& rLinkStatBase=rLinkStat.GetStat(i);
       // m_aStatBase.Add( rLinkStatBase );
        CtStatBase* pLinkStatBase=rLinkStat.GetStat(i);
        m_aStatBase.Add(pLinkStatBase);


    }
}

CLinkStat::CLinkStat( CLinkStat& rLinkStat) {
    Copy( rLinkStat );
}

void CLinkStat::operator=(CLinkStat& rLinkStat) {
    Copy( rLinkStat );
}

*/
void CLinkStat::Init() {
    m_aSubTables.RemoveAll();
    m_aStatVars.RemoveAll();
    //Savy for changing it to pointer m_aStatBase.RemoveAll();
    RemoveAllStat();
}

void CLinkStat::AddSubTable( CLinkSubTable& rLinkSubTable ) {
    m_aSubTables.Add( rLinkSubTable );
}

int CLinkStat::GetNumSubTable(){
    return m_aSubTables.GetSize();
}

CLinkSubTable& CLinkStat::GetSubTable( int i ){
    return m_aSubTables.ElementAt(i);
}

void CLinkStat::AddStatVar( CLinkStatVar& rLinkStatVar ){
    m_aStatVars.Add( rLinkStatVar );
}

int CLinkStat::GetNumStatVar(){
    return m_aStatVars.GetSize();
}

CLinkStatVar& CLinkStat::GetStatVar( int i ){
    return m_aStatVars.ElementAt(i);
}

/* Savy changed this to pointer
void CLinkStat::AddStat( CtStatBase& rStatBase ){
    m_aStatBase.Add( rStatBase );
}
*/

int CLinkStat::GetNumStat(){
    return m_aStatBase.GetSize();
}
//Savy  20090424
void CLinkStat::AddStat(CtStatBase *pStatBase){
    m_aStatBase.Add( pStatBase );
}
CtStatBase* CLinkStat::GetStat(int i){
    return m_aStatBase.GetAt(i);
}
void CLinkStat::RemoveAllStat(){

    for( int i=0; i < GetNumStat(); i++ ) {
        CtStatBase* pStatBase=GetStat(i);
        delete pStatBase;
    }

    m_aStatBase.RemoveAll();

}
/* Savy changed this to pointer
CtStatBase& CLinkStat::GetStat( int iStat ){
    return m_aStatBase.ElementAt(iStat);
}
*/
