#include "StdAfx.h"
#include "NewFileDlg.h"


IMPLEMENT_DYNAMIC(NewFileDlg, CDialog)

BEGIN_MESSAGE_MAP(NewFileDlg, CDialog)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_NEW_FILE_CATEGORIES, OnLvnItemchangedListCategory)
    ON_NOTIFY(NM_CLICK, IDC_NEW_FILE_CATEGORIES, OnClickList)
    ON_NOTIFY(NM_DBLCLK, IDC_NEW_FILE_CATEGORIES, OnDoubleClickList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_NEW_FILE_TYPES, OnLvnItemchangedListType)
    ON_NOTIFY(NM_CLICK, IDC_NEW_FILE_TYPES, OnClickList)
    ON_NOTIFY(NM_DBLCLK, IDC_NEW_FILE_TYPES, OnDoubleClickList)
END_MESSAGE_MAP()


namespace
{
    struct NewFileType
    {
        unsigned icon;
        AppFileType app_file_type;
        std::optional<EntryApplicationStyle> entry_application_type;
        const TCHAR* const name;
        const TCHAR* const description;
    };

    struct NewFileCategory
    {
        unsigned icon;
        const TCHAR* const name;
        const std::vector<NewFileType> types;
    };

    const std::vector<NewFileCategory> NewFileCategories =
    {
        {
            IDI_ENT_FILE_ALL_TYPES,
            _T("Entry"),
            {
                {
                    IDI_ENT_FILE_CAPI,
                    AppFileType::ApplicationEntry,
                    EntryApplicationStyle::CAPI,
                    _T("CAPI Data Entry Application"),
                    _T("CAPI: Computer Assisted Personal Interviewing\n\n")
                    _T("A data entry application will be created with settings tailored for conducting CAPI censuses and surveys. ")
                    _T("The program will use system-controlled mode to ensure enumerators strictly follow any value and logic checks.")
                },
                {
                    IDI_ENT_FILE,
                    AppFileType::ApplicationEntry,
                    EntryApplicationStyle::PAPI,
                    _T("PAPI Data Entry Application"),
                    _T("PAPI: Paper and Pencil Interviewing\n\n")
                    _T("A data entry application will be created with settings tailored for keying paper forms collected during a census or survey. ")
                    _T("The program will use operator-controlled mode to give keyers maximum flexibility while entering the data on the forms.")
                },
                {
                    IDI_ENT_FILE_OPERATIONAL_CONTROL,
                    AppFileType::ApplicationEntry,
                    EntryApplicationStyle::OperationalControl,
                    _T("Operational Control Application"),
                    _T("Operational Control\n\n")
                    _T("A data entry application will be created with settings tailored for managing operational control, which is sometimes called a menu program. ")
                    _T("These programs are not intended to collect data that will be saved, but rather to lead enumerators through a series of options, presenting actions to be taken.")
                }
            }
        },

        {
            IDI_BCH_FILE,
            _T("Batch"),
            {
                {
                    IDI_BCH_FILE,
                    AppFileType::ApplicationBatch,
                    std::nullopt,
                    _T("Batch Edit Application"),
                    _T("Batch edit applications are generally used to detect and correct errors in data files. They can also be used for running logic as a script, or to export data.")
                }
            }
        },

        {
            IDI_XTB_FILE,
            _T("Tabulation"),
            {
                {
                    IDI_XTB_FILE,
                    AppFileType::ApplicationTabulation,
                    std::nullopt,
                    _T("Tabulation Application"),
                    _T("Tabulation applications produce publication-ready tables.")
                }
            }
        },

        {
            IDI_RESOURCE_FOLDER,
            _T("Other"),
            {
                {
                    IDI_DCF_FILE,
                    AppFileType::Dictionary,
                    std::nullopt,
                    _T("Dictionary"),
                    _T("Dictionaries describe CSPro data files.\n\n")
                    _T("If you are creating a new application, you may prefer creating the entire application rather than a standalone dictionary.")
                },
                {
                    IDI_FRM_FILE,
                    AppFileType::Form,
                    std::nullopt,
                    _T("Form"),
                    _T("Forms contain the order and instructions for collecting information as part of a data entry application.\n\n")
                    _T("This option does not create a complete data entry application; it is most commonly used to bring in a standalone form used by several applications, such as a consent form. ")
                    _T("If you are creating a new application, you should select either the CAPI or PAPI entry options.")
                }
            }
        },
    };
}


NewFileDlg::NewFileDlg(CWnd* pParent /*= nullptr*/)
    :   CDialog(NewFileDlg::IDD, pParent),
        m_selectedCategory(0),
        m_appFileType(AppFileType::ApplicationEntry),
        m_entryApplicationStyle(EntryApplicationStyle::CAPI)
{
}


void NewFileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NEW_FILE_CATEGORIES, m_newFileCategories);
    DDX_Control(pDX, IDC_NEW_FILE_TYPES, m_newFileTypes);
}


BOOL NewFileDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_imageList.Create(32, 32, ILC_COLOR32, 0, 1);
    m_imageList.SetBkColor(GetSysColor(COLOR_WINDOW));

    // set up the icons,
    auto add_icon = [&](unsigned icon)
    {
        // only add the icon if it has not already been added
        if( m_imageMapping.insert({ icon, m_imageMapping.size() }).second )
            m_imageList.Add(AfxGetApp()->LoadIcon(icon));
        else
        {
            int x=icon;++x;
        }
    };

    for( const auto& new_file_category : NewFileCategories )
    {
        add_icon(new_file_category.icon);

        for( const auto& new_file_type : new_file_category.types )
            add_icon(new_file_type.icon);
    }

    m_newFileCategories.SetImageList(&m_imageList, LVSIL_SMALL);
    m_newFileTypes.SetImageList(&m_imageList, LVSIL_SMALL);


    // add the categories
    int category_count = 0;
    for( const auto& new_file_category : NewFileCategories )
        m_newFileCategories.InsertItem(category_count++, new_file_category.name, m_imageMapping[new_file_category.icon]);


    // preselect the first category
    m_newFileCategories.SetItemState(m_selectedCategory, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    return TRUE;
}


void NewFileDlg::OnLvnItemchangedListCategory(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) == LVIF_STATE &&
        ( pNMLV->uNewState & LVIS_SELECTED ) == LVIS_SELECTED && ( pNMLV->uOldState & LVIS_SELECTED ) == 0 )
    {
        m_selectedCategory = pNMLV->iItem;

        // update the file types
        m_newFileTypes.DeleteAllItems();

        int type_count = 0;
        for( const auto& new_file_type : NewFileCategories[m_selectedCategory].types )
            m_newFileTypes.InsertItem(type_count++, new_file_type.name, m_imageMapping[new_file_type.icon]);

        // preselect the first file type (or the type last selected)
        m_newFileTypes.SetItemState(m_selectedType[m_selectedCategory], LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    *pResult = 0;
}


void NewFileDlg::OnLvnItemchangedListType(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) == LVIF_STATE &&
        ( pNMLV->uNewState & LVIS_SELECTED ) == LVIS_SELECTED && ( pNMLV->uOldState & LVIS_SELECTED ) == 0 )
    {
        m_selectedType[m_selectedCategory] = pNMLV->iItem;

        const auto& new_file_type = NewFileCategories[m_selectedCategory].types[pNMLV->iItem];
        SetDlgItemText(IDC_NEW_FILE_DESCRIPTION, new_file_type.description);
    }

    *pResult = 0;
}


void NewFileDlg::OnClickList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // always make sure something is selected
    if( pNMLV->iItem == -1 )
    {
        if( pNMHDR->hwndFrom == m_newFileCategories.GetSafeHwnd() )
        {
            m_newFileCategories.SetItemState(m_selectedCategory, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }

        else
        {
            m_newFileTypes.SetItemState(m_selectedType[m_selectedCategory], LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }

    *pResult = 0;
}


void NewFileDlg::OnDoubleClickList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // allow double-clicking to select the item
    PostMessage(WM_COMMAND, IDOK);
    *pResult = 0;
}


void NewFileDlg::OnOK()
{
    const auto& new_file_type = NewFileCategories[m_selectedCategory].types[m_selectedType[m_selectedCategory]];

    m_appFileType = new_file_type.app_file_type;
    m_entryApplicationStyle = new_file_type.entry_application_type.value_or(m_entryApplicationStyle);

    CDialog::OnOK();
}
