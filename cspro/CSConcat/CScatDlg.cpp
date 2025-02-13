#include "StdAfx.h"
#include "CScatDlg.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>
#include <zUtilO/PathHelpers.h>
#include <zConcatO/Concatenator.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <iterator>


BEGIN_MESSAGE_MAP(CCSConcatDlg, CDialog)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_RUN, OnOK)
    ON_BN_CLICKED(IDC_ADDFILES, OnAddFiles)
    ON_BN_CLICKED(IDC_OUTPUTOPEN, OnOutputOpen)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_CLEAR, OnClear)
    ON_NOTIFY(LVN_BEGINDRAG, IDC_FILELIST, OnBeginDragFileList)
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_NOTIFY(HDN_ENDDRAG, IDC_FILELIST, OnEndDragFileList)
    ON_EN_CHANGE(IDC_OUTPUT, OnChangeOutput)
    ON_BN_CLICKED(IDC_DICT_BROWSE, OnBnClickedDictBrowse)
    ON_EN_CHANGE(IDC_DICTIONARY, OnEnChangeDictionary)
    ON_BN_CLICKED(IDC_CONCAT_METHOD_CASE, OnBnClickedConcatMethodCase)
    ON_BN_CLICKED(IDC_CONCAT_METHOD_FILE, OnBnClickedConcatMethodFile)
    ON_MESSAGE(UWM::CSConcat::UpdateDialogUI, OnUpdateDialogUI)
END_MESSAGE_MAP()


CCSConcatDlg::CCSConcatDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(CCSConcatDlg::IDD, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
    SetDefaultPffSettings();
}


void CCSConcatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FILELIST, m_fileList);
    DDX_Text(pDX, IDC_OUTPUT, m_outputConnectionString);
    DDX_Text(pDX, IDC_DICTIONARY, m_dictionaryFilename);
}


BOOL CCSConcatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the menu
    m_menu.LoadMenu(IDR_CSCONCAT_MENU);
    SetMenu(&m_menu);

    // set the icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_fileList.SetExtendedStyle( LVS_EX_FULLROWSELECT );
    m_fileList.SetHeadings(_T("Name,120;Directory,200;Type,120;Date,120;Size,90"));
    m_fileList.LoadColumnInfo();
    m_bDragging = FALSE;

    // set up the callback to allow the dragging of files onto the list of input data
    m_fileList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto,
        [&](const std::vector<std::wstring>& paths)
        {
            OnDropFiles(paths);
        });

    PostMessage(UWM::CSConcat::UpdateDialogUI);

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CCSConcatDlg::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void CCSConcatDlg::SetDefaultPffSettings()
{
    m_pff.ResetContents();

    m_pff.SetAppType(APPTYPE::CONCAT_TYPE);
    m_pff.SetConcatenateMethod(ConcatenateMethod::Case);
    m_pff.SetViewListing(VIEWLISTING::ALWAYS);
    m_pff.SetViewResultsFlag(true);
}


void CCSConcatDlg::OnFileOpen()
{
    CIMSAFileDialog file_dlg(TRUE, FileExtensions::Pff, nullptr, OFN_HIDEREADONLY, FileFilters::Pff);
    file_dlg.m_ofn.lpstrTitle = _T("Select Input PFF");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_pff.ResetContents();
    m_pff.SetPifFileName(file_dlg.GetPathName());

    if( !m_pff.LoadPifFile() || m_pff.GetAppType() != CONCAT_TYPE )
    {
        AfxMessageBox(_T("The PFF could not be read or was not a Concatenate Data PFF."));
        SetDefaultPffSettings();
    }

    m_outputConnectionString = m_pff.GetSingleOutputDataConnectionString();
    m_dictionaryFilename = m_pff.GetInputDictFName();

    m_fileList.DeleteAllItems();
    AddConnectionStrings(m_pff.GetInputDataConnectionStrings());

    UpdateData(FALSE);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


bool CCSConcatDlg::UIToPff(const bool show_errors)
{
    UpdateData(TRUE);

    m_pff.SetSingleOutputDataConnectionString(m_outputConnectionString);
    m_pff.SetInputDictFName(WS2CS(m_dictionaryFilename));

    m_pff.ClearInputDataConnectionStrings();

    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        ConnectionString connection_string(m_fileList.GetItemText(i, 0));
        connection_string.AdjustRelativePath(CS2WS(m_fileList.GetItemText(i, 1)));

        if( show_errors && m_pff.GetSingleOutputDataConnectionString().Equals(connection_string) )
        {
            const std::wstring message = FormatTextCS2WS(_T("Output file '%s' is also one of the files to concatenate.\n\n")
                                                         _T("If you proceed, it will concatentate correctly ")
                                                         _T("but the original copy will be lost.\n\nDo you want to do this?"),
                                                         m_pff.GetSingleOutputDataConnectionString().GetFilename().c_str());

            if( AfxMessageBox(message, MB_YESNO | MB_DEFBUTTON2) == IDNO )
                return false;
        }

        m_pff.AddInputDataConnectionString(std::move(connection_string));
    }

    return true;
}


void CCSConcatDlg::OnFileSaveAs()
{
    CIMSAFileDialog file_dlg(FALSE, FileExtensions::Pff, m_pff.GetPifFileName(), OFN_HIDEREADONLY, FileFilters::Pff);
    file_dlg.m_ofn.lpstrTitle = _T("Select Output PFF");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_pff.SetPifFileName(file_dlg.GetPathName());

    UIToPff(false);

    // base the listing filename on the PFF filename
    if( m_pff.GetListingFName().IsEmpty() )
        m_pff.SetListingFName(WS2CS(PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::WithDot::Listing)));

    m_pff.Save();
}


void CCSConcatDlg::AddConnectionStrings(const std::vector<ConnectionString>& connection_strings)
{
    for( const ConnectionString& connection_string : connection_strings )
    {
        for( const ConnectionString& expanded_connection_string : PathHelpers::ExpandConnectionStringWildcards(connection_string) )
        {
            if( expanded_connection_string.IsFilenamePresent() )
            {
                CFileStatus file_status;

                if( CFile::GetStatus(expanded_connection_string.GetFilename().c_str(), file_status) )
                {
                    m_fileList.AddItem(PortableFunctions::PathGetFilename(expanded_connection_string.GetFilename()),
                                       PortableFunctions::PathGetDirectory(expanded_connection_string.GetFilename()).c_str(),
                                       ToString(expanded_connection_string.GetType()),
                                       file_status.m_mtime.Format(_T("%c")).GetString(),
                                       PortableFunctions::FileSizeString(file_status.m_size).c_str());
                }
            }
        }
    }
}


void CCSConcatDlg::OnAddFiles()
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_outputConnectionString);
    data_file_dlg.SetTitle(_T("Select Files To Concatenate"))
                 .AllowMultipleSelections();

    if( m_pff.GetConcatenateMethod() == ConcatenateMethod::Case )
        data_file_dlg.SetDictionaryFilename(WS2CS(m_dictionaryFilename));

    if( data_file_dlg.DoModal() != IDOK )
        return;

    AddConnectionStrings(data_file_dlg.GetConnectionStrings());

    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnOutputOpen()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false, m_outputConnectionString);

    if( m_pff.GetConcatenateMethod() == ConcatenateMethod::Case )
        data_file_dlg.SetDictionaryFilename(WS2CS(m_dictionaryFilename));

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_outputConnectionString = data_file_dlg.GetConnectionString();

    UpdateData(FALSE);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnRemove()
{
    if( m_fileList.GetItemCount() == 0 )
    {
        AfxMessageBox(_T("No files to remove."));
        return;
    }

    int iFirstSel = m_fileList.GetSelectionMark();
    POSITION pos = m_fileList.GetFirstSelectedItemPosition();
    CArray<int,int> adellist;
    if (pos == NULL)
    {
       AfxMessageBox(_T("No files selected."));
    }
    else
    {
       while (pos)
       {
          int nItem = m_fileList.GetNextSelectedItem(pos);
          adellist.Add(nItem);
//        m_fileList.DeleteItem(nItem-count);
//        count++;
          // you could do your own processing on nItem here
       }
    }
    for (int i = adellist.GetSize()-1; i >= 0 ; i--)
        m_fileList.DeleteItem(adellist[i]);

    if (iFirstSel >= m_fileList.GetItemCount()) {
        iFirstSel = m_fileList.GetItemCount() - 1;
    }
    m_fileList.SetItemState(iFirstSel, LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED );
    m_fileList.SetFocus();
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnClear()
{
    if( m_fileList.GetItemCount() == 0 )
        return;

    const std::wstring prompt = FormatTextCS2WS(_T("Are you sure that you want to clear %d file%s?"),
                                                m_fileList.GetItemCount(), PluralizeWord(m_fileList.GetItemCount()));

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return;

    m_fileList.DeleteAllItems();
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnBeginDragFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
    //This routine sets the parameters for a Drag and Drop operation.
    //It sets some variables to track the Drag/Drop as well
    // as creating the drag image to be shown during the drag.

    NM_LISTVIEW* pNMListView = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);

    //// Save the index of the item being dragged in m_nDragIndex
    ////  This will be used later for retrieving the info dragged
    m_nDragIndex = pNMListView->iItem;

    //// Create a drag image
    POINT pt;
    int nOffset = -8; //offset in pixels for drag image (positive is up and to the left; neg is down and to the right)
    if(m_fileList.GetSelectedCount() > 1) //more than one item is selected
    pt.x = nOffset;
    pt.y = nOffset;

    m_pDragImage = m_fileList.CreateDragImage(m_nDragIndex, &pt);
    ASSERT(m_pDragImage); //make sure it was created
    //We will call delete later (in LButtonUp) to clean this up

    if(m_fileList.GetSelectedCount() > 1) //more than 1 item in list is selected
    {
    //  CBitmap bitmap;
    //  bitmap.LoadBitmap(IDB_BITMAP_MULTI);
    //  m_pDragImage->Replace(0, &bitmap, &bitmap);
    }

    //// Change the cursor to the drag image
    ////    (still must perform DragMove() in OnMouseMove() to show it moving)
    m_pDragImage->BeginDrag(0, CPoint(nOffset, nOffset));
    m_pDragImage->DragEnter(GetDesktopWindow(), pNMListView->ptAction);

    //// Set dragging flag and others
    m_bDragging = TRUE; //we are in a drag and drop operation
    m_nDropIndex = -1;  //we don't have a drop index yet
    m_pDropWnd = &m_fileList;

    //// Capture all mouse messages
    SetCapture ();

    *pResult = 0;
}


void CCSConcatDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    //This routine is the end of the drag/drop operation.
    //When the button is released, we are to drop the item.
    //There are a few things we need to do to clean up and
    // finalize the drop:
    //  1) Release the mouse capture
    //  2) Set m_bDragging to false to signify we are not dragging
    //  3) Actually drop the item (we call a separate function to do that)

    //If we are in a drag and drop operation (otherwise we don't do anything)
    if (m_bDragging)
    {
        // Release mouse capture, so that other controls can get control/messages
        ReleaseCapture ();

        // Note that we are NOT in a drag operation
        m_bDragging = FALSE;

        // End dragging image
        m_pDragImage->DragLeave (GetDesktopWindow ());
        m_pDragImage->EndDrag ();
        delete m_pDragImage; //must delete it because it was created at the beginning of the drag

        CPoint pt (point); //Get current mouse coordinates
        ClientToScreen (&pt); //Convert to screen coordinates
        // Get the CWnd pointer of the window that is under the mouse cursor
        CWnd* pDropWnd = WindowFromPoint (pt);
        ASSERT (pDropWnd); //make sure we have a window pointer
        // If window is CListCtrl, we perform the drop
        if (pDropWnd->IsKindOf (RUNTIME_CLASS (CListCtrl)))
        {
//          m_pDropList = (CListCtrl*)pDropWnd; //Set pointer to the list we are dropping on
            DropItemOnList((CListCtrl*)pDropWnd, (CListCtrl*)pDropWnd); //Call routine to perform the actual drop
        }
    }

    CDialog::OnLButtonUp(nFlags, point);
}


void CCSConcatDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    //While the mouse is moving, this routine is called.
    //This routine will redraw the drag image at the present
    // mouse location to display the dragging.
    //Also, while over a CListCtrl, this routine will highlight
    // the item we are hovering over.

    //// If we are in a drag/drop procedure (m_bDragging is true)
    if (m_bDragging)
    {
        //// Move the drag image
        CPoint pt(point);   //get our current mouse coordinates
        ClientToScreen(&pt); //convert to screen coordinates
        m_pDragImage->DragMove(pt); //move the drag image to those coordinates
        // Unlock window updates (this allows the dragging image to be shown smoothly)
        m_pDragImage->DragShowNolock(false);

        //// Get the CWnd pointer of the window that is under the mouse cursor
        CWnd* pDropWnd = WindowFromPoint (pt);
        ASSERT(pDropWnd); //make sure we have a window

        //// If we drag outside current window we need to adjust the highlights displayed
        if (pDropWnd != m_pDropWnd)
        {
            if (m_nDropIndex != -1) //If we drag over the CListCtrl header, turn off the hover highlight
            {
                TRACE(_T("m_nDropIndex is -1\n"));
                CListCtrl* pList = (CListCtrl*)m_pDropWnd;
                VERIFY (pList->SetItemState (m_nDropIndex, 0, LVIS_DROPHILITED));
                // redraw item
                VERIFY (pList->RedrawItems (m_nDropIndex, m_nDropIndex));
                pList->UpdateWindow ();
                m_nDropIndex = -1;
            }
            else //If we drag out of the CListCtrl altogether
            {
                TRACE(_T("m_nDropIndex is not -1\n"));
                CListCtrl* pList = (CListCtrl*)m_pDropWnd;
                int i = 0;
                int nCount = pList->GetItemCount();
                for(i = 0; i < nCount; i++)
                {
                    pList->SetItemState(i, 0, LVIS_DROPHILITED);
                }
                pList->RedrawItems(0, nCount);
                pList->UpdateWindow();
            }
        }

        // Save current window pointer as the CListCtrl we are dropping onto
        m_pDropWnd = pDropWnd;

        // Convert from screen coordinates to drop target client coordinates
        pDropWnd->ScreenToClient(&pt);

        //If we are hovering over a CListCtrl we need to adjust the highlights
        if(pDropWnd->IsKindOf(RUNTIME_CLASS (CListCtrl)))
        {
            //Note that we can drop here
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            UINT uFlags;
            CListCtrl* pList = (CListCtrl*)pDropWnd;

            // Turn off hilight for previous drop target
            pList->SetItemState (m_nDropIndex, 0, LVIS_DROPHILITED);
            // Redraw previous item
            pList->RedrawItems (m_nDropIndex, m_nDropIndex);

            // Get the item that is below cursor
            m_nDropIndex = ((CListCtrl*)pDropWnd)->HitTest(pt, &uFlags);
            // Highlight it
            pList->SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
            // Redraw item
            pList->RedrawItems(m_nDropIndex, m_nDropIndex);
            pList->UpdateWindow();
        }
        else
        {
            //If we are not hovering over a CListCtrl, change the cursor
            // to note that we cannot drop here
            SetCursor(LoadCursor(NULL, IDC_NO));
        }
        // Lock window updates
        m_pDragImage->DragShowNolock(true);
    //  return;
    }

    CDialog::OnMouseMove(nFlags, point);
}


void CCSConcatDlg::DropItemOnList(CListCtrl* pDragList, CListCtrl* pDropList)
{
    //This routine performs the actual drop of the item dragged.
    //It simply grabs the info from the Drag list (pDragList)
    // and puts that info into the list dropped on (pDropList).
    //Send: pDragList = pointer to CListCtrl we dragged from,
    //      pDropList = pointer to CListCtrl we are dropping on.
    //Return: nothing.

    // Unhilight the drop target
    pDropList->SetItemState (m_nDropIndex, 0, LVIS_DROPHILITED);

    //Set up the LV_ITEM for retrieving item from pDragList and adding the new item to the pDropList

    if(pDragList->GetSelectedCount() == 1)
    {
        // Get item that was dragged
        CString csName = pDragList->GetItemText(m_nDragIndex,0);
        CString csPath = pDragList->GetItemText(m_nDragIndex,1);
        CString csRepoType = pDragList->GetItemText(m_nDragIndex,2);
        CString csDate = pDragList->GetItemText(m_nDragIndex,3);
        CString csSize = pDragList->GetItemText(m_nDragIndex,4);

        // Delete the original item (for Move operation)
        // This is optional. If you want to implement a Copy operation, don't delete.
        // This works very well though for re-arranging items within a CListCtrl.
        // It is written at present such that when dragging from one list to the other
        //  the item is copied, but if dragging within one list, the item is moved.
        if(pDragList == pDropList)
        {
            static_cast<CSortListCtrl*>(pDragList)->DeleteItem (m_nDragIndex);
            //static_cast<CSortListCtrl*>(pDragList)->FreeItemMemory(m_nDragIndex);
            if(m_nDragIndex < m_nDropIndex) m_nDropIndex--; //decrement drop index to account for item
                                                            //being deleted above it
        }

        // Insert item into pDropList
        // if m_nDropIndex == -1, iItem = GetItemCount() (inserts at end of list), else iItem = m_nDropIndex
//      lvi->iItem = (m_nDropIndex == -1) ? pDropList->GetItemCount () : m_nDropIndex;
        int pos = (m_nDropIndex == -1) ? pDropList->GetItemCount () : m_nDropIndex;
        static_cast<CSortListCtrl*>(pDropList)->SInsertItem(pos, csName.GetString(), csPath.GetString(), csRepoType.GetString(), csDate.GetString(), csSize.GetString());
//      pDragList->SetItemText(pos,1,csPath);
//      pDragList->SetItemText(pos,2,csDate);
//      pDragList->SetItemText(pos,3,csSize);

        // Select the new item we just inserted
        pDropList->SetItemState (pos, LVIS_SELECTED, LVIS_SELECTED);
//      delete lvi;
    }
    else //more than 1 item is being dropped
    {
        if(pDragList == pDropList) //We are reordering the list (moving)
        {
            //We have to parse through all of the selected items from the DragList
            //1) Retrieve the info for the items and store them in memory
            //      Maintain a linked list (CList) of the pointers to the info in memory
            //2) Delete the items from the List
            //      If the index of the item deleted is < the m_nDropIndex, decrement m_nDropIndex
            //4) Once all the selected items are deleted and put in the linked list, insert them back in at m_nDropIndex

            //Stuff for storing the Items in memory
//          CList<LVITEM*, LVITEM*> listItems; //declare a linked list of LV_ITEMS
            CStringArray csaName;
            CStringArray csaPath;
            CStringArray csaRepoType;
            CStringArray csaDate;
            CStringArray csaSize;
            // Get item that was dragged


            //Retrieve the selected items, store them in memory and delete them from the CListCtrl
            POSITION pos = pDragList->GetFirstSelectedItemPosition(); //iterator for the CListCtrl
            while(pos) //so long as we have a valid POSITION, we keep iterating
            {
                //In this loop, we simply retrieve info on selected items,
                // and put them in memory.
                // Subsequent loops will delete the items and add them back into the list at the new positions

                m_nDragIndex = pDragList->GetNextSelectedItem(pos);

                csaName.Add(pDragList->GetItemText(m_nDragIndex,0));
                csaPath.Add(pDragList->GetItemText(m_nDragIndex,1));
                csaRepoType.Add(pDragList->GetItemText(m_nDragIndex,2));
                csaDate.Add(pDragList->GetItemText(m_nDragIndex,3));
                csaSize.Add(pDragList->GetItemText(m_nDragIndex,4));

                //Save the pointer to the new item in our CList
            } //EO while(pos) -- at this point we have deleted the moving items and stored them in memory


            //Delete the selected items
            pos = pDragList->GetFirstSelectedItemPosition();
            while(pos)
            {
                pos = pDragList->GetFirstSelectedItemPosition();
                m_nDragIndex = pDragList->GetNextSelectedItem(pos);

                static_cast<CSortListCtrl*>(pDragList)->DeleteItem(m_nDragIndex); //since we are MOVING, delete the item
                if(m_nDragIndex < m_nDropIndex) m_nDropIndex--; //must decrement the drop index to account
                                                                //for the deleted items
            } //EO while(pos)


            //Iterate through the items stored in memory and add them back into the CListCtrl at the drop index
            //listPos = listItems.GetHeadPosition();
            for (int i = 0; i < csaName.GetSize();i++)
            {
                //pItem = listItems.GetNext(listPos); //retrieve subsequent items

                int pos1 = (m_nDropIndex == -1) ? pDropList->GetItemCount() : m_nDropIndex;
                //pItem->iItem = m_nDropIndex;
                static_cast<CSortListCtrl*>(pDropList)->SInsertItem(pos1, csaName[i].GetString(), csaPath[i].GetString(), csaRepoType[i].GetString(), csaDate[i].GetString(), csaSize[i].GetString());
                //pDropList->InsertItem(pItem); //add the item

                pDropList->SetItemState(pos1, LVIS_SELECTED, LVIS_SELECTED); //highlight/select the item we just added

                m_nDropIndex++; //increment the index we are dropping at to keep the dropped items in the same order they were in in the Drag List
                //If we dont' increment this, the items are added in reverse order
            } //EO while(listPos)


        } //EO if(pDragList == pDropList)

    }
}


void CCSConcatDlg::OnEndDragFileList(NMHDR*, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    *pResult = 0;
}


void CCSConcatDlg::OnDropFiles(const std::vector<std::wstring>& filenames)
{
    std::vector<ConnectionString> connection_strings;

    std::transform(filenames.cbegin(), filenames.cend(),
                   std::back_inserter(connection_strings), [](const std::wstring& filename) { return ConnectionString(filename); });

    AddConnectionStrings(connection_strings);

    m_fileList.EnsureVisible(m_fileList.GetItemCount() - 1, FALSE);

    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


LRESULT CCSConcatDlg::OnUpdateDialogUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    bool enable_run = ( m_fileList.GetItemCount() > 0 && m_outputConnectionString.IsDefined() );
    const bool case_concat = ( m_pff.GetConcatenateMethod() == ConcatenateMethod::Case );

    if( case_concat )
        enable_run &= !SO::IsBlank(m_dictionaryFilename);

    GetDlgItem(IDOK)->EnableWindow(enable_run);
    m_menu.EnableMenuItem(ID_FILE_RUN, enable_run ? MF_ENABLED : MF_DISABLED);

    GetDlgItem(IDC_DICTIONARY)->EnableWindow(case_concat);
    GetDlgItem(IDC_DICT_BROWSE)->EnableWindow(case_concat);

    static_cast<CButton*>(GetDlgItem(IDC_CONCAT_METHOD_CASE))->SetCheck(case_concat ? BST_CHECKED : BST_UNCHECKED);
    static_cast<CButton*>(GetDlgItem(IDC_CONCAT_METHOD_FILE))->SetCheck(case_concat ? BST_UNCHECKED : BST_CHECKED);

    // update the number of files
    WindowsWS::SetDlgItemText(this, IDC_NUMFILES,
                              FormatTextCS2WS(_T("%d file%s"), m_fileList.GetItemCount(), PluralizeWord(m_fileList.GetItemCount())));

    return 0;
}


void CCSConcatDlg::OnChangeOutput()
{
    UpdateData(TRUE);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnBnClickedDictBrowse()
{
    UpdateData(TRUE);

    CIMSAFileDialog dlg(FALSE, NULL, m_dictionaryFilename.c_str(), OFN_HIDEREADONLY, _T("Data Dictionary Files (*.dcf)|*.dcf||"));
    dlg.m_ofn.lpstrTitle = _T("Choose Data Dictionary");

    if( dlg.DoModal() != IDOK )
        return;

    m_dictionaryFilename = dlg.GetPathName();

    UpdateData(FALSE);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnEnChangeDictionary()
{
    UpdateData(TRUE);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnBnClickedConcatMethodCase()
{
    m_pff.SetConcatenateMethod(ConcatenateMethod::Case);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnBnClickedConcatMethodFile()
{
    m_pff.SetConcatenateMethod(ConcatenateMethod::Text);
    PostMessage(UWM::CSConcat::UpdateDialogUI);
}


void CCSConcatDlg::OnOK()
{
    if( !UIToPff(true) )
        return;

    // if the listing file hasn't been defined, put it in the same folder as the PFF, or in the temporary folder if the PFF hasn't been saved
    if( m_pff.GetListingFName().IsEmpty() )
    {
        m_pff.SetListingFName(WS2CS(m_pff.GetPifFileName().IsEmpty() ? PortableFunctions::PathAppendToPath(GetTempDirectory(), _T("CSConcat.lst")) :
                                                                       PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::Listing)));
    }
    
    try
    {
        const Concatenator::RunSuccess run_success = Concatenator().Run(m_pff, false);

        if( run_success == Concatenator::RunSuccess::Success )
        {
            AfxMessageBox(_T("Concatenate completed."));
        }

        else if( run_success == Concatenator::RunSuccess::SuccessWithErrors )
        {
            AfxMessageBox(_T("Concatenate completed with errors."));
        }

        else if( run_success == Concatenator::RunSuccess::Errors )
        {
            AfxMessageBox(_T("Concatenate failed."));
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CCSConcatDlg::RunBatch(const std::wstring& pff_filename)
{
    try
    {
        m_pff.SetPifFileName(WS2CS(pff_filename));

        if( !m_pff.LoadPifFile(true) || m_pff.GetAppType() != CONCAT_TYPE )
        {
            throw CSProException(_T("PFF file %s was not read correctly. Check the file for parameters invalid to CSConcat."),
                                 m_pff.GetPifFileName().GetString());
        }

        Concatenator().Run(m_pff, true);

        m_pff.ExecuteOnExitPff();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
