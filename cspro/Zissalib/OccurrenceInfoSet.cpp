#include "StdAfx.h"
#include "OccurrenceVisitor.h"

RecordOccurrenceInfoSet::RecordOccurrenceInfoSet()
// no modification to default m_LastSeen. Default is Ok
{
}

void RecordOccurrenceInfoSet::accept( OccurrenceVisitor& v )
{
    v.visit( this );
}

ItemOccurrenceInfoSet::ItemOccurrenceInfoSet( int iNumOfRecords ) :
    m_iSize( iNumOfRecords )
// no modification to default m_LastSeen. Default is Ok
{
    ASSERT( iNumOfRecords > 0 );

    for( int i = 0; i < iNumOfRecords; i++ )
    {
        OccurrenceInfo info;

        m_aInfo.push_back(info);
    }
}

void ItemOccurrenceInfoSet::accept( OccurrenceVisitor& v )
{
    v.visit( this );
}

SubItemOccurrenceInfoSet::SubItemOccurrenceInfoSet( int iNumOfRecords, int iNumItems ) :
    m_iXSize( iNumOfRecords ), m_iYSize( iNumItems )
// no modification to default m_LastSeen. Default is Ok
{
    ASSERT( iNumOfRecords > 0 );
    ASSERT( iNumItems > 0 );

    for( int i = 0; i < iNumOfRecords; i++ )
    {
        std::vector<OccurrenceInfo> v;
        OccurrenceInfo info;

        for( int j = 0; j < iNumItems; j++ )
        {
            v.push_back(info);
        }

        m_aInfo.push_back( v );
    }
}

void SubItemOccurrenceInfoSet::accept( OccurrenceVisitor& v )
{
    v.visit( this );
}

//........................................................................
