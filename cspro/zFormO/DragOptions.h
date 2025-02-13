#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/FormFile.h>


class CLASS_DECL_ZFORMO DragOptions
{
    friend class DragOptionsDlg;

public:
    enum class RosterUse { Horizontal, Vertical, None };
    
    DragOptions();

    CDEFormBase::TextLayout GetTextLayout() const { return m_textLayout; }

    CDEFormBase::TextUse GetTextUse() const       { return m_textUse; }

    bool LinkFieldTextToDictionary() const        { return m_linkFieldTextToDictionary; }

    RosterUse GetRosterUse() const                { return m_rosterUse; }
    bool UseRosters() const                       { return ( m_rosterUse != RosterUse::None ); }

    RosterOrientation GetRosterOrientation() const;

    bool UseOccurrenceLabels() const              { return m_useOccurrenceLabels; }

    bool UseExtendedControls() const              { return m_useExtendedControls; }

    bool UseSubitems() const                      { return m_useSubitems; }

    bool UseEnterKey() const                      { return m_useEnterKey; }

private:
    void SaveToRegistry() const;

private:
    // the default action is to place a dictionary item's text portion
    // on the left side of the form, and its data entry box on the
    // form's right side
	CDEFormBase::TextLayout m_textLayout;
	CDEFormBase::TextUse m_textUse;

    // will the field label be updated if the dictionary label/name changes?
    bool m_linkFieldTextToDictionary;

    // if the item/record being dropped is a multiple, the item/record will be
    // rostered (if possible; location of drop may dictate differently)
    RosterUse m_rosterUse;

    // should occurrence labels be used in a roster
	bool m_useOccurrenceLabels;

    // should capture types default to CAPI-style capture types?
	bool m_useExtendedControls;

    // if for a given item, subitems are present, do you wish to use them?
    // (will only work if none of the item's subitems overlap)
    bool m_useSubitems;

    // must the data entry operator hit the 'enter' key after each field
    // is keyed, or will the cursor automatically advance to the next field?
    // if set to T, this will require keyer to press the enter key
    bool m_useEnterKey;
};
