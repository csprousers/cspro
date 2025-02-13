#pragma once

#include <zCaseTreeF/zCaseTreeF.h>

/*

- Lista de constantes admisibles para m_iTypeOfInfo :

                CS_LEVEL_INFO
                CS_GROUP_INFO
                CS_GROUP_OCCURRENCES_TITTLE
                CS_ROSTER_OCCURRENCES_TITTLE
                CS_ROSTER_INFO
                CS_FIELD_INFO
                CS_OCCURRENCE_INDEX

         - La variable m_iDataOccurrences en el caso de un Field se refiere al DataOcc de su contenedor.

*/

class ZCASETREEF_API CTreeItemInfo
{
public:
    CTreeItemInfo* GetTreeItemInfoParent();
    void SetItemInfoParent( CTreeItemInfo* pTreeItemInfo);

    void            SetItemParent( HTREEITEM hItem);
    HTREEITEM       GetItemParent();

    //
    void SetIndex                ( int iIndex                       );
    void SetTypeOfInfo           ( int iTypeOfInfo                  );
    void SetItem                 ( CDEItemBase* pItem               );
    void SetLevel                ( CDELevel* pLevel                 );
    void SetOccurrence           ( int iOccurrenceIndex             );
    void SetMaxNumOccurrences    ( int iMaxNumOccurrences           );
    void SetMaxItemLabelLength   ( int iMaxItemLabelLength          );
    void SetNonSelectedIconIndex ( int iNonSelectedIconIndex        );
    void SetSelectedIconIndex    ( int iSelectedIconIndex           ); //FABN 14/Aug/2002
    void SetDataOccurrences      ( int iDataOccurrences             );
    bool equalKey                ( CTreeItemInfo * pOtherItemInfo   ); //FABN 19/Aug/2002
    void setExpanded             ( bool bExpanded                   );
    void SetLabel                ( CString csLabel                  );
    void SetToolTip              ( CString csToolTip                );
    bool Match                   ( CDEField* pField, int iOcc       );

    //
    int             GetTypeOfInfo           ();
    int             GetIndex                ();
    int             GetOccurrence           ();
    int             GetMaxNumOccurrences    ();
    int             GetMaxItemLabelLength   ();
    int             GetNonSelectedIconIndex ();
    int             GetSelectedIconIndex    (); //FABN 14/Aug/2002
    const CString&  GetKey                  (); //FABN 19/Aug/2002
    bool            IsExpanded              (); //FABN 22/Aug/2002
    CString&        GetToolTip              ();
    int             GetNodeIdx              ();


    CDEItemBase*     GetItem();
    CDELevel*        GetLevel();
    const CDictItem* GetDictItem();
    CString          GetLabel();

    CTreeItemInfo();
    CTreeItemInfo(  int              iNodeIdx,
                    CString          csKey,
                    int              iIndex,
                    int              iTypeOfInfo,
                    int              iOccurrenceIndex,
                    int              iMaxNumOccurrences,
                    int              iMaxItemLabelLength,
                    int              iNonSelectedIconIndex,
                    int              iSelectedIconIndex,
                    CString          csToolTip,
                    const CDictItem* pDictItem = NULL);

    CTreeItemInfo(  int     iTypeOfInfo,
                    CString csKey,
                    CString csToolTip,
                    int     iNonSelectedIconIndex,
                    int     iSelectedIconIndex);

    virtual ~CTreeItemInfo();

public:
    CArray<DWORD,DWORD> m_dwArray;

protected:
    //int            m_iNodeIdx;
    int              m_iTypeOfInfo;
    int              m_iIndex;
    int              m_iOccurrenceIndex;
    int              m_iMaxNumOccurrences;
    int              m_iMaxItemLabelLength;
    int              m_iNonSelectedIconIndex;
    int              m_iSelectedIconIndex;           //FABN 14/Aug/2002
    CString          m_csKey;                        //FABN 22/Aug/2002
    bool             m_bIsExpanded;                  //FABN 22/Aug/2002
    CString          m_csLabel;
    CString          m_csToolTip;

    HTREEITEM        m_hitemParent;
    CTreeItemInfo*   m_pItemInfoParent;

    CDEItemBase*     m_pItem;
    CDELevel*        m_pLevel;
    const CDictItem* m_pDictItem;
};
