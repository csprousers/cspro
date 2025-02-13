#include "StdAfx.h"
#include <engine/Engine.h>
#include <engine/Batdrv.h>
#include <engine/Ctab.h>

// Return number of tables to be written in the TBD
int CBatchDriverBase::GetNumCtabToWrite()
{
    int iNumCtabs = 0;

    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        if( pCtab->GetAcumArea() != NULL )
            iNumCtabs++;
    }

    return iNumCtabs;
}
