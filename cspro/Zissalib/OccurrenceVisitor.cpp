#include "StdAfx.h"
#include "OccurrenceVisitor.h"

OccurrenceVisitor::OccurrenceVisitor( int* pArray ) :
    m_pArray( pArray )
{
    ASSERT( pArray != 0 );
}

int OccurrenceVisitor::GetValue()
{
    return m_iValue;
}

void OccurrenceVisitor::SetValue( int iValue )
{
    m_iValue = iValue;
}

void GetTotalOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    SetValue( p->m_Info.m_iTotal );
}

void GetTotalOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    SetValue( p->m_aInfo[m_pArray[0]].m_iTotal );
}

void GetTotalOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    SetValue( p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iTotal );
}

void GetDataOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    SetValue( p->m_Info.m_iData );
}

void GetDataOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    SetValue( p->m_aInfo[m_pArray[0]].m_iData );
}

void GetDataOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    SetValue( p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iData );
}

void GetCurrOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    SetValue( p->m_Info.m_iCurrent );
}

void GetCurrOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    SetValue( p->m_aInfo[m_pArray[0]].m_iCurrent );
}

void GetCurrOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    SetValue( p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iCurrent );
}

//--

void SetTotalOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    p->m_Info.m_iTotal = GetValue();
}

void SetTotalOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    p->m_aInfo[m_pArray[0]].m_iTotal = GetValue();
}

void SetTotalOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iTotal = GetValue();
}

void SetDataOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    p->m_Info.m_iData = GetValue();
}

void SetDataOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    p->m_aInfo[m_pArray[0]].m_iData = GetValue();
}

void SetDataOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iData = GetValue();
}

void SetCurrOccVisitor::visit( RecordOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );

    p->m_Info.m_iCurrent = GetValue();
}

void SetCurrOccVisitor::visit( ItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iSize );

    p->m_aInfo[m_pArray[0]].m_iCurrent = GetValue();
}

void SetCurrOccVisitor::visit( SubItemOccurrenceInfoSet* p )
{
    ASSERT( p != 0 );
    ASSERT( m_pArray != 0 );
    ASSERT( m_pArray[0] < p->m_iXSize );
    ASSERT( m_pArray[1] < p->m_iYSize );

    p->m_aInfo[m_pArray[0]][m_pArray[1]].m_iCurrent = GetValue();
}

//////////////////////////////////////////////////////////////////////////