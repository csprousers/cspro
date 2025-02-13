#pragma once
// CRunAplE.h: interface for the CCapiRunAplEntry class.
//
//////////////////////////////////////////////////////////////////////

#include <zCaseTreeF/zCaseTreeF.h>
#include <engine/DEFLD.H>

class CRunAplEntry;
class CNPifFile;
class CDEFormFile;
class CDataDict;
class CEngineDriver;
class CCapi;
class CTreeItemInfo;

class ZCASETREEF_API CCapiRunAplEntry
{
public:
    CCapiRunAplEntry(CRunAplEntry* pRunAplEntry, CNPifFile* pPifFile, CDEItemBase* pCurItemBase);
    virtual ~CCapiRunAplEntry();

public:
    CRunAplEntry*   GetRunAplEntry();

    //SOME ITEM DESCRIPTIVE FUNCTIONS
    bool    IsMirror    (CDEItemBase* pItem, int iOcc);
    bool    IsPersistent(CDEItemBase* pItem, int iOcc);
    bool    IsProtected (CDEItemBase* pItem, int iOcc);

    CDEField*           GetDeField( DEFLD* pFld );

    CString             GetNodeLabel(int iNodeIdx);
    bool                InEnterMode();

    int                 GetNNodes();
    void                GetInfo(bool bVerified, CDEItemBase* pItem, int iOccurrenceIndex, int iMaxItemLabelLength, bool bShortRepresentation, CTreeItemInfo* pTreeItemInfo,
                                                int* iNonSelectedIconIndex, int* iSelectedIconIndex, int* iIconColor, CString* CSFieldLabel, CString* CSToolTip);

    CDEFormFile*        GetDEFormFile(bool bPrimaryFlow);
    CDEItemBase*        GetCurItemBase();

    //Node functions :
    bool                IsOpen(int iNodeIdx );
    int                 GetLevel( int iNodeIdx );

    //Verify functions :
    bool                InVerify();

    void                GetVal( CDEField* pField, const int iOcc, CIMSAString& sData, int* iStatus, bool bFormatValue, COLORREF* pBkColor = NULL );

    bool                IsItemSelectable( CDEItemBase* pItem, int iOcc );

    static void         GetFieldColorRGB(int iColor, COLORREF& rgbColor );

private:
    CString StringRepresentation(CDEItemBase* pItem, bool bShortRepresentation);

private:
    CRunAplEntry*               m_pRunAplEntry;
    CNPifFile*                  m_pCPifFile;
    CEngineDriver*              m_pEngineDriver;
};
