#include "StdAfx.h"
#include <engine/Engdrv.h>

// the following two functions are also in zFormF\KeyboardInputDlg.cpp

UINT LayoutName2KLID(TCHAR * pLayoutName) // GHM 20120822 this function treats the string as a hex value, converting it to a number
{
    UINT klid = 0;

    for( int i = 0; i < KL_NAMELENGTH - 1; i++ ) // -1 for the 0 byte at the end of the string
    {
        klid *= 16;

        if( isdigit(pLayoutName[i]) )
            klid += pLayoutName[i] - '0';

        else
            klid += tolower(pLayoutName[i]) - 'a' + 10;
    }

    return klid;
}

void KLID2LayoutName(UINT klid,TCHAR * pLayoutName) // GHM 20120822
{
    CString temp;
    temp.Format(_T("%08x"),klid);
    wcscpy(pLayoutName,temp);
}


HKL CEngineDriver::LoadKLID(UINT klid) // GHM 20120822
{
    HKL hKL = NULL;

    if( !klid || m_mKLID2HKLMap.Lookup(klid,hKL) )
        return hKL;

    // load the key ... first check the active keyboards
    int iNumEnumeratedHKLs = GetKeyboardLayoutList(0,NULL);
    HKL * pEnumeratedHKLs = new HKL[iNumEnumeratedHKLs];
    GetKeyboardLayoutList(iNumEnumeratedHKLs,pEnumeratedHKLs);

    HKL hCurrentKL = GetKeyboardLayout(0);
    TCHAR layoutName[KL_NAMELENGTH];

    for( int i = 0; !hKL && i < iNumEnumeratedHKLs; i++ )
    {
        ActivateKeyboardLayout(pEnumeratedHKLs[i],0);
        GetKeyboardLayoutName(layoutName);

        if( LayoutName2KLID(layoutName) == klid )
            hKL = pEnumeratedHKLs[i];
    }

    if( !hKL ) // it isn't a current keyboard, so see if it's in the system and if we can load it
    {
        KLID2LayoutName(klid,layoutName);

        TCHAR layoutNameCheck[KL_NAMELENGTH];

        hKL = LoadKeyboardLayout(layoutName,KLF_ACTIVATE);
        GetKeyboardLayoutName(layoutNameCheck);

        if( _tcsicmp(layoutName,layoutNameCheck) )
            hKL = NULL;

        else
            m_vAddedKeyboards.Add(hKL);
    }

    ActivateKeyboardLayout(hCurrentKL,0);

    delete [] pEnumeratedHKLs;

    m_mKLID2HKLMap.SetAt(klid,hKL); // add it to the map (even if HKL is NULL, because in that case we won't continually look up a bad keyboard)

    return hKL;
}

void CEngineDriver::UnloadAddedKeyboards() // GHM 20120822
{
    for( int i = 0; i < m_vAddedKeyboards.GetSize(); i++ )
        UnloadKeyboardLayout(m_vAddedKeyboards[i]);
}


UINT CEngineDriver::GetKLIDFromHKL(HKL hKL) // GHM 20120822 search the map for the KLID
{
    POSITION pos = m_mKLID2HKLMap.GetStartPosition();

    UINT thisKLID;
    HKL thisHKL;

    while( pos )
    {
        m_mKLID2HKLMap.GetNextAssoc(pos,thisKLID,thisHKL);

        if( thisHKL == hKL )
            return thisKLID;
    }

    return 0;
}