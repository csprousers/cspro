#include "StdAfx.h"
#include <engine/Engine.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zMessageO/Messages.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>
#include <zUtilF/NoteEditDlg.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/EngineRecord.h>
#include <zEngineF/EngineUI.h>
#include <zEngineF/ReviewNotesDlg.h>


const Pre74_CaseLevel* CEngineDriver::FindNoteCaseLevel_pre80(const DICX* pDicX, int field_symbol) const
{
    // with the input dictionary, non-case notes have to be on the proper level
    if( pDicX == DIX(0) && !NPT(field_symbol)->IsA(SymbolType::Pre80Dictionary) )
    {
        return m_aLoadedLevels[SymbolCalculator::GetLevelNumber_base1(NPT_Ref(field_symbol)) - 1].m_caseLevel;
    }

    else
    {
        return pDicX->GetCase().GetPre74_Case()->GetRootLevel();
    }
}


NoteByCaseLevel* CEngineDriver::GetNoteByCaseLevel_pre80(const NamedReference& named_reference,
    const std::optional<CString>& operator_id, int field_symbol) const
{
    DICX* pDicX = SymbolCalculator(GetSymbolTable()).GetDicT(NPT_Ref(field_symbol))->GetDicX();

    // get the case level for this note
    const Pre74_CaseLevel* pre74_case_level = FindNoteCaseLevel_pre80(pDicX, field_symbol);

    const std::vector<Note>& notes = pDicX->GetCase().GetNotes();
    NoteByCaseLevel* first_note_by_case_level = nullptr;

    for( auto& note_by_case_level : pDicX->GetNotesByCaseLevel() )
    {
        // make sure the note is on a currently loaded case level
        if( note_by_case_level.pre74_case_level != pre74_case_level )
            continue;

        const Note& note = notes[note_by_case_level.note_index];

        // check that the name and occurrences match
        if( !named_reference.NameAndOccurrencesMatch(note.GetNamedReference()) )
            continue;

        // if an operator ID is explicitly specified, only the note matching that ID is returned
        // if no operator ID is specified, the priority of the note returned is:
        //      1) the note with a blank operator ID
        //      2) the first note

        if( operator_id.has_value() )
        {
            if( operator_id->Compare(note.GetOperatorId()) == 0 )
                return &note_by_case_level;
        }

        else
        {
            if( note.GetOperatorId().IsEmpty() )
            {
                return &note_by_case_level;
            }

            else if( first_note_by_case_level == nullptr )
            {
                first_note_by_case_level = &note_by_case_level;
            }
        }
    }

    return first_note_by_case_level;
}


Note* CEngineDriver::FindNote(const NamedReference& named_reference, const std::optional<CString>& operator_id, int field_symbol) const
{
    Symbol* symbol = NPT(field_symbol);
    int level_number_base1 = SymbolCalculator::GetLevelNumber_base1(*symbol);

    EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(*symbol);
    ASSERT(engine_dictionary != nullptr && engine_dictionary->HasEngineCase());

    auto& engine_case = engine_dictionary->GetEngineCase();
    auto& data_case = engine_case.GetCase();

    // find the note
    Note* first_note_found = nullptr;

    for( auto& note : data_case.GetNotes() )
    {
        // check that the name and occurrences match
        if( !named_reference.NameAndOccurrencesMatch(note.GetNamedReference()) )
            continue;

        // check that this note is on the right level
        if( !named_reference.GetLevelKey().IsEmpty() )
        {
            // ENGINECR_TODO need to make sure that if a second-level ID changes, that notes are updated; [test this level check also]
            if( named_reference.GetLevelKey().Compare(engine_case.GetCurrentCaseLevel_base1(level_number_base1)->GetLevelKey()) != 0 )
                continue;
        }

        // if an operator ID is explicitly specified, only the note matching that ID is returned
        // if no operator ID is specified, the priority of the note returned is:
        //      1) the note with a blank operator ID
        //      2) the first note
        if( operator_id.has_value() )
        {
            if( operator_id->Compare(note.GetOperatorId()) == 0 )
                return &note;
        }

        else
        {
            if( note.GetOperatorId().IsEmpty() )
            {
                return &note;
            }

            else if( first_note_found == nullptr )
            {
                first_note_found = &note;
            }
        }
    }

    return first_note_found;
}


void CEngineDriver::GetNamedReferenceFromField(const DEFLD& defld, std::shared_ptr<NamedReference>& named_reference, int& field_symbol)
{
    field_symbol = defld.GetSymbol();

    const VART* pVarT = VPT(field_symbol);

    auto case_item_reference = std::make_shared<CaseItemReference>(*pVarT->GetCaseItem(), CString());
    m_pIntDriver->ConvertIndex(defld, *case_item_reference);

    named_reference = case_item_reference;
}


CString CEngineDriver::GetNoteContent(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id, int field_symbol)
{
    const EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(NPT_Ref(field_symbol));

    if( engine_dictionary != nullptr )
    {
        const Note* note = FindNote(*named_reference, operator_id, field_symbol);
        return ( note != nullptr ) ? note->GetContent() : CString();
    }

    else
    {
        DICX* pDicX = SymbolCalculator(GetSymbolTable()).GetDicT(NPT_Ref(field_symbol))->GetDicX();
        const NoteByCaseLevel* note_by_case_level = GetNoteByCaseLevel_pre80(*named_reference, operator_id, field_symbol);
        return ( note_by_case_level != nullptr ) ? pDicX->GetCase().GetNotes()[note_by_case_level->note_index].GetContent() : CString();
    }
}


void CEngineDriver::SetNotesModified(const Symbol* dictionary_symbol)
{
    if( dictionary_symbol->IsA(SymbolType::Dictionary) )
    {
        // ENGINECR_TODO implement SetNotesModified
    }

    else
    {
        // the notes modified flag is only for the entry input dictionary
        if( UsingWriteCaseParameter() && ((const DICT*)dictionary_symbol)->GetDicX() == DIX(0) )
            GetWriteCaseParameter()->SetNotesModified();
    }
}


void CEngineDriver::CreateNote_pre80(DICX* pDicX, std::shared_ptr<NamedReference> named_reference,
    const CString& operator_id, const CString& note_content, int field_symbol)
{
    // add the note
    auto& notes = pDicX->GetCase().GetNotes();
    notes.emplace_back(Note(note_content, named_reference, operator_id));

    // and add the case level information
    pDicX->GetNotesByCaseLevel().push_back({ notes.size() - 1, FindNoteCaseLevel_pre80(pDicX, field_symbol) });

    SetNotesModified(pDicX->GetDicT());
}


void CEngineDriver::DeleteNote_pre80(DICX* pDicX, const NoteByCaseLevel& note_by_case_level)
{
    // remove the note
    size_t note_index = note_by_case_level.note_index;

    auto& notes = pDicX->GetCase().GetNotes();
    notes.erase(notes.begin() + note_index);

    // adjust all the other note indices and remove this entry
    auto& notes_by_case_level = pDicX->GetNotesByCaseLevel();
    bool erased_entry = false;

    for( auto note_by_case_level_itr = notes_by_case_level.begin(); note_by_case_level_itr != notes_by_case_level.end(); )
    {
        if( !erased_entry && &(*note_by_case_level_itr) == &note_by_case_level )
        {
            note_by_case_level_itr = notes_by_case_level.erase(note_by_case_level_itr);
            erased_entry = true;
        }

        else
        {
            if( note_by_case_level_itr->note_index >= note_index )
                --note_by_case_level_itr->note_index;

            ++note_by_case_level_itr;
        }
    }

    SetNotesModified(pDicX->GetDicT());
}


void CEngineDriver::DeleteNote_pre80(DICX* pDicX, const Note& note)
{
    DICT* pDicT = pDicX->GetDicT();
    const CString& name = note.GetNamedReference().GetName();

    const Symbol* symbol = SO::Equals(pDicT->GetName(), name) ? pDicT : pDicT->FindChildSymbol(CS2WS(name));

    if( symbol == nullptr )
    {
        ASSERT(false);
        return;
    }

    const NoteByCaseLevel* note_by_case_level = GetNoteByCaseLevel_pre80(note.GetNamedReference(), note.GetOperatorId(), symbol->GetSymbolIndex());

    if( note_by_case_level == nullptr )
    {
        ASSERT(false);
        return;
    }

    DeleteNote_pre80(pDicX, *note_by_case_level);
}


void CEngineDriver::SetNote_pre80(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
                                  const CString& note_content, int field_symbol, bool add_paradata_event_if_applicable/* = true*/)
{
    DICX* pDicX = SymbolCalculator(GetSymbolTable()).GetDicT(NPT_Ref(field_symbol))->GetDicX();

    const NoteByCaseLevel* note_by_case_level = GetNoteByCaseLevel_pre80(*named_reference, operator_id, field_symbol);
    CString usable_operator_id = ValueOrDefault(operator_id);

    // if a blank string is passed as a note, the note will be deleted
    if( note_content.IsEmpty() )
    {
        if( note_by_case_level != nullptr )
            DeleteNote_pre80(pDicX, *note_by_case_level);
    }

    // modify the note if it already exists
    else if( note_by_case_level != nullptr )
    {
        auto& note = pDicX->GetCase().GetNotes()[note_by_case_level->note_index];

        if( note.GetContent().Compare(note_content) != 0 )
        {
            note.SetContent(note_content);
            SetNotesModified(pDicX->GetDicT());
        }
    }

    // or create a new note
    else
    {
        CreateNote_pre80(pDicX, named_reference, usable_operator_id, note_content, field_symbol);
    }

    if( add_paradata_event_if_applicable && Paradata::Logger::IsOpen() )
    {
        const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(named_reference.get());
        ASSERT(case_item_reference == nullptr || NPT(field_symbol)->IsA(SymbolType::Variable));

        auto note_event = std::make_shared<Paradata::NoteEvent>(
            Paradata::NoteEvent::Source::PutNote,
            m_pIntDriver->m_pParadataDriver->CreateObject(NPT_Ref(field_symbol)),
            ( case_item_reference != nullptr ) ? m_pIntDriver->m_pParadataDriver->CreateFieldInfo(VPT(field_symbol), *case_item_reference) : nullptr,
            usable_operator_id);

        const bool record_values = m_pPifFile->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordValues();
        note_event->SetPostEditValues(record_values ? std::make_optional(CS2WS(note_content)) : std::nullopt);

        m_pIntDriver->m_pParadataDriver->RegisterAndLogEvent(note_event);
    }
}

void CEngineDriver::SetNote(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
                            const CString& note_content, int field_symbol, bool add_paradata_event_if_applicable/* = true*/)
{
    EngineDictionary* engine_dictionary = SymbolCalculator::GetEngineDictionary(NPT_Ref(field_symbol));

    if( engine_dictionary == nullptr )
    {
        SetNote_pre80(named_reference, operator_id, note_content, field_symbol, add_paradata_event_if_applicable);
        return;
    }

    auto& engine_case = engine_dictionary->GetEngineCase();
    auto& notes = engine_case.GetCase().GetNotes();

    Note* note = FindNote(*named_reference, operator_id, field_symbol);
    CString usable_operator_id = ValueOrDefault(operator_id);


    // if a blank string is passed as a note, the note will be deleted
    if( note_content.IsEmpty() )
    {
        if( note != nullptr )
        {
            notes.erase(std::remove_if(notes.begin(), notes.end(), [&](const Note& this_note) { return ( note == &this_note ); }));
            SetNotesModified(engine_dictionary);
        }
    }

    // modify the note if it already exists
    else if( note != nullptr )
    {
        if( note->GetContent().Compare(note_content) != 0 )
        {
            note->SetContent(note_content);
            SetNotesModified(engine_dictionary);
        }
    }

    // or create a new note
    else
    {
        notes.emplace_back(note_content, named_reference, usable_operator_id);
        SetNotesModified(engine_dictionary);
    }


    if( add_paradata_event_if_applicable && Paradata::Logger::IsOpen() )
    {
        const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(named_reference.get());
        ASSERT(case_item_reference == nullptr || NPT(field_symbol)->IsA(SymbolType::Variable));

        auto note_event = std::make_shared<Paradata::NoteEvent>(
            Paradata::NoteEvent::Source::PutNote,
            m_pIntDriver->m_pParadataDriver->CreateObject(NPT_Ref(field_symbol)),
            ( case_item_reference != nullptr ) ? m_pIntDriver->m_pParadataDriver->CreateFieldInfo(VPT(field_symbol), *case_item_reference) : nullptr,
            usable_operator_id);

        const bool record_values = m_pPifFile->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordValues();
        note_event->SetPostEditValues(record_values ? std::make_optional(CS2WS(note_content)) : std::nullopt);

        m_pIntDriver->m_pParadataDriver->RegisterAndLogEvent(note_event);
    }
}


std::tuple<CString, bool> CEngineDriver::EditNote(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
                                                  int field_symbol, bool called_from_interface/* = true*/)
{
    if( !UseHtmlDialogs() )
        return EditNote_pre77(named_reference, operator_id, field_symbol, called_from_interface);

    CString usable_operator_id = ValueOrDefault(operator_id);
    CString original_note_content = m_pEngineDriver->GetNoteContent(named_reference, operator_id, field_symbol);
    const std::wstring escaped_original_note_content = m_pIntDriver->ConvertV0Escapes(CS2WS(original_note_content), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes);
    bool case_note = false;

    // generate the dialog title
    const Symbol& symbol = NPT_Ref(field_symbol);

    CString title_prefix;
    CString field_label;

    auto set_values = [&](const TCHAR* prefix, bool set_label)
    {
        title_prefix = prefix;

        if( set_label )
            field_label = WS2CS(SymbolCalculator::GetLabel(symbol));
    };

    if( symbol.IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
    {
        set_values(_T("Case"), false);
        case_note = true;
    }

    else if( symbol.IsA(SymbolType::Group) )
    {
        set_values(_T("Node"), false);
    }

    else if( symbol.IsOneOf(SymbolType::Record, SymbolType::Section) )
    {
        set_values(_T("Record"), true);
    }

    else if( symbol.IsA(SymbolType::Variable) )
    {
        set_values(_T("Field"), true);
    }

    else
    {
        ASSERT(false);
    }

    CString occurrences = named_reference->GetMinimalOccurrencesText();

    std::wstring title = FormatTextCS2WS(_T("%s Note%s%s%s%s"), title_prefix.GetString(),
                                         field_label.IsEmpty() ? _T("") : _T(": "), field_label.GetString(),
                                         occurrences.IsEmpty() ? _T("") : _T(" "), occurrences.GetString());

    std::unique_ptr<Paradata::NoteEvent> note_event;

    if( Paradata::Logger::IsOpen() )
    {
        const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(named_reference.get());
        ASSERT(case_item_reference == nullptr || symbol.IsA(SymbolType::Variable));

        note_event = std::make_unique<Paradata::NoteEvent>(
            called_from_interface ? Paradata::NoteEvent::Source::Interface : Paradata::NoteEvent::Source::EditNote,
            m_pIntDriver->m_pParadataDriver->CreateObject(symbol),
            ( case_item_reference != nullptr ) ? m_pIntDriver->m_pParadataDriver->CreateFieldInfo(assert_cast<const VART*>(&symbol), *case_item_reference) : nullptr,
            usable_operator_id);
    }

    NoteEditDlg note_edit_dlg(std::move(title), escaped_original_note_content);

    if( note_edit_dlg.DoModalOnUIThread() == IDOK )
    {
        if( escaped_original_note_content != note_edit_dlg.GetNote() )
        {
            std::wstring modified_note_content = m_pIntDriver->ApplyV0Escapes(note_edit_dlg.GetNote(), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes);
            SetNote(named_reference, operator_id, WS2CS(modified_note_content), field_symbol, false);

            if( note_event != nullptr )
            {
                const bool record_values = m_pPifFile->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordValues();
                note_event->SetPostEditValues(record_values ? std::make_optional(modified_note_content) : std::nullopt);
                m_pIntDriver->m_pParadataDriver->RegisterAndLogEvent(std::move(note_event));
            }

#ifndef WIN_DESKTOP
            // refresh the page contents in case this note is currently showing
            if( !case_note )
                PlatformInterface::GetInstance()->GetApplicationInterface()->RefreshPage(RefreshPageContents::Notes);
#endif

            return std::make_tuple(WS2CS(modified_note_content), true);
        }
    }

    return std::make_tuple(original_note_content, false);
}


std::tuple<CString, bool> CEngineDriver::EditNote_pre77(std::shared_ptr<NamedReference> named_reference, const std::optional<CString>& operator_id,
                                                        int field_symbol, bool called_from_interface/* = true*/)
{
    CString usable_operator_id = ValueOrDefault(operator_id);
    CString original_note_content = m_pEngineDriver->GetNoteContent(named_reference, operator_id, field_symbol);
    bool case_note = false;

    // generate the dialog title
    const int MaximumLabelDisplayLength = 32;
    const Symbol* symbol = NPT(field_symbol);

    CString title_prefix;
    CString field_label;

    if( symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
    {
        title_prefix = _T("Case");
        case_note = true;
    }

    else if( symbol->IsA(SymbolType::Group) )
    {
        title_prefix = _T("Node");
    }

    else if( symbol->IsA(SymbolType::Record) )
    {
        title_prefix = _T("Record");
        field_label = assert_cast<const EngineRecord*>(symbol)->GetDictionaryRecord().GetLabel();
    }

    else if( symbol->IsA(SymbolType::Section) )
    {
        title_prefix = _T("Record");
        field_label = assert_cast<const SECT*>(symbol)->GetDictRecord()->GetLabel();
    }

    else if( symbol->IsA(SymbolType::Variable) )
    {
        title_prefix = _T("Field");
        field_label = assert_cast<const VART*>(symbol)->GetDictItem()->GetLabel();
    }

    else
    {
        ASSERT(false);
    }

    if( field_label.GetLength() > MaximumLabelDisplayLength )
    {
        field_label.Truncate(MaximumLabelDisplayLength);
        field_label.AppendFormat(_T("..."));
    }

    CString occurrences = named_reference->GetMinimalOccurrencesText();

    CString title = FormatText(_T("%s Note%s%s%s%s"), title_prefix.GetString(), field_label.IsEmpty() ? _T("") : _T(": "), field_label.GetString(),
                                                      occurrences.IsEmpty() ? _T("") : _T(" "), occurrences.GetString());

    std::shared_ptr<Paradata::NoteEvent> note_event;

    if( Paradata::Logger::IsOpen() )
    {
        const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(named_reference.get());
        ASSERT(case_item_reference == nullptr || NPT(field_symbol)->IsA(SymbolType::Variable));

        note_event = std::make_shared<Paradata::NoteEvent>(
            called_from_interface ? Paradata::NoteEvent::Source::Interface : Paradata::NoteEvent::Source::EditNote,
            m_pIntDriver->m_pParadataDriver->CreateObject(NPT_Ref(field_symbol)),
            ( case_item_reference != nullptr ) ? m_pIntDriver->m_pParadataDriver->CreateFieldInfo(VPT(field_symbol), *case_item_reference) : nullptr,
            usable_operator_id);
    }

    CString escaped_note_content = WS2CS(m_pIntDriver->ConvertV0Escapes(CS2WS(original_note_content), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes));
    CString original_escaped_note_content = escaped_note_content;

    EngineUI::EditNoteNode edit_note_node { escaped_note_content, title, case_note };
    SendEngineUIMessage(EngineUI::Type::EditNote, edit_note_node);

    // if the note was modified, update it
    if( escaped_note_content != original_escaped_note_content )
    {
        std::wstring modified_note_content = m_pIntDriver->ApplyV0Escapes(CS2WS(escaped_note_content), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes);

        SetNote(named_reference, operator_id, WS2CS(modified_note_content), field_symbol, false);

        if( note_event != nullptr )
        {
            const bool record_values = m_pPifFile->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordValues();
            note_event->SetPostEditValues(record_values ? std::make_optional(modified_note_content) : std::nullopt);
            m_pIntDriver->m_pParadataDriver->RegisterAndLogEvent(note_event);
        }

        return std::make_tuple(WS2CS(modified_note_content), true);
    }

    return std::make_tuple(original_note_content, false);
}


void CEngineDriver::ParseCaseNotes_pre80(DICX* pDicX)
{
    // link each note with the case level that it's currently on
    auto& data_case = pDicX->GetCase();
    auto& notes = data_case.GetNotes();

    auto& notes_by_case_level = pDicX->GetNotesByCaseLevel();
    notes_by_case_level.clear();

    // get out quickly if there are no notes
    if( notes.empty() )
        return;

    const Pre74_Case* pCase = data_case.GetPre74_Case();
    const std::vector<const Pre74_CaseLevel*>& case_levels = pCase->GetLevelArray();

    for( auto note_itr = notes.begin(); note_itr != notes.end(); )
    {
        const CString& level_key = note_itr->GetNamedReference().GetLevelKey();
        const Pre74_CaseLevel* pre74_case_level = nullptr;

        if( level_key.IsEmpty() )
        {
            pre74_case_level = pCase->GetRootLevel();
        }

        else
        {
            // start at 1 to skip checking the root case level
            for( size_t i = 1; i < case_levels.size(); ++i )
            {
                if( pCase->GetLevelKey(case_levels[i]).Compare(level_key) == 0 )
                {
                    pre74_case_level = case_levels[i];
                    break;
                }
            }
        }

        // remove the note if there is no case level match
        if( pre74_case_level == nullptr )
        {
            note_itr = notes.erase(note_itr);
        }

        else
        {
            notes_by_case_level.push_back({ (size_t)( note_itr - notes.begin() ), pre74_case_level });
            ++note_itr;
        }
    }
}


void CEngineDriver::UpdateCaseNotesLevelKeys_pre80(DICX* pDicX)
{
    // level keys only need to be updated in multiple level applications
    if( !pDicX->GetDicT()->IsMultiLevel() )
        return;

    ASSERT(pDicX == DIX(0));

    auto& data_case = pDicX->GetCase();
    auto& notes = data_case.GetNotes();

    if( notes.empty() )
        return;

    const Pre74_Case* pCase = data_case.GetPre74_Case();
    const std::vector<const Pre74_CaseLevel*>& case_levels = pCase->GetLevelArray();

    std::vector<size_t> note_indices_to_remove;
    bool level_key_modified = false;

    for( const NoteByCaseLevel& note_by_case_level : pDicX->GetNotesByCaseLevel() )
    {
        // mark the note for removal if the case level was removed
        if( std::find(case_levels.cbegin(), case_levels.cend(), note_by_case_level.pre74_case_level) == case_levels.cend() )
        {
            note_indices_to_remove.push_back(note_by_case_level.note_index);
        }

        // otherwise update the level key if not the root level
        else if( note_by_case_level.pre74_case_level != case_levels[0] )
        {
            const CString& new_level_key = pCase->GetLevelKey(note_by_case_level.pre74_case_level);

            // the level_key_modified flag is a simple optimization so that, if a level key has
            // already been modified, the key comparison isn't done anymore because we know the notes have been modified
            if( level_key_modified || notes[note_by_case_level.note_index].GetNamedReference().GetLevelKey().Compare(new_level_key) != 0 )
            {
                notes[note_by_case_level.note_index].GetNamedReference().SetLevelKey(new_level_key);

                if( !level_key_modified )
                {
                    level_key_modified = true;
                    SetNotesModified(pDicX->GetDicT());
                }
            }
        }
    }

    if( !note_indices_to_remove.empty() )
    {
        // sort in reverse order
        std::sort(note_indices_to_remove.begin(), note_indices_to_remove.end(), [](size_t a, size_t b) { return ( a > b ); });

        for( size_t note_index : note_indices_to_remove )
            notes.erase(notes.begin() + note_index);

        if( !level_key_modified )
            SetNotesModified(pDicX->GetDicT());
    }
}


std::vector<std::wstring> CEngineDriver::GetOccurrenceLabels(const VART* pVarT, const CaseItemReference* case_item_reference/* = nullptr*/)
{
    // return record and item/subitem occurrence labels
    std::vector<std::wstring> occurrence_labels(2);

    const Symbol* symbol_with_occs = SymbolCalculator::GetFirstSymbolWithOccurrences(*pVarT);

    // process items with occurrences
    if( pVarT == symbol_with_occs )
    {
        std::optional<int> zero_based_occurrence;

        if( case_item_reference != nullptr )
        {
            zero_based_occurrence = case_item_reference->GetItemIndexHelper().HasSubitemOccurrences() ?
                case_item_reference->GetSubitemOccurrence() : case_item_reference->GetItemOccurrence();
        }

        occurrence_labels[1] = CS2WS(m_pIntDriver->EvaluateOccurrenceLabel(symbol_with_occs, zero_based_occurrence));

        // properly assign the symbol with occurrences for the record occurrence label (which will be located two levels up)
        const GROUPT* pGroupT = ((const VART*)symbol_with_occs)->GetOwnerGPT()->GetOwnerGPT();
        symbol_with_occs = SymbolCalculator::GetFirstSymbolWithOccurrences(*pGroupT);
    }

    // process records with occurrences
    if( symbol_with_occs != nullptr )
    {
        std::optional<int> zero_based_occurrence;

        if( case_item_reference != nullptr )
            zero_based_occurrence = case_item_reference->GetRecordOccurrence();

        occurrence_labels[0] = CS2WS(m_pIntDriver->EvaluateOccurrenceLabel(symbol_with_occs, zero_based_occurrence));
    }

    return occurrence_labels;
}


std::shared_ptr<const CaseItemReference> CEngineDriver::ReviewNotes()
{
    // ENGINECR_TODO implement "review notes"
    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();

    std::vector<ReviewNotesDlg::ReviewNote> review_notes;

    for( const Note& note : pDicX->GetCase().GetNotes() )
    {
        const NamedReference& named_reference = note.GetNamedReference();

        // only show notes on the first level
        if( !named_reference.GetLevelKey().IsEmpty() )
            continue;

        ReviewNotesDlg::ReviewNote& review_note = review_notes.emplace_back(ReviewNotesDlg::ReviewNote { note });

        review_note.content = m_pIntDriver->ConvertV0Escapes(CS2WS(note.GetContent()), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes);

        const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(&named_reference);

        if( case_item_reference == nullptr )
        {
            if( SO::Equals(named_reference.GetName(), pDicT->GetName()) )
            {
                review_note.label = _T("Case Note");
                review_note.sort_index = _T("!");
            }

            else
            {
                review_note.label = named_reference.GetName();
                review_note.sort_index = FormatTextCS2WS(_T("#%s"), named_reference.GetName().GetString());
            }
        }

        else
        {
            const VART* field_vart = VPT(case_item_reference->GetCaseItem().GetDictionaryItem().GetSymbol());
            const GROUPT* record_group = field_vart->GetOwnerGPT();

            review_note.can_goto = ( Issamod == ModuleType::Entry );
            review_note.label = field_vart->GetDictItem()->GetLabel();

            review_note.group_symbol_index = record_group->GetSymbolIndex();
            review_note.group_label = record_group->GetLabel();

            // get the record and item occurrence labels
            const TCHAR* occurrence_label_formatter = _T(" (%s)");
            std::vector<std::wstring> occurrence_labels = GetOccurrenceLabels(field_vart, case_item_reference);

            if( !occurrence_labels[0].empty() )
                SO::AppendFormat(review_note.group_label, occurrence_label_formatter, occurrence_labels[0].c_str());

            if( !occurrence_labels[1].empty() )
            {
                SO::AppendFormat(review_note.label, occurrence_label_formatter, occurrence_labels[1].c_str());
            }

            // add the item occurrence numbers if there is no occurrence label
            else
            {
                review_note.label.append(case_item_reference->GetMinimalOccurrencesText());
            }


            // have this note sorted in the order that it appears in the data entry application; this
            // calculation might not work in all situations but it is good enough for these purposes
            const GROUPT* item_group = nullptr;

            if( record_group->GetDimType() == CDimension::VDimType::Item ||
                record_group->GetDimType() == CDimension::VDimType::SubItem )
            {
                item_group = record_group;
                record_group = record_group->GetOwnerGPT();
            }

            constexpr const TCHAR* FlowOrderFormatter = _T("%07d");
            constexpr const TCHAR* OccurrenceFormatter = _T("%05d");

            review_note.sort_index = FormatTextCS2WS(FlowOrderFormatter, record_group->GetAbsoluteFlowOrder());
            SO::AppendFormat(review_note.sort_index, OccurrenceFormatter, static_cast<int>(case_item_reference->GetRecordOccurrence()));

            if( item_group != nullptr )
            {
                const size_t item_subitem_occurrence =
                    case_item_reference->GetItemIndexHelper().HasSubitemOccurrences() ? case_item_reference->GetSubitemOccurrence() :
                                                                                        case_item_reference->GetItemOccurrence();

                SO::AppendFormat(review_note.sort_index, FlowOrderFormatter, item_group->GetAbsoluteFlowOrder());
                SO::AppendFormat(review_note.sort_index, OccurrenceFormatter, static_cast<int>(item_subitem_occurrence));
            }

            SO::AppendFormat(review_note.sort_index, FlowOrderFormatter, field_vart->GetAbsoluteFlowOrder());
        }
    }


    if( review_notes.empty() )
    {
        ErrorMessage::Display(MGF::GetMessageText(46512, _T("There are no notes to review.")));
        return nullptr;
    }

    // sort the notes...
    std::sort(review_notes.begin(), review_notes.end(),
        [](const ReviewNotesDlg::ReviewNote& review_note1, const ReviewNotesDlg::ReviewNote& review_note2)
        {
            return ( review_note1.sort_index.compare(review_note2.sort_index) < 0 );
        });

    // ...and group by unique group label
    std::vector<std::vector<const ReviewNotesDlg::ReviewNote*>> grouped_review_notes;

    for( const ReviewNotesDlg::ReviewNote& review_note : review_notes )
    {
        if( grouped_review_notes.empty() ||
            grouped_review_notes.back().front()->group_symbol_index != review_note.group_symbol_index ||
            grouped_review_notes.back().front()->group_label != review_note.group_label )
        {
            grouped_review_notes.emplace_back();
        }

        grouped_review_notes.back().emplace_back(&review_note);
    }

    // show the dialog
    ReviewNotesDlg review_notes_dlg;

    review_notes_dlg.SetGroupedReviewNotes(std::move(grouped_review_notes));

    if( review_notes_dlg.DoModalOnUIThread() == IDOK )
    {
        // delete any notes; the note in ReviewNote is a copy, so there will be no
        // problem passing it to routines that modify the underlying notes vector
        for( const Note* note : review_notes_dlg.GetDeletedNotes() )
            DeleteNote_pre80(pDicX, *note);

        // return the field of the note that the user wants to go to
        if( review_notes_dlg.GetGotoNote() != nullptr )
            return std::dynamic_pointer_cast<const CaseItemReference, const NamedReference>(review_notes_dlg.GetGotoNote()->GetSharedNamedReference());
    }

    return nullptr;
}
