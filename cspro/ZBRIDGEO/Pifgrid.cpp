/***************************************************
****************************************************
Skeleton Class for a Derived CPifGrid v3.5
****************************************************
****************************************************/
#include "StdAfx.h"
#include "Pifgrid.h"
#include "DataFileDlg.h"
#include "DatFDlg.h"
#include "PifDlg.h"


const int GRID_DEFAULT_COLWIDTH = 75;


namespace
{
    const TCHAR* const DataRepositoryMultipleFiles = _T("Multiple Files");

    void TrimRelativePathIfInSameDirectory(CString& text)
    {
        if( text.GetLength() >= 2 && text.GetAt(0) == '.' && text.GetAt(1) == '\\' )
            text = text.Mid(2);
    }

    CString GetConnectionStringText(const std::vector<ConnectionString>& connection_strings, CString sPifFileName)
    {
        CString text;

        // don't show any text for repositories that do not use filenames
        if( connection_strings.size() != 1 || !DataRepositoryHelpers::TypeDoesNotUseFilename(connection_strings.front().GetType()) )
        {
            text = PathHelpers::CreateSingleStringFromConnectionStrings(connection_strings, false, sPifFileName);
            TrimRelativePathIfInSameDirectory(text);
        }

        return text;
    }

    CString GetFilenameText(CString filename, CString sPifFileName)
    {
        filename = GetRelativeFName<CString>(sPifFileName, filename);
        TrimRelativePathIfInSameDirectory(filename);
        return filename;
    }
}

/***************************************************
****************************************************/
CPifGrid::CPifGrid()
{
}

/***************************************************
****************************************************/
CPifGrid::~CPifGrid()
{
}

void CPifGrid::SetGridData(const CArray<PIFINFO*, PIFINFO*>& gridData, int windowWidth)
{
    int descriptionColumnWidth = windowWidth / 5; // GHM 20120510 changed from 4 to 5 because of enlarging the window
    CClientDC dc(this);
    CFont* pOldFont = dc.SelectObject(&m_font);
    int iCount = gridData.GetSize();
    int iCurRow = 0;
    LONG options_column_width = 0;

    std::set<const TCHAR*> options_cell_strings;

    for( int iIndex = 0; iIndex < iCount; iIndex++ )
    {
        PIFINFO* pifInfo = gridData[iIndex];

        QuickSetText(-1, iCurRow, pifInfo->sDisplay);

        // 20120510 make sure that the column is large enough to display the dictionary name (the + 25 is for some padding)
        descriptionColumnWidth = std::max(descriptionColumnWidth, (int) dc.GetTextExtent(pifInfo->sDisplay).cx + 25);

        QuickSetAlignment(-1, iCurRow, UG_ALIGNLEFT);

        CUGCell cell;
        GetCell(-1, iCurRow, &cell);

        void** pPifOb = (void**) cell.AllocExtraMem(sizeof(LPVOID));
        *pPifOb = pifInfo;

        SetCell(-1, iCurRow, &cell);

        QuickSetCellType(0, iCurRow, m_iEllipsisIndex);


        auto build_options_cell = [&](const std::vector<const TCHAR*>& options)
        {
            // build newline-separated list of options
            CString options_text;

            for( const auto& option : options )
            {
                options_text.AppendFormat(_T("%s\n"), option);

                options_column_width = std::max(options_column_width, dc.GetTextExtent(option).cx);
            }

            GetCell(1, iCurRow, &cell);
            cell.SetCellType(UGCT_DROPLIST);
            cell.SetReadOnly(FALSE);
            cell.SetLabelText(options_text);
            SetCell(1, iCurRow, &cell);
        };


        // set up filtered extension processors
        std::shared_ptr<FilteredExtensionProcessor> filtered_extension_processor;

        if( pifInfo->sUName.CompareNoCase(LISTFILE) == 0 )
            filtered_extension_processor = CreateListingFilteredExtensionProcessor();

        else if( pifInfo->sUName.CompareNoCase(FREQFILE) == 0 || pifInfo->sUName.CompareNoCase(IMPUTEFILE) == 0 )
            filtered_extension_processor = CreateFrequencyFilteredExtensionProcessor();


        // files with a filtered extension processor
        if( filtered_extension_processor != nullptr )
        {
            m_filteredExtensionProcessors[iCurRow] = filtered_extension_processor;

            build_options_cell(filtered_extension_processor->GetTypes());

            UpdateFilteredExtensionRow(iCurRow, pifInfo->sFileName);
        }

        // non-data files
        else if( pifInfo->dictionary_filename.IsEmpty() )
        {
            CString sString;

            if( pifInfo->sFileName.Find(_T("\" ")) > 0 )
            {
                sString = pifInfo->sFileName;
                m_arrMultFiles.RemoveAll();
                for( const std::wstring& filename : SO::SplitString(sString, '"') ) {
                    if (!filename.empty())
                        m_arrMultFiles.Add(WS2CS(filename));
                }
            }

            else
            {
                sString = GetFilenameText(pifInfo->sFileName, m_sPifFileName);
            }

            JoinCells(0, iCurRow, 1, iCurRow);

            QuickSetText(0, iCurRow, sString);
        }

        // data files
        else
        {
            bool add_only_readable_types = ( ( pifInfo->uOptions & ( PIF_FILE_MUST_EXIST | PIF_REPOSITORY_MUST_BE_READABLE ) ) != 0 );
            const DataFileFilterManager& data_file_filter_manager = DataFileFilterManager::Get(DataFileFilterManager::UseType::FileAssociationsDlg, add_only_readable_types);

            m_dataFileFilterManagers[iCurRow] = &data_file_filter_manager;

            std::vector<const TCHAR*> data_repository_types = data_file_filter_manager.GetTypeNames();

            if( ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) != 0 )
                data_repository_types.emplace_back(DataRepositoryMultipleFiles);

            build_options_cell(data_repository_types);

            UpdateDataRepositoryRow(iCurRow, pifInfo->connection_strings);
        }


        QuickSetAlignment(0, iCurRow, UG_ALIGNLEFT);

        iCurRow++;
    }

    SetColWidth(-1, descriptionColumnWidth);

    // adjust the options cell width so that it isn't too tight
    options_column_width += GetSystemMetrics(SM_CXVSCROLL) + dc.GetTextExtent(_T("XX")).cx; // so it isn't too tight

    SetColWidth(0, windowWidth - descriptionColumnWidth - options_column_width - GetSystemMetrics(SM_CXVSCROLL));
    SetColWidth(1, options_column_width);
    dc.SelectObject(pOldFont);
}


PIFINFO* CPifGrid::GetRowInfo(long row)
{
    CUGCell cell;
    GetCell(-1, row, &cell);
    PIFINFO** ppPifInfo = (PIFINFO**)cell.GetExtraMemPtr();
    return ppPifInfo != nullptr ? *ppPifInfo : nullptr;
}


void CPifGrid::GetDialogTitleAndFilter(PIFINFO* pPifInfo, CString& sTitle, CString& sFilter)
{
    ASSERT(pPifInfo->dictionary_filename.IsEmpty());

    if (pPifInfo->sUName.CompareNoCase(PARADATAFILE) == 0) {
        sFilter = _T("Paradata Log (*.cslog)|*.cslog||");
        sTitle = _T("Select Paradata Log");
    }
    else if (pPifInfo->sUName.CompareNoCase(LISTFILE) == 0) {
        sFilter = FileFilters::Listing;
        sTitle = _T("Select Listing File");
    }
    else if (pPifInfo->sUName.CompareNoCase(WRITEFILE) == 0) {
        sFilter = _T("All Files (*.*)|*.*||");
        sTitle = _T("Select Write File");
    }
    else if (pPifInfo->sUName.CompareNoCase(FREQFILE) == 0) {
        sFilter = _T("All Files (*.*)|*.*||");
        sTitle = _T("Select Freq File");
    }
    else if (pPifInfo->sUName.CompareNoCase(IMPUTEFILE) == 0) {
        sFilter = _T("All Files (*.*)|*.*||");
        sTitle = _T("Select Impute Freq File");
    }
    else if (pPifInfo->sUName.CompareNoCase(SAVEARRAYFILE) == 0) {
        sFilter = _T("Save Array Files (*.sva)|*.sva||");
        sTitle = _T("Select Save Array File");
    }
    else if (pPifInfo->sUName.CompareNoCase(WRITEFILE) == 0) {
        sFilter = _T("All Files (*.*)|*.*||");
        sTitle = _T("Select Write File");
    }
    else if (pPifInfo->sUName.CompareNoCase(INPUTTBD) == 0) {
        sFilter = _T("TAB Files (*.tab)|*.tab||");
        sTitle = _T("Select Input TAB File");
    }
    else if (pPifInfo->sUName.CompareNoCase(OUTPUTTBD) == 0) {
        sFilter = _T("TAB Files (*.tab)|*.tab||");
        sTitle = _T("Select Output TAB File");
    }
    else if (pPifInfo->sUName.CompareNoCase(OUTPUTTBW) == 0) {
        sFilter = _T("Select TBW File (*.tbw)|*.tbw||");
        sTitle = _T("Select Output TBW File");
    }
    else if (pPifInfo->sUName.CompareNoCase(AREANAMES) == 0) {
        sFilter = _T("CSPro Area Names Files (*.anm)|*.anm|All Files (*.*)|*.*||");
        sTitle = _T("Select Area Names File");
    }
    else {
        sFilter = _T("All Files (*.*)|*.*||");
        sTitle = _T("Select File");
    }
}

/***************************************************
OnSetup
This function is called just after the grid window
is created or attached to a dialog item.
It can be used to initially setup the grid
****************************************************/
void CPifGrid::OnSetup(){

    RECT rect = {0,0,0,0};
    m_pPifEdit.Create(WS_VISIBLE,rect,this,125);

    VScrollAlwaysPresent(TRUE);
    EnableExcelBorders(TRUE);
    SetMultiSelectMode(TRUE);

    m_iEllipsisIndex = AddCellType(&m_ellipsis);
    SetNumberCols(m_iCols);
    SetNumberRows(m_iRows);
    for (int iCol =0 ; iCol < m_iCols ; iCol++){
        SetColWidth(iCol, GRID_DEFAULT_COLWIDTH);
    }

    m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Sans Serif"));
    SetDefFont(&m_font);

    QuickSetFont(-1,-1,&m_font);
    QuickSetText(-1,-1,_T(""));
    QuickSetText(0, -1, _T("Data File Name"));
    QuickSetText(1, -1, _T("Source Type"));

    QuickSetAlignment (-1,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);
    QuickSetAlignment (0,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);

    AdjustComponentSizes();
}

/***************************************************
OnSheetSetup
****************************************************/
void CPifGrid::OnSheetSetup(int){
    SetMultiSelectMode(TRUE);
}

/***************************************************
OnCanMove
Sent when the current cell in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CPifGrid::OnCanMove(int ,long ,int ,long ){
    return TRUE;
}
/***************************************************
OnCanMove
Sent when the top row or left column in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CPifGrid::OnCanViewMove(int ,long ,int ,long ){
    return TRUE;
}
/***************************************************
****************************************************/
void CPifGrid::OnHitBottom(long ,long ,long ){
}
/***************************************************
****************************************************/
void CPifGrid::OnHitTop(long ,long ){

}
/***************************************************
OnCanSizeCol
Sent when the user is over a separation line on
the top heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int CPifGrid::OnCanSizeCol(int ){
//  if(col == -1)
//      return TRUE;
//  else
        return FALSE;
}
/***************************************************
OnColSizing
Sent when the user is sizing a column
The column that is being sized is given as
well as the width. Plus the width can be modified
at this point. This makes it easy to set min and
max widths
****************************************************/
void CPifGrid::OnColSizing(int ,int *){
}
/***************************************************
OnColSized
This is sent when the user finished sizing the
given column (see above for more details)
****************************************************/
void CPifGrid::OnColSized(int ,int *){
}
/***************************************************
OnCanSizeRow
Sent when the user is over a separation line on
the side heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int  CPifGrid::OnCanSizeRow(long ){
    return FALSE;
}
/***************************************************
OnRowSizing
Sent when the user is sizing a row
The row that is being sized is given as
well as the height. Plus the height can be modified
at this point. This makes it easy to set min and
max heights
****************************************************/
void CPifGrid::OnRowSizing(long ,int *){
}
/***************************************************
OnRowSized
This is sent when the user is finished sizing hte
given row ( see above for more details)
****************************************************/
void CPifGrid::OnRowSized(long ,int *){
}
/***************************************************
OnCanSizeSideHdg
This is sent when the user moves into position
for sizing the width of the side heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CPifGrid::OnCanSizeSideHdg(){
    return FALSE;
}
/***************************************************
OnCanSizeTopHdg
This is sent when the user moves into position
for sizing the height of the top heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CPifGrid::OnCanSizeTopHdg(){
    return FALSE;
}
/***************************************************
OnSideHdgSizing
****************************************************/
int CPifGrid::OnSideHdgSizing(int *){
    return TRUE;

}
/***************************************************
OnSideHdgSized
****************************************************/
int CPifGrid::OnSideHdgSized(int *){
    return TRUE;

}
/***************************************************
OnTopHdgSized
****************************************************/
int CPifGrid::OnTopHdgSized(int *){
    return TRUE;

}
/***************************************************
OnTopHdgSizing
****************************************************/
int CPifGrid::OnTopHdgSizing(int *){
    return TRUE;

}
/***************************************************
OnColChange
Sent whenever the current column changes
The old and the new columns are given
****************************************************/
void CPifGrid::OnColChange(int ,int ){
}
/***************************************************
OnRowChange
Sent whenever the current row changes
The old and the new rows are given
****************************************************/
void CPifGrid::OnRowChange(long ,long ){
}
/***************************************************
OnCellChange
Sent whenever the current cell changes rows or
columns
****************************************************/
void CPifGrid::OnCellChange(int ,int ,long ,long ){
}
/***************************************************
OnLeftColChange
Sent whenever the left visible column in the grid
changes
****************************************************/
void CPifGrid::OnLeftColChange(int ,int ){
}
/***************************************************
OnTopRowChange
Sent whenever the top visible row in the grid changes
****************************************************/
void CPifGrid::OnTopRowChange(long ,long ){
}
/***************************************************
OnLClicked
Sent whenever the user clicks the left mouse
button within the grid
this message is sent when the button goes down
then again when the button goes up

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnLClicked(int col,long row,int ,RECT *,POINT *,int ){
    if(col ==0 && row != -1) {
        StartEdit();
    }

}
/***************************************************
OnRClicked
Sent whenever the user clicks the right mouse
button within the grid
this message is sent when the button goes down
then again when the button goes up

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnRClicked(int ,long ,int ,RECT *,POINT *,int ){


}
/***************************************************
OnDClicked
Sent whenever the user double clicks the left mouse
button within the grid

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
****************************************************/
void CPifGrid::OnDClicked(int ,long ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnMouseMove
****************************************************/
void CPifGrid::OnMouseMove(int ,long ,POINT *,UINT ,BOOL ){
}
/***************************************************
OnTH_LClicked
Sent whenever the user clicks the left mouse
button within the top heading
this message is sent when the button goes down
then again when the button goes up

  'col' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnTH_LClicked(int ,long ,int ,RECT *,POINT *,BOOL ){

}
/***************************************************
OnTH_RClicked
Sent whenever the user clicks the right mouse
button within the top heading
this message is sent when the button goes down
then again when the button goes up

  'col' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnTH_RClicked(int ,long ,int ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnTH_LClicked
Sent whenever the user double clicks the left mouse
button within the top heading

  'col' is negative if the area clicked in is not valid
****************************************************/
void CPifGrid::OnTH_DClicked(int ,long ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnSH_LClicked
Sent whenever the user clicks the left mouse
button within the side heading
this message is sent when the button goes down
then again when the button goes up

  'row' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnSH_LClicked(int ,long ,int ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnSH_RClicked
Sent whenever the user clicks the right mouse
button within the side heading
this message is sent when the button goes down
then again when the button goes up

  'row' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnSH_RClicked(int ,long ,int ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnSH_DClicked
Sent whenever the user double clicks the left mouse
button within the side heading

  'row' is negative if the area clicked in is not valid
****************************************************/
void CPifGrid::OnSH_DClicked(int ,long ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnCB_LClicked
Sent whenever the user clicks the left mouse
button within the top corner button
this message is sent when the button goes down
then again when the button goes up

  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnCB_LClicked(int ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnCB_RClicked
Sent whenever the user clicks the right mouse
button within the top corner button
this message is sent when the button goes down
then again when the button goes up

  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CPifGrid::OnCB_RClicked(int ,RECT *,POINT *,BOOL ){
}
/***************************************************
OnCB_DClicked
Sent whenever the user double clicks the left mouse
button within the top corner button
****************************************************/
void CPifGrid::OnCB_DClicked(RECT *,POINT *,BOOL ){
}
/***************************************************
OnKeyDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_KEYDOWN for more information)
****************************************************/
void CPifGrid::OnKeyDown(UINT *,BOOL ){
}
/***************************************************
OnCharDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_CHAR for more information)
****************************************************/
void CPifGrid::OnCharDown(UINT *vcKey,BOOL processed){
    if(!processed)
        StartEdit(*vcKey);
}

/***************************************************
OnGetCell
This message is sent everytime the grid needs to
draw a cell in the grid. At this point the cell
class has been filled with the information to be
used to draw the cell. The information can now be
changed before it is used for drawing
****************************************************/
void CPifGrid::OnGetCell(int ,long ,CUGCell *){

}
/***************************************************
OnSetCell
This message is sent everytime the a cell is about
to change.
****************************************************/
void CPifGrid::OnSetCell(int ,long ,CUGCell *){
}
/***************************************************
OnDataSourceNotify
This message is sent from a data source , message
depends on the data source - check the information
on the data source(s) being used
- The ID of the Data source is also returned
****************************************************/
void CPifGrid::OnDataSourceNotify(int ,long ,long ){
}
/***************************************************
OnCellTypeNotify
This message is sent from a cell type , message
depends on the cell type - check the information
on the cell type classes
- The ID of the cell type is given
****************************************************/
int CPifGrid::OnCellTypeNotify(long,int col,long row,long msg,long param)
{
    PIFINFO* pPifInfo = GetRowInfo(row);

    if( pPifInfo == nullptr )
        return FALSE;

    if( col == 0 && msg == UGCT_ELLIPSISBUTTONCLICK )
        return ChooseFile(pPifInfo, col, row) ? TRUE : FALSE;

    else if( col == 1 && msg == UGCT_DROPLISTSELECT )
    {
        CString selected_text = *((CString*)param);

        const auto& filtered_extension_processor_lookup = m_filteredExtensionProcessors.find(row);

        // files with a filtered extension processor
        if( filtered_extension_processor_lookup != m_filteredExtensionProcessors.cend() )
        {
            pPifInfo->sFileName = filtered_extension_processor_lookup->second->GetFilenameFromType(selected_text, pPifInfo->sFileName);
            return UpdateFilteredExtensionRow(row, pPifInfo->sFileName);
        }

        // data files
        else
        {
            // change the file extension based on the repository type
            ASSERT(m_dataFileFilterManagers.find(row) != m_dataFileFilterManagers.cend());
            const auto& data_file_filter_manager = m_dataFileFilterManagers[row];
            const DataFileFilter* data_file_filter = data_file_filter_manager->GetDataFileFilterFromTypeName(CS2WS(selected_text));

            // don't allow the selection of DataRepositoryMultipleFiles
            if( data_file_filter == nullptr )
                return FALSE;

            if( data_file_filter->type == DataRepositoryType::Null )
            {
                pPifInfo->connection_strings = { ConnectionString::CreateNullRepositoryConnectionString() };
            }

            else if( data_file_filter->type == DataRepositoryType::Memory )
            {
                pPifInfo->connection_strings = { ConnectionString::CreateMemoryRepositoryConnectionString() };
            }

            else
            {
                for( auto connection_string = pPifInfo->connection_strings.begin(); connection_string != pPifInfo->connection_strings.end(); )
                {
                    if( connection_string->IsFilenamePresent() )
                    {
                        data_file_filter_manager->AdjustConnectionStringFromDataFileFilter(*connection_string, *data_file_filter);
                        ++connection_string;
                    }

                    // remove repositories without filenames
                    else
                    {
                        connection_string = pPifInfo->connection_strings.erase(connection_string);
                    }
                }
            }

            return UpdateDataRepositoryRow(row, pPifInfo->connection_strings, data_file_filter->type);
        }
    }

    return FALSE;
}

/***************************************************
OnEditStart
This message is sent whenever the grid is ready
to start editing a cell
A return of TRUE allows the editing a return of
FALSE stops editing
Plus the properties of the CEdit class can be modified
****************************************************/
int CPifGrid::OnEditStart(int col, long row,CWnd **edit){
    if(col == 0) {
        CUGCell cell;
        GetCell(col,row,&cell);
        if(cell.GetBackColor() ==RGB(192,192,192) )
            return FALSE;

        *edit =&m_pPifEdit;
        return TRUE;
    }
    else {
        return FALSE;
    }
}
/***************************************************
OnEditVerify
This is send when the editing is about to end
****************************************************/
int CPifGrid::OnEditVerify(int , long ,CWnd *,UINT *){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CPifGrid::OnEditFinish(int col, long row,CWnd *,LPCTSTR string, BOOL)
{
    CIMSAString sBeforeChange = QuickGetText(col,row);

    if( sBeforeChange.CompareNoCase(string) != 0 )
    {
        QuickSetText(col, row, string);
        m_arrMultFiles.RemoveAll();

        PIFINFO* pPifInfo = GetRowInfo(row);

        if( pPifInfo != nullptr )
        {
            // non-data files
            if( pPifInfo->dictionary_filename.IsEmpty() )
            {
                pPifInfo->sFileName = WS2CS(MakeFullPath(GetWorkingFolder(m_sPifFileName), string));

                // files with a filtered extension processor
                const auto& filtered_extension_processor_lookup = m_filteredExtensionProcessors.find(row);

                if( filtered_extension_processor_lookup != m_filteredExtensionProcessors.cend() )
                    UpdateFilteredExtensionRow(row, pPifInfo->sFileName);
            }

            // data files
            else
            {
                pPifInfo->connection_strings = PathHelpers::SplitSingleStringIntoConnectionStrings(string);

                const DataFileFilter* data_file_filter = GetSelectedDataFileFilter(row);

                for( ConnectionString& connection_string : pPifInfo->connection_strings )
                {
                    connection_string.AdjustRelativePath(GetWorkingFolder(m_sPifFileName));

                    // add the file extension of the selected type if they didn't add an extension
                    // and the data file doesn't already exist
                    if( data_file_filter != nullptr && data_file_filter->type != connection_string.GetType() &&
                        data_file_filter->force_extension && connection_string.IsFilenamePresent() &&
                        !PathHasWildcardCharacters(connection_string.GetFilename()) &&
                        !PortableFunctions::FileExists(connection_string.GetFilename()) &&
                        PortableFunctions::PathGetFileExtension(connection_string.GetFilename()).empty() )
                    {
                        ASSERT(m_dataFileFilterManagers.find(row) != m_dataFileFilterManagers.cend());
                        const auto& data_file_filter_manager = m_dataFileFilterManagers[row];

                        data_file_filter_manager->AdjustConnectionStringFromDataFileFilter(connection_string, *data_file_filter);
                    }
                }

                UpdateDataRepositoryRow(row, pPifInfo->connection_strings);

                // the filename will be updated correctly at this point but will sometimes
                // not display properly but this seems to be a redraw bug in the grid control
            }
        }
    }

    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CPifGrid::OnEditContinue(int ,long ,int* ,long* ){
    return TRUE;
}
/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CPifGrid::OnMenuCommand(int ,long ,int ,int ){
}
/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CPifGrid::OnMenuStart(int ,long ,int ){
    return TRUE;
}
/***************************************************
OnHint
****************************************************/
int CPifGrid::OnHint(int col,long row,int ,CString *string){
    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}
/***************************************************
OnVScrollHint
****************************************************/
int CPifGrid::OnVScrollHint(long ,CString *){
    return TRUE;
}
/***************************************************
OnHScrollHint
****************************************************/
int CPifGrid::OnHScrollHint(int ,CString *){
    return TRUE;
}


void CPifGrid::OnScreenDCSetup(CDC *,int ){
}
/***************************************************
OnSortEvaluate
return      -1  <
0   ==
1   >
****************************************************/

/***************************************************
OnAdjustComponentSizes
****************************************************/
void CPifGrid::OnAdjustComponentSizes(RECT *,RECT *,RECT *,
                                      RECT *,RECT *,RECT *,RECT *){
}

/***************************************************
OnDrawFocusRect
****************************************************/
void CPifGrid::OnDrawFocusRect(CDC *dc,RECT *rect){

    //  DrawExcelFocusRect(dc,rect);

    //  rect->bottom --;
    //  rect->right --;
    //  dc->DrawFocusRect(rect);

    rect->bottom --;
    dc->DrawFocusRect(rect);
    rect->left++;
    rect->top++;
    rect->right--;
    rect->bottom--;
    dc->DrawFocusRect(rect);

}


/***************************************************
OnSetFocus
****************************************************/
void CPifGrid::OnSetFocus(int ){
}

/***************************************************
OnKillFocus
****************************************************/
void CPifGrid::OnKillFocus(int ){
}
/***************************************************
OnColSwapStart
****************************************************/
BOOL CPifGrid::OnColSwapStart(int ){

    return TRUE;
}

/***************************************************
OnCanColSwap
****************************************************/
BOOL CPifGrid::OnCanColSwap(int ,int ){

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// reset the grid
void CPifGrid::ResetGrid() {

    int iNumberofCols = GetNumberCols() ;
    SetRedraw(FALSE);

    for(int iIndex = 0; iIndex < iNumberofCols ; iIndex++)
    {
        DeleteCol(0);
    }

    int iNumberofRows  = GetNumberRows() ;
    for( int iIndex = 0; iIndex <iNumberofRows; iIndex++)
    {
        DeleteRow(0);
    }
    SetRedraw(TRUE);
}

void CPifGrid::SetDefaultInputDataFilename(CString filename)
{
    PIFINFO* pPifInfo = GetRowInfo(0);

    if( pPifInfo != nullptr && pPifInfo->eType == PIFDICT )
    {
        if( pPifInfo->connection_strings.empty() || !pPifInfo->connection_strings.front().IsDefined() )
        {
            pPifInfo->connection_strings = { ConnectionString(filename) };
            UpdateDataRepositoryRow(0, pPifInfo->connection_strings);
        }
    }
}

bool CPifGrid::ChooseFile(PIFINFO* pPifInfo, int col, long row)
{
    // data files
    if( !pPifInfo->dictionary_filename.IsEmpty() )
    {
        DataFileDlg::Type data_file_dlg_type =
            ( ( pPifInfo->uOptions & PIF_MULTIPLE_FILES ) != 0 ) ? DataFileDlg::Type::OpenExisting :
            ( pPifInfo->sUName.CompareNoCase(OUTPFILE) == 0 )    ? DataFileDlg::Type::CreateNew :
                                                                   DataFileDlg::Type::OpenOrCreate;
        bool add_only_readable_types = ( ( pPifInfo->uOptions & ( PIF_FILE_MUST_EXIST | PIF_REPOSITORY_MUST_BE_READABLE ) ) != 0 );

        DataFileDlg data_file_dlg(data_file_dlg_type, add_only_readable_types, pPifInfo->connection_strings);
        data_file_dlg.SetDictionaryFilename(pPifInfo->dictionary_filename);

        const DataFileFilter* data_file_filter = GetSelectedDataFileFilter(row);

        if( data_file_filter != nullptr && !DataRepositoryHelpers::TypeDoesNotUseFilename(data_file_filter->type) )
            data_file_dlg.SetCreateNewDefaultDataRepositoryType(data_file_filter->type);

        if( ( pPifInfo->uOptions & PIF_MULTIPLE_FILES ) != 0 )
            data_file_dlg.AllowMultipleSelections();

        if( data_file_dlg.DoModal() != IDOK )
            return false;

        pPifInfo->connection_strings = data_file_dlg.GetConnectionStrings();
        UpdateDataRepositoryRow(row, pPifInfo->connection_strings);
    }

    // non-data-files
    else
    {
        CIMSAString sDatFileName = pPifInfo->sFileName;
        if( ( pPifInfo->uOptions & PIF_MULTIPLE_FILES ) != 0 && !m_arrMultFiles.IsEmpty() )
            sDatFileName = m_arrMultFiles[0];

        //Flags
        DWORD  dwFlags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        if (pPifInfo->uOptions & PIF_MULTIPLE_FILES)
            dwFlags |= OFN_ALLOWMULTISELECT;
        if (pPifInfo->uOptions & PIF_FILE_MUST_EXIST)
            dwFlags |= OFN_FILEMUSTEXIST;

        std::vector<TCHAR> bigBuff(132000, 0);

        CString sStartDir = AfxGetApp()->GetProfileString(_T("Settings"), _T("Last DataFolder"));
        if (sDatFileName.ReverseFindOneOf(_T("\\")) == sDatFileName.GetLength() - 1) {
            //if the last character is "\" then we have a directory
		    sStartDir = sDatFileName.TrimRight(_T("\\"));
		    sDatFileName = _T("");
	    }

	    // GHM 20110322 if the user had <none> or multiple files selected ("file1" "file2" etc.)
	    // it was impossible to open the file dialog
	    bool usingProperName = sDatFileName.FindOneOf(_T("\"<")) < 0;

	    if (usingProperName)
		    _tcscpy(bigBuff.data(), sDatFileName);


	    CString sTitle, sFilter;
	    GetDialogTitleAndFilter(pPifInfo, sTitle, sFilter);
	    bool allowCSProExtensions = ( ( pPifInfo->uOptions & PIF_DISALLOW_CSPRO_EXTENSIONS ) == 0 );

	    CDatFDlg fileDlg(allowCSProExtensions, TRUE, pPifInfo->sDefaultFileExtension, usingProperName ? sDatFileName : _T(""), dwFlags, sFilter);

	    if (!sStartDir.IsEmpty()) {
		    fileDlg.m_ofn.lpstrInitialDir = sStartDir;
	    }
	    fileDlg.m_ofn.lpstrTitle = sTitle;
	    fileDlg.m_ofn.lpstrFile = bigBuff.data();
	    fileDlg.m_ofn.nMaxFile = bigBuff.size();

	    if (fileDlg.DoModal() != IDOK) {
		    return false;
	    }

	    if (pPifInfo->uOptions & PIF_MULTIPLE_FILES)
        {
		    m_arrMultFiles.RemoveAll();
		    CIMSAString sMultipleFiles = bigBuff.data();
		    POSITION pos = fileDlg.GetStartPosition();
		    while (pos) {
			    CString sFileName = fileDlg.GetNextPathName(pos);
			    m_arrMultFiles.Add(sFileName);
		    }

		    CIMSAString sText;
		    if (m_arrMultFiles.GetSize() > 1) {
			    for (int iFile = 0; iFile < m_arrMultFiles.GetSize(); iFile++) {
				    sText += _T("\"") + GetFileName(m_arrMultFiles[iFile]) + _T("\" ");
			    }
			    sText.Trim();
			    QuickSetText(col, row, sText);
		    } else {
	            pPifInfo->sFileName = bigBuff.data();
			    QuickSetText(col, row, GetFilenameText(bigBuff.data(), m_sPifFileName));
		    }
	    }

        else
        {
            pPifInfo->sFileName = bigBuff.data();

            // if using a filtered extension, update the entire row; otherwise just set the cell text
            const auto& filtered_extension_processor_lookup = m_filteredExtensionProcessors.find(row);

            if( filtered_extension_processor_lookup != m_filteredExtensionProcessors.cend() )
                UpdateFilteredExtensionRow(row, pPifInfo->sFileName);

            else
                QuickSetText(col, row, GetFilenameText(pPifInfo->sFileName, m_sPifFileName));
	    }


	    CIMSAString sFileName = fileDlg.GetPathName();
	    CString sPath(sFileName);
	    PathRemoveFileSpec(sPath.GetBuffer(_MAX_PATH));
	    sPath.ReleaseBuffer();
	    AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last DataFolder"), sPath);
    }

    return true;
}


bool CPifGrid::UpdateDataRepositoryRow(long row, const std::vector<ConnectionString>& connection_strings,
    DataRepositoryType default_data_repository_type/* = DataRepositoryType::SQLite*/)
{
    const TCHAR* repository_text = nullptr;
    bool using_repository_without_filename = false;

    if( connection_strings.size() > 1 )
    {
        repository_text = DataRepositoryMultipleFiles;
    }

    else if( !connection_strings.empty() )
    {
        repository_text = ToString(connection_strings.front().GetType());
        using_repository_without_filename = DataRepositoryHelpers::TypeDoesNotUseFilename(connection_strings.front().GetType());
    }

    // default to CSPro DB (unless the user manually modified the type)
    else
    {
        repository_text = ToString(default_data_repository_type);
    }

	CUGCell fileNameCell;
	GetCell(0, row, &fileNameCell);
    fileNameCell.SetText(GetConnectionStringText(connection_strings, m_sPifFileName));
    fileNameCell.SetBackColor(GetSysColor(using_repository_without_filename ? COLOR_BTNFACE : COLOR_WINDOW));
    SetCell(0, row, &fileNameCell);
	RedrawCell(0, row);

	CUGCell repoTypeCell;
    GetCell(1, row, &repoTypeCell);

    // check if there is a need to update the repository type
    if( CString(repoTypeCell.GetText()) == repository_text )
        return false;

    repoTypeCell.SetText(repository_text);
    SetCell(1, row, &repoTypeCell);
    RedrawCell(1, row);

    return true;
}


const DataFileFilter* CPifGrid::GetSelectedDataFileFilter(long row)
{
    ASSERT(m_dataFileFilterManagers.find(row) != m_dataFileFilterManagers.cend());

    CUGCell repoTypeCell;
    GetCell(1, row, &repoTypeCell);

    return m_dataFileFilterManagers[row]->GetDataFileFilterFromTypeName(repoTypeCell.GetText());
}


bool CPifGrid::UpdateFilteredExtensionRow(long row, const CString& filename)
{
    auto filtered_extension_processor = m_filteredExtensionProcessors[row];

	CUGCell filename_cell;
	GetCell(0, row, &filename_cell);
    filename_cell.SetText(GetFilenameText(filename, m_sPifFileName));
    SetCell(0, row, &filename_cell);
	RedrawCell(0, row);

    filtered_extension_processor->UpdateWinSettings(filename);

	CUGCell extension_cell;
    GetCell(1, row, &extension_cell);

    CString type = filtered_extension_processor->GetTypeFromFilename(filename);

    // only update the type if necessary
    if( CString(extension_cell.GetText()) == type )
        return false;

    extension_cell.SetText(type);
    SetCell(1, row, &extension_cell);
    RedrawCell(1, row);

    return true;
}
