// ctstatv.cpp: implementation of the CtStatV class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ctstatv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

CtStatVar::CtStatVar() {
    Init();
}

#define CLONE(x) x = other.x

void CtStatVar::cloneFrom( CtStatVar& other )
{
    if( &other != this  )
    {
        CLONE(m_iSubtableNumber);
        CLONE(m_iCoordNumber);
        CLONE(m_iSymVar);
        CLONE(m_bOverlappedCat);
    }
}

CtStatVar::CtStatVar( CtStatVar& other )
{
    cloneFrom(other);
}

void CtStatVar::operator =( CtStatVar& other )
{
    cloneFrom(other);
}

void CtStatVar::Init() {
    SetSubtableNumber(-1);
    SetCoordNumber(-1);
    SetSymVar(-1);
    SetHasOverlappedCat( false );
}

void    CtStatVar::SetSubtableNumber(  int iSubTableNumber  )   { m_iSubtableNumber = iSubTableNumber; }
void    CtStatVar::SetCoordNumber( int iCoordNumber  )          { m_iCoordNumber = iCoordNumber; }
void    CtStatVar::SetSymVar( int iSymVar )                     { m_iSymVar = iSymVar; }
void    CtStatVar::SetHasOverlappedCat( bool bOverlappedCat )   { m_bOverlappedCat = bOverlappedCat; }

int     CtStatVar::GetSubtableNumber()                          { return m_iSubtableNumber; }
int     CtStatVar::GetCoordNumber()                             { return m_iCoordNumber; }
int     CtStatVar::GetSymVar()                                  { return m_iSymVar; }
bool    CtStatVar::GetHasOverlappedCat()                        { return m_bOverlappedCat; }


