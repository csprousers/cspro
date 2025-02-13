#include "StdAfx.h"
#include "DragOptions.h"


namespace RK // RK = Registry Key
{
    const TCHAR* const Settings                  = _T("Settings");

    const TCHAR* const TextLayout                = _T("Text Position");
    const TCHAR* const TextUse                   = _T("Use Names");
    const TCHAR* const LinkFieldTextToDictionary = _T("Link Field Label");
    const TCHAR* const RosterUse                 = _T("Roster");
    const TCHAR* const UseOccurrenceLabels       = _T("Occurrence Labels");
    const TCHAR* const UseExtendedControls       = _T("Extended Controls");
    const TCHAR* const UseSubitems               = _T("Other Opt#2");
    const TCHAR* const UseEnterKey               = _T("Other Opt#1");

#ifdef WIN_DESKTOP

    int Get(const TCHAR* key_name, int default_value)
    {
        return AfxGetApp()->GetProfileInt(Settings, key_name, default_value);
    }

    void Put(const TCHAR* key_name, int value)
    {
        AfxGetApp()->WriteProfileInt(Settings, key_name, value);
    }

#else

    int Get(const TCHAR* /*key_name*/, int default_value)
    {
        return default_value;
    }

    void Put(const TCHAR* /*key_name*/, int /*value*/)
    {
    }

#endif
}


DragOptions::DragOptions()
{
    // load the settings from the registry
    m_textLayout = ( RK::Get(RK::TextLayout, 0) == 0 ) ? CDEFormBase::TextLayout::Left :
                                                         CDEFormBase::TextLayout::Right;

    m_textUse = ( RK::Get(RK::TextUse, 0) == 0 ) ? CDEFormBase::TextUse::Label :
                                                   CDEFormBase::TextUse::Name;

    m_linkFieldTextToDictionary = ( RK::Get(RK::LinkFieldTextToDictionary, 0) == 1 );

    int serialized_roster_use =  RK::Get(RK::RosterUse, 1);
    m_rosterUse = ( serialized_roster_use == 0 ) ? RosterUse::None :
                  ( serialized_roster_use == 1 ) ? RosterUse::Horizontal :
                                                   RosterUse::Vertical;

	m_useOccurrenceLabels = ( RK::Get(RK::UseOccurrenceLabels, 0) == 1 );

    m_useExtendedControls = ( RK::Get(RK::UseExtendedControls, 1) == 1 );

    m_useSubitems = ( RK::Get(RK::UseSubitems, 0) == 1 );

    m_useEnterKey = ( RK::Get(RK::UseEnterKey, 0) == 1 );
}


void DragOptions::SaveToRegistry() const
{
    RK::Put(RK::TextLayout, (int)m_textLayout);

    RK::Put(RK::TextUse, (int)m_textUse);

    RK::Put(RK::LinkFieldTextToDictionary, (int)m_linkFieldTextToDictionary);

    RK::Put(RK::RosterUse, ( m_rosterUse == RosterUse::None )       ? 0 :
                           ( m_rosterUse == RosterUse::Horizontal ) ? 1 :
                                                                      2);

    RK::Put(RK::UseOccurrenceLabels, (int)m_useOccurrenceLabels);

    RK::Put(RK::UseExtendedControls, (int)m_useExtendedControls);

    RK::Put(RK::UseSubitems, (int)m_useSubitems);

    RK::Put(RK::UseEnterKey, (int)m_useEnterKey);
}


RosterOrientation DragOptions::GetRosterOrientation() const
{
    return ( m_rosterUse == RosterUse::Horizontal ) ? RosterOrientation::Horizontal :
           ( m_rosterUse == RosterUse::Vertical )   ? RosterOrientation::Vertical :
                                                      ReturnProgrammingError(FormDefaults::RosterOrientation);
}
