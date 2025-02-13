#pragma once

// CReadyFlags
//    Helper class to reduce common code in
//       varsanal_tryInheritance and varsanal_generateVarNode methods
//
// rcl, Nov 2004

class CReadyFlags
{
    bool m_bReady[DIM_MAXDIM];
    int  m_iHowManyReady;
    int  m_iTotalToSet;
public:
    CReadyFlags()
    {
        m_iTotalToSet = 0;
        m_iHowManyReady = 0;
        for( int i = 0; i < DIM_MAXDIM; i++ )
            m_bReady[i] = false;
    }

    void setTotalSet( int iTotalSet ) { m_iTotalToSet = iTotalSet; }
    bool flagIsSet( int i )           { return m_bReady[i];        }
    bool flagsAreCompleted()          { return m_iHowManyReady >= m_iTotalToSet; }
    void protectFlag( int i )         { m_bReady[i] = true;  }
    void unprotectFlag( int i )       { m_bReady[i] = false; }
    void setFlag( int i )
    {
        ASSERT( i >= 0 && i < DIM_MAXDIM );

        if( !flagIsSet(i) )
        {
            ASSERT( m_iHowManyReady < m_iTotalToSet );
            m_bReady[i] = true;
            m_iHowManyReady++;
        }
    }
};
