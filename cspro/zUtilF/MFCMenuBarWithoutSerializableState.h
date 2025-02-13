#pragma once

#include <afxmenubar.h>


// this class overrides the LoadState and SaveState methods so that
// the menu's settings aren't loaded/saved to the registry and thus
// remain the way that they were designed

class MFCMenuBarWithoutSerializableState : public CMFCMenuBar
{
public:
	BOOL LoadState(LPCTSTR /*lpszProfileName = NULL*/, int /*nIndex = -1*/, UINT /*uiID = (UINT) -1*/) override
    {
        return TRUE;
    }

    BOOL SaveState(LPCTSTR /*lpszProfileName = NULL*/, int /*nIndex = -1*/, UINT /*uiID = (UINT) -1*/) override
    {
        return TRUE;
    }
};
