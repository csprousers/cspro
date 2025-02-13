#pragma once

//---------------------------------------------------------------------------
//  File name: GroupT2.h
//
//  Description:
//          Header for engine-GroupT2 class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Aug 00   RHF     Created
//
//
//---------------------------------------------------------------------------

#include <Zissalib/GroupT.h>
#include <zEngineO/AllSymbolDeclarations.h>


class CLoopControl
{
private:
    int m_iLoopIndex;
    int m_iFromOcc;
    int m_iUntilOcc;
    int m_iMaxOcc;

public:
    CLoopControl() {
        CLoopControl(0,0,0,0);
    }

    CLoopControl( const int iLoopIndex, const int iFromOcc, const int iUntilOcc, const  int iMaxOcc ) {
        m_iLoopIndex = iLoopIndex;
        m_iFromOcc = iFromOcc;
        m_iUntilOcc = iUntilOcc;
        m_iMaxOcc = iMaxOcc;
        ASSERT( m_iFromOcc <= iUntilOcc );
        ASSERT( m_iMaxOcc >= iUntilOcc );
    }

    int GetLoopIndex() const { return( m_iLoopIndex ); }
    int GetFromOcc() const { return( m_iFromOcc ); }
    int GetUntilOcc() const { return( m_iUntilOcc ); }
    int GetMaxOcc() const { return( m_iMaxOcc ); }
};

class CLoopInstruction {
public:
    CLoopInstruction( const GROUPT::eGroupOperation eMode, std::vector<int>* pTarget ) {
        m_eMode = eMode;
        m_pTarget = pTarget;
    }
    GROUPT::eGroupOperation m_eMode;
    std::vector<int>*       m_pTarget;  // Each entry keep the source occurrences
};
