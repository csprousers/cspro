#include "StdAfx.h"
#include "TypeDlg.h"


BEGIN_MESSAGE_MAP(CSortTypeDlg, CDialog)
    ON_BN_CLICKED(IDC_QUEST_SORT, SetupRecSortOptions)
    ON_BN_CLICKED(IDC_QUEST_SORT_PLUS, SetupRecSortOptions)
    ON_BN_CLICKED(IDC_REC_SORT, SetupRecSortOptions)
    ON_BN_CLICKED(IDC_REC_SORT_USING, SetupRecSortOptions)
END_MESSAGE_MAP()


CSortTypeDlg::CSortTypeDlg(SortSpec& sort_spec, CWnd* pParent /*=nullptr*/)
    :   CDialog(CSortTypeDlg::IDD, pParent),
        m_sortSpec(sort_spec),
        m_sortType(static_cast<int>(m_sortSpec.GetSortType()))
{
}


void CSortTypeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio(pDX, IDC_QUEST_SORT, m_sortType);
}


BOOL CSortTypeDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CListCtrl* pBox = static_cast<CListCtrl*>(GetDlgItem(IDC_REC_LIST));

    CRect rect;
    pBox->GetWindowRect(&rect);
    pBox->InsertColumn(0, _T("Items"), LVCFMT_LEFT, rect.Width() - 4);

    SetupRecSortOptions();

    return TRUE;
}


void CSortTypeDlg::SetupRecSortOptions()
{
    UpdateData(TRUE);

    CListCtrl* pBox = static_cast<CListCtrl*>(GetDlgItem(IDC_REC_LIST));
    pBox->DeleteAllItems();

    if( m_sortType != static_cast<int>(SortSpec::SortType::RecordUsing) )
    {
        pBox->EnableWindow(FALSE);
    }

    else
    {
        pBox->EnableWindow(TRUE);

        // only calculate the records on demand
        if( m_dictRecords.empty() )
        {
            const CDataDict& dictionary = m_sortSpec.GetDictionary();

            for( const DictLevel& dict_level : dictionary.GetLevels() )
            {
                for( int r = 0; r < dict_level.GetNumRecords(); ++r )
                    m_dictRecords.emplace_back(dict_level.GetRecord(r));
            }
        }

        int selected_record_index = 0;
        int record_counter = 0;

        for( const CDictRecord* dict_record : m_dictRecords )
        {
            if( dict_record == m_sortSpec.GetRecordSortDictRecord() )
                selected_record_index = record_counter;

            pBox->InsertItem(record_counter++, dict_record->GetLabel());
        }

        pBox->SetItemState(selected_record_index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    }
}


void CSortTypeDlg::OnOK()
{
    UpdateData(TRUE);

    const SortSpec::SortType sort_type = static_cast<SortSpec::SortType>(m_sortType);
    const CDictRecord* record_sort_dict_record = nullptr;

    if( sort_type == SortSpec::SortType::RecordUsing )
    {
        const CListCtrl* pBox = static_cast<const CListCtrl*>(GetDlgItem(IDC_REC_LIST));

        const size_t selected_record_index = static_cast<size_t>(pBox->GetNextItem(-1, LVNI_SELECTED));
        ASSERT(selected_record_index < m_dictRecords.size());

        record_sort_dict_record = m_dictRecords[selected_record_index];
    }

    m_sortSpec.SetSortType(sort_type, record_sort_dict_record);

    CDialog::OnOK();
}
