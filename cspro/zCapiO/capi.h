#pragma once

//////////////////////////////////////////////////////////////////////
//
// Capi.h: interface for the CCapi class.
//
//////////////////////////////////////////////////////////////////////

class CEntryDriver;
class CEngineArea;
class CWnd;
class CExtendedControl;

#include <zUtilO/imsaStr.h>
#include <engine/DEFLD.H>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zCapiO/CapiContent.h>
#include <zCapiO/CapiStyle.h>


class AFX_EXT_CLASS CCapi
{
private:
    CEntryDriver*       m_pEntryDriver;
    CEngineArea*        m_pEngineArea;

#ifdef WIN_DESKTOP
    CExtendedControl*   m_pExtendedControl;
    CWnd*               m_pAroundField;
    CWnd*               m_pFrameWindow;
    bool                m_bHasHlp;
    bool                m_showing_help;
#endif

public:
                    CCapi();
    virtual         ~CCapi();

    void            SetEntryDriver(CEntryDriver* pEntryDriver);

#ifdef WIN_DESKTOP
    int             DoLabelsModeless(const DEFLD* pDeField);
    void            DeleteLabels();

    void            DoQuestion(const DEFLD* pDeField);
    void            ShowHelp(const DEFLD* pDeField);
    void            ToggleHelp(const DEFLD* pDeField);

    CWnd*           GetAroundField() { return m_pAroundField; }
    void            SetAroundField(CWnd* pAroundField) { m_pAroundField = pAroundField; }
    void            SetFrameWindow(CWnd* pFrameWindow) { m_pFrameWindow = pFrameWindow; }

    void            RefreshPosition();
    void            CheckOverlap( bool bRefresh);
    void            CheckInZone( bool bRefresh );

    void            UpdateSelection(const CString& csText);
    bool            CheckInZone( CRect* pResponsesRect, CRect maxRect );
#endif

    enum class CapiContentType { Question, Help, All };
    void GetCapiContent(CapiContent* capi_content, int symbol_index, CapiContentType capi_content_type) const;
    CapiContent GetFieldAndBlockCombinedCapiContent(int symbol_index, CapiContentType capi_content_type) const;
    const std::wstring& GetRuntimeStylesCss();

private:
    void            Init();
    void            End();

#ifdef WIN_DESKTOP
    void            ResponseUnOverlap( CWnd* pWnd, CRect maxRect );
#endif

    const Logic::SymbolTable& GetSymbolTable() const;
};
