#include "StdAfx.h"
#include "cLinkTab.h"
#include "cLinkUnt.h"
#include "cLinkSub.h"
#include "ctstat.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CLinkTable::CLinkTable() {
    Init();
}

CLinkTable::~CLinkTable() {
}

void CLinkTable::Init() {
    m_csFullCommand.Empty();
    m_csName.Empty();
    m_csRowExpr.Empty();
    m_csColExpr.Empty();
    m_csLayExpr.Empty();
    m_csSelect.Empty();
    m_csWeight.Empty();
    m_csTabLogic.Empty();
    m_csInclude.Empty();
    m_csExclude.Empty();
    m_csLevel.Empty();

    m_csTitle.Empty();
    m_csStubTitle.Empty();

    m_csNoBreak.Empty();
    m_csNoPrint.Empty();

    m_csProcName.Empty();
    m_bHasSyntaxError = false;


    m_RowTermExpr.RemoveAll();
    m_ColTermExpr.RemoveAll();
    m_LayTermExpr.RemoveAll();

    m_aUnits.RemoveAll();
    m_aStats.RemoveAll();
}

// Get methods
CStringPos  CLinkTable::GetFullCommand() {
    return m_csFullCommand;
}

CStringPos  CLinkTable::GetName() {
    return m_csName;
}

CStringPos  CLinkTable::GetRowExpr() {
    return m_csRowExpr;
}

CStringPos  CLinkTable::GetColExpr() {
    return m_csColExpr;
}

CStringPos  CLinkTable::GetLayExpr() {
    return m_csLayExpr;
}

CStringPos  CLinkTable::GetSelect() {
    return m_csSelect;
}

CStringPos  CLinkTable::GetWeight() {
    return m_csWeight;
}

CStringPos  CLinkTable::GetTablogic() {
    return m_csTabLogic;
}

CStringPos  CLinkTable::GetInclude() {
    return m_csInclude;
}

CStringPos  CLinkTable::GetExclude() {
    return m_csExclude;
}

CStringPos  CLinkTable::GetLevel() {
    return m_csLevel;
}

CStringPos  CLinkTable::GetTitle() {
    return m_csTitle;
}

CStringPos  CLinkTable::GetStubTitle() {
    return m_csStubTitle;
}

CStringPos CLinkTable::GetNoBreak() {
    return m_csNoBreak;
}

CStringPos CLinkTable::GetNoPrint() {
    return m_csNoPrint;
}

CString CLinkTable::GetProcName() {
    return m_csProcName;
}

bool CLinkTable::GetHasSyntaxError() {
    return m_bHasSyntaxError;
}

CArray<CLinkTerm, CLinkTerm>& CLinkTable::GetRowTermExpr() {
    return m_RowTermExpr;
}

CArray<CLinkTerm, CLinkTerm>& CLinkTable::GetColTermExpr() {
    return m_ColTermExpr;
}

CArray<CLinkTerm, CLinkTerm>& CLinkTable::GetLayTermExpr() {
    return m_LayTermExpr;
}






#include <engine/dimens.h>
int CLinkTable::GetNumCells( int iDim ) {
    int     iNumCells=0;
    ASSERT( iDim == DIM_ROW || iDim == DIM_COL || iDim == DIM_LAYER );

    CArray<CLinkTerm, CLinkTerm>& rDimExpr= (iDim==DIM_ROW) ? m_RowTermExpr :
                                            (iDim==DIM_COL) ? m_ColTermExpr : m_LayTermExpr;


    for( int iTerm=0; iTerm < rDimExpr.GetSize(); iTerm++ ) {
        CLinkTerm& rLinkTerm=rDimExpr.ElementAt(iTerm);

        iNumCells += rLinkTerm.GetNumCells();
    }

    return iNumCells;
}


void CLinkTable::AddUnit( CLinkUnit& rLinkUnit ) {
    m_aUnits.Add( rLinkUnit );
}

int CLinkTable::GetNumUnit() {
    return m_aUnits.GetSize();
}

CLinkUnit& CLinkTable::GetUnit( int iUnit ) {
    return m_aUnits.ElementAt(iUnit);
}
/*Savy changed this to pointe
void CLinkTable::AddStat( CLinkStat& rLinkStat ) {
    m_aStats.Add( rLinkStat );
}
*/
void CLinkTable::AddStat( CLinkStat* pLinkStat ) {
    m_aStats.Add( pLinkStat );
}
int CLinkTable::GetNumStat() {
    return m_aStats.GetSize();
}
// Savy Changed this to pointer
CLinkStat* CLinkTable::GetStat( int iStat ) {
    return m_aStats.ElementAt(iStat);
}
