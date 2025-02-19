#pragma once

#include <engine/ParadataDriver.h>
#include <zUtilF/SelectDlg.h>
#include <zParadataO/Logger.h>


class SelectDlgHelper
{
public:
    SelectDlgHelper(EngineParadataDriver& paradata_driver, SelectDlg& select_dlg,
                    Paradata::OperatorSelectionEvent::Source operator_selection_event_source);

    int GetSingleSelection();

    const std::optional<std::set<size_t>>& GetMultipleSelections();

private:
    void FinalizeParadataEvent();

private:
    EngineParadataDriver& m_paradataDriver;
    SelectDlg& m_selectDlg;
    std::unique_ptr<Paradata::OperatorSelectionEvent> m_operatorSelectionEvent;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SelectDlgHelper::SelectDlgHelper(EngineParadataDriver& paradata_driver, SelectDlg& select_dlg,
                                        Paradata::OperatorSelectionEvent::Source operator_selection_event_source)
    :   m_paradataDriver(paradata_driver),
        m_selectDlg(select_dlg)
{
    if( Paradata::Logger::IsOpen() )
        m_operatorSelectionEvent = std::make_unique<Paradata::OperatorSelectionEvent>(operator_selection_event_source);
}


inline int SelectDlgHelper::GetSingleSelection()
{
    if( m_selectDlg.GetRows().empty() )
        return 0;

    int selected_row_base_one = 0;

    if( m_selectDlg.DoModalOnUIThread() == IDOK )
    {
        ASSERT(m_selectDlg.GetSelectedRows().has_value() && m_selectDlg.GetSelectedRows()->size() == 1);
        selected_row_base_one = 1 + *m_selectDlg.GetSelectedRows()->cbegin();
    }

    FinalizeParadataEvent();

    return selected_row_base_one;
}


inline const std::optional<std::set<size_t>>& SelectDlgHelper::GetMultipleSelections()
{
    if( !m_selectDlg.GetRows().empty() )
    {
        m_selectDlg.DoModalOnUIThread();

        FinalizeParadataEvent();
    }

    return m_selectDlg.GetSelectedRows();
}


inline void SelectDlgHelper::FinalizeParadataEvent()
{
    if( m_operatorSelectionEvent == nullptr )
        return;

    std::optional<int> selected_row_base_one;
    SharableString selected_text;

    // only log the first selection when multiple rows have been selected
    if( m_selectDlg.GetSelectedRows().has_value() && !m_selectDlg.GetSelectedRows()->empty() )
    {
        const size_t selected_row = *m_selectDlg.GetSelectedRows()->cbegin();
        selected_row_base_one = static_cast<int>(selected_row + 1);
        selected_text = m_selectDlg.GetRows()[selected_row].column_texts.front();
    }

    m_operatorSelectionEvent->SetPostSelectionValues(std::move(selected_row_base_one), std::move(selected_text), true);
    m_paradataDriver.RegisterAndLogEvent(std::move(m_operatorSelectionEvent));
}
