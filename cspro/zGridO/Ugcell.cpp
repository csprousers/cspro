/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCell
    Purpose
        This class hold all the properties of a cell.
        This class is used internally and externally
        to set and retrieve cell properties.
        Although this is the native format for cell
        information datasouces may keep their own
        data in any format, as long as they convert
        the data into CUGCell format when the grid
        needs the information.
        The grid has many functions which allow cell
        properties to be set and retrieved without
        the need to use one of these object directly
        (QuickSet and QuickGet functions)
        But to have the greatest control, and performance
        the use of these objects directly is required.
    Details

        Each cell object has over 20 properties
        which can be set and retrieved. Each property
        also has two states SET and NOT-SET
        When a property is set a flag in the cell is set
        to indicate that the property is SET.
        The reason for this SET/NOT-SET design is to
        allow default styles for a cell. This way when
        the grid is gathering cell information for display
        it can  get the cells default style(s) first
        (ie. column defaults, cell styles, grid default,
        heading defaults), then retrieve the cell properties
        next, and only the SET properties are retrieved.
        This way the a cell can have default values but
        can easily over-ride these values as well.

        The cells text (or string) property is also
        its data property. This means that numbers,
        times, currency, etc. are all stored in text
        format in this property. Even though the cell
        stores all data as text, there is a flag
        which indecates how the text should be treaded
        (text, time, currency, numbers etc.).

************************************************/

#include "StdAfx.h"
#include <ctype.h>
#include "Ugctrl.h"
 //#include "UGCell.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/********************************************
Constructor
    Inits all of the cell variables (properties)
    Calls ClearAll which clears all cell properties
*********************************************/
CUGCell::CUGCell(){

    m_extraMem = NULL;
    m_pExtraMem = &m_extraMem; // GHM 20100526 optimization
    ClearAll();
}
/********************************************
Destructor
    Performs clean-up
*********************************************/
CUGCell::~CUGCell(){

    //perform clean up
    ClearAll();

}

/********************************************
ClearInfo
    Purpose
        Sets all the cell information properties to
        an "unused" state
    Params
        none
    Return
        none
*********************************************/
void CUGCell::ClearAll(){

    //clear the strings
    m_propSetFlags  = 0;
    m_string        = _T("");

    //extra memory
    if(m_extraMem != NULL){
        //////delete[] m_extraMem; GHM 2010 optimization
        m_extraMem = NULL;
        m_extraMemSize =0;
    }
}
/********************************************
SetDefaultInfo
    Purpose
        Sets up the cell with default information
        NOTE: when a cell is created there is no
        default information
    Params
        none
    Return
        UG_SUCCESS - success
        UG_ERROR - error
*********************************************/
int CUGCell::SetDefaultInfo(){

    m_propSetFlags  =   UGCELL_STRING_SET |
                        UGCELL_MASK_SET  |
                        UGCELL_LABEL_SET |
                        UGCELL_TEXTCOLOR_SET |
                        UGCELL_BACKCOLOR_SET |
                        UGCELL_HTEXTCOLOR_SET |
                        UGCELL_HBACKCOLOR_SET |
                        UGCELL_ALIGNMENT_SET;

    m_string        = _T("");
    m_mask          = _T("");
    m_label         = _T("");
    m_format        = NULL;
    m_cellStyle     = NULL;

    m_textColor     = GetSysColor(COLOR_WINDOWTEXT);
    m_backColor     = GetSysColor(COLOR_WINDOW);
    m_hTextColor    = GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_hBackColor    = GetSysColor(COLOR_HIGHLIGHT);

    m_alignment     = 0;

    return UG_SUCCESS;

}
/********************************************
CopyInfoTo
    Purpose
        Copies all the cell information to the given
        destination cell
    Params
        dest - destination cell to copy properties to
    return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::CopyInfoTo(CUGCell *dest){

    //property flags
    dest->m_propSetFlags = m_propSetFlags;

    return AddCellInfo(this,dest);
}
/********************************************
CopyInfoFrom
    Purpose
        Copies all the cell information from the given
        source cell
    Params
        src - destination cell to copy properties from
    return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::CopyInfoFrom(CUGCell *src){

    //property flags
    m_propSetFlags = src->m_propSetFlags;

    return AddCellInfo(src,this);
}
/*****************************************************
CopyCellInfo
    Purpose
        Copies all the cell information from the given
        source to the destination cell
    Params
        src  - cell to copy properties from
        dest - cell to copy properties to
    return
        UG_SUCCESS  - success
        UG_ERROR    - fail
*****************************************************/
int CUGCell::CopyCellInfo(CUGCell *src,CUGCell *dest){

    //property flags
    dest->m_propSetFlags = src->m_propSetFlags;

    return AddCellInfo(src,dest);

}

/********************************************
AddInfoTo
    Purpose
        Copies only the properties that are set
        from this cell to the destiination cell.
        NOTE: all properties have a set and unset
        state. Their state is detirmined by a bit
        int the m_propSetFlags member variable
    Params
        dest -  cell that the set properties are
                copied to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::AddInfoTo(CUGCell *dest){
    return AddCellInfo(this,dest);
}

/********************************************
AddInfoFrom
    Purpose
        Copies only the properties that are set
        in the source cell to this cell object.
        NOTE: all properties have a set and unset
        state. Their state is detirmined by a bit
        int the m_propSetFlags member variable
    Params
        src -   cell that the set properties are
                copied from
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::AddInfoFrom(CUGCell *src){
    return AddCellInfo(src,this);
}
/*****************************************************
AddCellInfo
    Purpose
        Copies only the properties that are set
        in the source cell to the dest cell object.
        NOTE: all properties have a set and unset
        state. Their state is detirmined by a bit
        int the m_propSetFlags member variable
    Params
        src -   cell that the set properties are
                copied from
        dest -  cell that the set properties are
                copied to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*****************************************************/
int CUGCell::AddCellInfo(CUGCell *src,CUGCell *dest){

    //property flags
    dest->m_propSetFlags |= src->m_propSetFlags;

    //String
    if(src->m_propSetFlags&UGCELL_STRING_SET)
        dest->m_string = src->m_string;

    //mask
    if(src->m_propSetFlags&UGCELL_MASK_SET)
        dest->m_mask = src->m_mask;

    //label
    if(src->m_propSetFlags&UGCELL_LABEL_SET)
        dest->m_label = src->m_label;

    //format and style class pointers
    if(src->m_propSetFlags&UGCELL_FORMAT_SET)
        dest->m_format = src->m_format;
    if(src->m_propSetFlags&UGCELL_STYLE_SET)
        dest->m_cellStyle = src->m_cellStyle;

    //data type
    if(src->m_propSetFlags&UGCELL_DATATYPE_SET)
        dest->m_dataType = src->m_dataType;

    //general purpose value
    if(src->m_propSetFlags&UGCELL_PARAM_SET)
        dest->m_param = src->m_param;

    //cell type
    if(src->m_propSetFlags&UGCELL_CELLTYPE_SET)
        dest->m_cellType = src->m_cellType;
    if(src->m_propSetFlags&UGCELL_CELLTYPEEX_SET)
        dest->m_cellTypeEx = src->m_cellTypeEx;

    //colors
    if(src->m_propSetFlags&UGCELL_TEXTCOLOR_SET)
        dest->m_textColor = src->m_textColor;
    if(src->m_propSetFlags&UGCELL_BACKCOLOR_SET)
        dest->m_backColor = src->m_backColor;
    if(src->m_propSetFlags&UGCELL_HTEXTCOLOR_SET)
        dest->m_hTextColor = src->m_hTextColor;
    if(src->m_propSetFlags&UGCELL_HBACKCOLOR_SET)
        dest->m_hBackColor = src->m_hBackColor;

    //borders
    if(src->m_propSetFlags&UGCELL_BORDERSTYLE_SET)
        dest->m_borderStyle = src->m_borderStyle;
    if(src->m_propSetFlags&UGCELL_BORDERCOLOR_SET)
        dest->m_borderColor = src->m_borderColor;

    //font and bitmap
    if(src->m_propSetFlags&UGCELL_FONT_SET)
        dest->m_font = src->m_font;
    if(src->m_propSetFlags&UGCELL_BITMAP_SET)
        dest->m_bitmap = src->m_bitmap;

    //alignment
    if(src->m_propSetFlags&UGCELL_ALIGNMENT_SET)
        dest->m_alignment = src->m_alignment;

    //extra memory
    if(src->m_propSetFlags&UGCELL_EXTRAMEMORY_SET){
        // GHM 2010 optimization
        //if(dest->m_extraMem != NULL)
        //      delete[] dest->m_extraMem;
        if(src->m_extraMem != NULL){
            dest->m_extraMem = src->m_extraMem;
            /*
            TRY{
                dest->m_extraMem = new BYTE[src->m_extraMemSize];
                memcpy(dest->m_extraMem,src->m_extraMem,(size_t)src->m_extraMemSize);
                dest->m_extraMemSize = src->m_extraMemSize;
            }
            CATCH( CException, e ){
                dest->m_extraMem = NULL;
                dest->m_extraMemSize =0;
            }
            END_CATCH
            */
        }
    }

    //join cells
    if(src->m_propSetFlags&UGCELL_JOIN_SET){
        dest->m_joinOrigin = src->m_joinOrigin;
        dest->m_joinCol = src->m_joinCol;
        dest->m_joinRow = src->m_joinRow;
    }

    //decimals
    if(src->m_propSetFlags&UGCELL_NUMBERDEC_SET)
        dest->m_numDecimals = src->m_numDecimals;

    //read only
    if(src->m_propSetFlags&UGCELL_READONLY_SET)
        dest->m_readOnlyFlag = src->m_readOnlyFlag;

    return UG_SUCCESS;
}

/********************************************
//********* text data type functions ********
*********************************************/

/********************************************
SetText
    Purpose
        Sets the text property of a cell object.
        The text property is also the 'data'
        property of a cell
    Params
        text - pointer to a string
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetText(LPCTSTR text){

    m_string = text;
    m_dataType  = UGCELLDATA_STRING;
    m_propSetFlags |= (UGCELL_STRING_SET+UGCELL_DATATYPE_SET);

    return UG_SUCCESS;
}
/********************************************
AppendText
    Purpose
        Appends text to the cells text property
    Params
        text - pointer to a string
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::AppendText(LPCTSTR text){

    if( ( m_propSetFlags&UGCELL_STRING_SET ) == 0)
        return UG_ERROR;

    m_string += text;
    m_dataType  = UGCELLDATA_STRING;
    m_propSetFlags |= (UGCELL_STRING_SET +UGCELL_DATATYPE_SET);

    return UG_SUCCESS;
}
/********************************************
GetText
    Purpose
        Returns the text from the cells text
        property.
    Params
        text - pointer to a CString object to
            receive the text
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetText(CString *text){

    if(m_propSetFlags&UGCELL_STRING_SET){
        *text = m_string;
        return UG_SUCCESS;
    }

    return UG_ERROR;
}
/********************************************
GetText
    Purpose
        Returns the text from the cells text
        property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        pointer to the text object
*********************************************/
LPCTSTR CUGCell::GetText(){

    if(m_propSetFlags&UGCELL_STRING_SET)
        return (LPCTSTR)m_string;
    return NULL;
}
/********************************************
GetTextLength
    Purpose
        Returns the length of the cell's text
        property.
    Params
        none
    Return
        the length of the text properties text
*********************************************/
int CUGCell::GetTextLength(){

    if(m_propSetFlags&UGCELL_STRING_SET)
        return m_string.GetLength();
    return 0;
}
/********************************************
SetMask
    Purpose
        Sets the mask property of a cell object.
    Params
        text - pointer to a string
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetMask(LPCTSTR text){

    m_mask = text;
    m_propSetFlags |= UGCELL_MASK_SET;

    return UG_SUCCESS;
}
/********************************************
GetMask
    Purpose
        Returns the mask text from the cells mask
        property.
    Params
        text - pointer to a CString object to
            receive the mask
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetMask(CString *text){

    if(m_propSetFlags&UGCELL_MASK_SET){
        *text = m_mask;
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetMask
    Purpose
        Returns the mask text from the cells mask
        property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        LPCTSTR to the mask string
*********************************************/
LPCTSTR CUGCell::GetMask(){

    if(m_propSetFlags&UGCELL_MASK_SET)
        return (LPCTSTR)m_mask;
    return NULL;
}
/********************************************
SetLabelText
    Purpose
        Sets the label text property of a cell
        object.
    Params
        text - pointer to a string
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetLabelText(LPCTSTR text){

    m_label = text;
    m_propSetFlags |= UGCELL_LABEL_SET;
    return UG_SUCCESS;
}
/********************************************
GetLabelText
    Purpose
        Returns the label text from the cells label
        property.
    Params
        text - pointer to a CString object to
            receive the label text
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetLabelText(CString *text){

    if(m_propSetFlags&UGCELL_LABEL_SET){
        *text = m_label;
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetLabelText
    Purpose
        Returns the label text from the cells label
        property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        pointer to the label text
*********************************************/
LPCTSTR CUGCell::GetLabelText(){

    if(m_propSetFlags&UGCELL_LABEL_SET)
        return (LPCTSTR)m_label;
    return NULL;
}

/********************************************
    //***** BOOL data type functions *****
*********************************************/

/********************************************
SetBool
    Purpose
        Sets the booleen state for the cell.
        This data is stored in the cells text
        property, which is also its data property.
    Params
        state - boolean state to set the cell to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetBool(BOOL state){

    if (state)
        m_string = _T("1");
    else
        m_string = _T("0");

    m_dataType  = UGCELLDATA_BOOL;

    m_propSetFlags |= (UGCELL_STRING_SET + UGCELL_DATATYPE_SET);

    return UG_SUCCESS;
}
/********************************************
GetBool
    Purpose
        Returns the booleen state for the cell.
        This information is retrieved from the
        cell's text property.  This function does
        not check to see if the proprty is set,
        so make sure to use the IsPropertySet
        function first.
    Params
        none
    Return
        The boolean state of the cell
*********************************************/
BOOL CUGCell::GetBool(){

    //get the bool
    if(m_string.IsEmpty())
        return FALSE;

    long number = _ttol(m_string);
    if(m_string[0] == _T('T') || m_string[0] == _T('t') || number != 0)
        return TRUE;
    else
        return FALSE;
}
/********************************************
    //***** Number data type functions *****
*********************************************/

/********************************************
SetNumber
    Purpose
        Sets the number value for the cell. The
        number value is stored in the cell's
        text property
    Params
        number - number to set
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetNumber(double number){

    CString buf;
    CString buf2;
    if(m_propSetFlags&UGCELL_NUMBERDEC_SET){
        buf.Format(_T("%%1.%dlf"),m_numDecimals);
        buf2.Format(buf,number);
    }
    else{
        buf2.Format(_T("%lf"),number);
    }

    m_string = buf2;

    m_dataType      = UGCELLDATA_NUMBER;
    m_propSetFlags |= (UGCELL_STRING_SET + UGCELL_DATATYPE_SET);

    return UG_SUCCESS;
}
/********************************************
GetNumber
    Purpose
        Returns the number value for the cell.
        This number is retrieved from the cell's
        text property.
    Params
        number - pointer to an int to recieve
            the value
    Return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::GetNumber(int *number){

    if(m_propSetFlags&UGCELL_STRING_SET){
        *number = _ttoi(m_string);
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetNumber
    Purpose
        Returns the number value for the cell.
        This number is retrieved from the cell's
        text property.
    Params
        number - pointer to a long to recieve
            the value
    Return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::GetNumber(long *number){

    if(m_propSetFlags&UGCELL_STRING_SET){
        *number = _ttol(m_string);
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetNumber
    Purpose
        Returns the number value for the cell.
        This number is retrieved from the cell's
        text property.
    Params
        number - pointer to a float to recieve
            the value
    Return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::GetNumber(float *number){

    if(m_propSetFlags&UGCELL_STRING_SET){

        csprochar *end;
        *number = (float)_tcstod(m_string,&end);
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetNumber
    Purpose
        Returns the number value for the cell.
        This number is retrieved from the cell's
        text property.
    Params
        number - pointer to a double to recieve
            the value
    Return
        UG_SUCCESS  success
        1           fail
*********************************************/
int CUGCell::GetNumber(double *number){

    if(m_propSetFlags&UGCELL_STRING_SET){
        csprochar * end;
        *number = _tcstod(m_string,&end);
        return UG_SUCCESS;
    }
    return UG_ERROR;
}
/********************************************
GetNumber
    Purpose
        Returns the number value for the cell.
        This number is retrieved from the cell's
        text property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        the number value of the cell
*********************************************/
double  CUGCell::GetNumber(){

    if(m_propSetFlags&UGCELL_STRING_SET){
        csprochar * end;
        return _tcstod(m_string,&end);
    }
    return 0;
}
/********************************************
SetNumDecimals
    Purpose
        Sets the number of decimal points to
        display for numbers, this value does
        not change the actual value stored,
        it is only used for display purposes.
    Params
        number - number of decimal points to
            display
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetNumberDecimals(int number){

    m_numDecimals = number;

    m_propSetFlags |= UGCELL_NUMBERDEC_SET;

    return UG_SUCCESS;
}
/********************************************
GetNumberDecimals
    Purpose
        Sets the number of decimal points to
        display for numbers, this value does
        not change the actual value stored,
        it is only used for display purposes.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        number - number of decimal points to
            display
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetNumberDecimals(){

    if(m_propSetFlags&UGCELL_NUMBERDEC_SET)
        return m_numDecimals;
    return 0;
}


/********************************************
    //***** Time data type functions *****
*********************************************/

/********************************************
SetTime
    Purpose
        Sets the time value for the cell. This
        value is converted into a string and
        put in the cell's text property
    Params
        second  0 - 59
        minute  0 - 59
        hour    0 - 23
        day     0 - 31
        month   0 - 12
        year    100 - 9999

    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetTime(short second,short minute,short hour,short day,short month,short year){

    m_dataType  = UGCELLDATA_TIME;

    #ifdef __AFXOLE_H__  //OLE must be included COleVariant ov(string);

        COleDateTime odt;
        odt.SetDateTime(year,month,day,hour,minute,second);
        SetText(odt.Format());  // use default of LANG_USER_DEFAULT
                                // check regional settings re Y2K!!
        m_propSetFlags |= (UGCELL_STRING_SET + UGCELL_DATATYPE_SET);
        return UG_SUCCESS;

    #else

        return 1;

    #endif

}
/********************************************
GetTime
    Purpose
        Gets the time value stored in the cell's
        text property. The value is converted from
        a string to the time components.
    Params
        second  0 - 59
        minute  0 - 59
        hour    0 - 23
        day     0 - 31
        month   0 - 12
        year    100 - 9999

    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetTime(short* second,short* minute,short* hour,short* day,short* month,short* year){

    #ifdef __AFXOLE_H__  //OLE must be included COleVariant ov(string);

    if(m_propSetFlags&UGCELL_TEXT_SET){

        try{
            COleDateTime odt;
            odt.ParseDateTime(m_string);

            *day = odt.GetDay();
            *month = odt.GetMonth();
            *year = odt.GetYear();

            *second = odt.GetSecond();
            *minute = odt.GetMinute();
            *hour = odt.GetHour();
        }
        catch(...){
            return UG_ERROR;
        }
        return UG_SUCCESS;
    }

    #endif

    return UG_ERROR;

}
/********************************************
    //set and get data types
*********************************************/

/********************************************
SetDataType
    Purpose
        Sets the data type that the cells
        text property is to be treaded as.
        valid values:
            UGCELLDATA_STRING
            UGCELLDATA_NUMBER
            UGCELLDATA_BOOL
            UGCELLDATA_TIME
            UGCELLDATA_CURRENCY
    Params
        type
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetDataType(short type){

    m_dataType  = type;
    m_propSetFlags |= UGCELL_DATATYPE_SET;

    return UG_SUCCESS;
}
/********************************************
GetDataType
    Purpose
        Returns the data type that the cells
        data is to be treated as.
        valid values are:
            UGCELLDATA_STRING
            UGCELLDATA_NUMBER
            UGCELLDATA_BOOL
            UGCELLDATA_TIME
            UGCELLDATA_CURRENCY
    Params
        none
    Return
        the data type
*********************************************/
int CUGCell::GetDataType(){

    if(m_propSetFlags&UGCELL_DATATYPE_SET)
        return m_dataType;
    else
        return UGCELLDATA_STRING;
}

/********************************************
    //***** General functions *****
*********************************************/

/********************************************
SetTextColor
    Purpose
        Sets the text color property of a cell
    Params
        color - color to set the property to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetTextColor(COLORREF color){

    m_textColor = color;
    m_propSetFlags |= UGCELL_TEXTCOLOR_SET;

    return UG_SUCCESS;
}

/********************************************
GetTextColor
    Purpose
        Returns the current color of the
        cells text color property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
COLORREF CUGCell::GetTextColor(){

    if(m_propSetFlags&UGCELL_TEXTCOLOR_SET)
        return m_textColor;
    else
        return GetSysColor(COLOR_WINDOWTEXT);
}

/********************************************
SetBackColor
    Purpose
        Sets the background color property of a cell
    Params
        color - color to set the property to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetBackColor(COLORREF color){

    m_backColor = color;
    m_propSetFlags |= UGCELL_BACKCOLOR_SET;

    return UG_SUCCESS;
}

/********************************************
GetBackColor
    Purpose
        Returns the current color of the
        cells background color property. This
        function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
COLORREF CUGCell::GetBackColor(){

    if(m_propSetFlags&UGCELL_BACKCOLOR_SET)
        return m_backColor;
    else
        return GetSysColor(COLOR_WINDOW);
}

/********************************************
SetHTextColor
    Purpose
        Sets the highlight text color property
        of a cell
    Params
        color - color to set the property to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetHTextColor(COLORREF color){

    m_hTextColor = color;
    m_propSetFlags |= UGCELL_HTEXTCOLOR_SET;

    return UG_SUCCESS;
}

/********************************************
GetHTextColor
    Purpose
        Returns the current color of the
        cells highlight text color property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
COLORREF CUGCell::GetHTextColor(){

    if(m_propSetFlags&UGCELL_HTEXTCOLOR_SET)
        return m_hTextColor;
    else
        return GetSysColor(COLOR_HIGHLIGHTTEXT);
}

/********************************************
SetHBackColor
    Purpose
        Sets the highlight background color
        property of a cell
    Params
        color - color to set the property to
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetHBackColor(COLORREF color){

    m_hBackColor = color;
    m_propSetFlags |= UGCELL_HBACKCOLOR_SET;

    return UG_SUCCESS;
}

/********************************************
GetHBackColor
    Purpose
        Returns the current color of the
        cells highlight background color property.
        This function does not check to see
        if the property is set, so use the
        IsPropertySet function first.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
COLORREF CUGCell::GetHBackColor(){

    if(m_propSetFlags&UGCELL_HBACKCOLOR_SET)
        return m_hBackColor;
    else
        return GetSysColor(COLOR_HIGHLIGHT);
}

/********************************************
SetAlignment
    Purpose
        Sets the alignment property of the
        cell. Verticle and horizontal settings
        can be ORed together.
        Valid values are:
            UG_ALIGNLEFT
            UG_ALIGNRIGHT
            UG_ALIGNCENTER
            UG_ALIGNTOP
            UG_ALIGNBOTTOM
            UG_ALIGNVCENTER
    Params
        align - the alignment value to set
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetAlignment(short align){

    m_propSetFlags |= UGCELL_ALIGNMENT_SET;
    m_alignment = align;

    return UG_SUCCESS;
}

/********************************************
GetAlignment
    Purpose
        Returns the alignment value from the
        cell's alignment property.
        Valid values are:
            UG_ALIGNLEFT
            UG_ALIGNRIGHT
            UG_ALIGNCENTER
            UG_ALIGNTOP
            UG_ALIGNBOTTOM
            UG_ALIGNVCENTER
        These values are set as BITs within the
        return value.
    Params
        none
    Return
        Alignment property value
*********************************************/
short CUGCell::GetAlignment(){

    if(m_propSetFlags&UGCELL_ALIGNMENT_SET)
        return m_alignment;
    return 0;
}

/********************************************
SetBorder
    Purpose
        Sets the border property of the cell.
        Border styles are ORed together.
        Valid values are:
            UG_BDR_LTHIN
            UG_BDR_TTHIN
            UG_BDR_RTHIN
            UG_BDR_BTHIN
            UG_BDR_LMEDIUM
            UG_BDR_TMEDIUM
            UG_BDR_RMEDIUM
            UG_BDR_BMEDIUM
            UG_BDR_LTHICK
            UG_BDR_TTHICK
            UG_BDR_RTHICK
            UG_BDR_BTHICK
            UG_BDR_RECESSED
            UG_BDR_RAISED
    Params
        style
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetBorder(short style){

    m_propSetFlags |= UGCELL_BORDERSTYLE_SET;
    m_borderStyle = style;

    return UG_SUCCESS;
}

/********************************************
GetBorder
    Purpose
        Returns the cells border style property.
        The return value is made up of bits which
        corresond to a style for each piece of the
        border.
        Valid values are.
            UG_BDR_LTHIN
            UG_BDR_TTHIN
            UG_BDR_RTHIN
            UG_BDR_BTHIN
            UG_BDR_LMEDIUM
            UG_BDR_TMEDIUM
            UG_BDR_RMEDIUM
            UG_BDR_BMEDIUM
            UG_BDR_LTHICK
            UG_BDR_TTHICK
            UG_BDR_RTHICK
            UG_BDR_BTHICK
            UG_BDR_RECESSED
            UG_BDR_RAISED
    Params
        none
    Return
        border property value
*********************************************/
short CUGCell::GetBorder(){

    if(m_propSetFlags&UGCELL_BORDERSTYLE_SET)
        return m_borderStyle;
    return 0;
}

/********************************************
SetBorderColor
    Purpose
        Sets the border color property of
        the cell. The color used is in the
        form of a CPen object. The CPen should
        use the PS_SOLID style and be 1 pixel
        wide.
        The cell object will not destroy the object
        that is passed in here. It is the responsibilty
        of the caller to destroy the object when finished.
    Params
        pen - pointer to the cpen object to use.
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetBorderColor(CPen *pen){

    m_propSetFlags |= UGCELL_BORDERCOLOR_SET;
    m_borderColor = pen;

    return UG_SUCCESS;
}

/********************************************
GetBorderColor
    Purpose
        Returns the pointer to a CPen Object that
        is used to draw the cells border.
    Params
        none
    Return
        Pointer to a CPen object, or NULL if
        this property is not set
*********************************************/
CPen * CUGCell::GetBorderColor(){

    if(m_propSetFlags&UGCELL_BORDERCOLOR_SET)
        return m_borderColor;
    return NULL;
}

/********************************************
SetFont
    Purpose
        Sets the font property of the cell.
        The same CFont can be used for multiple
        cells.
        The cell object will not destroy the object
        that is passed in here. It is the responsibilty
        of the caller to destroy the object when finished.
    Params
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetFont(CFont *font){

    m_propSetFlags |= UGCELL_FONT_SET;
    m_font = font;

    return UG_SUCCESS;
}

/********************************************
GetFont
    Purpose
        Returns the font property for the cell.
        The font property is a pointer to a
        CFont object.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
CFont * CUGCell::GetFont(){

    if(m_propSetFlags&UGCELL_FONT_SET)
        return m_font;
    return NULL;
}

/********************************************
SetBitmap
    Purpose
        Sets the bitmap property of the cell.
        The bitmap property holds a pointer
        to a CBitmap object, but does not
        take ownership. When the cell object
        is destroyed it does not delete the
        bitmap object, it is the responsibility
        of the calling function/application.
    Params
        bitmap - pointer to a CBitmap object
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetBitmap(CBitmap * bitmap){

    if(NULL != bitmap) {
        m_propSetFlags |= UGCELL_BITMAP_SET;
    }
    else {
        if( m_propSetFlags&UGCELL_BITMAP_SET)
             m_propSetFlags ^= UGCELL_BITMAP_SET;   // clear flag
    }

    m_bitmap = bitmap;

    return UG_SUCCESS;
}

/********************************************
GetBitmap
    Purpose
        Returns the cells bitmap property.
        The property is a pointer to an
        existing CBitmap object.
    Params
        none
    Return
        pointer to a CBitmap
*********************************************/
CBitmap * CUGCell::GetBitmap(){

    if(m_propSetFlags&UGCELL_BITMAP_SET)
        return m_bitmap;
    return NULL;
}

/********************************************
SetReadOnly
    Purpose
        Sets the cells read only property.
        If true then the cell cannot be
        edited, but SetCell functions will
        still work.
    Params
        state - TRUE or FALSE
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetReadOnly(BOOL state){

    if(state)
        m_readOnlyFlag = TRUE;
    else
        m_readOnlyFlag = FALSE;

    m_propSetFlags |= UGCELL_READONLY_SET;

    return UG_SUCCESS;
}

/********************************************
GetReadOnly
    Purpose
        Returns the cell's read only property.
    Params
        none
    Return
        TRUE/FALSE - read only property state
*********************************************/
BOOL CUGCell::GetReadOnly(){

    if(m_propSetFlags&UGCELL_READONLY_SET)
        return m_readOnlyFlag;
    return FALSE;
}

/********************************************
SetCellType
    Purpose
        Sets the cells cell type property.
        The cell type must be registered with
        the grid to work (or built in)
    Params
        index - the index value of the registered
            cell type
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetCellType(int index){

    m_propSetFlags |= UGCELL_CELLTYPE_SET;

    m_cellType = index;

    return UG_SUCCESS;
}

/********************************************
SetCellTypeEx
    Purpose
        Sets the extended style of a cell type.
        Extended styles are cell type dependant.
    Params
        type - extended style number
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetCellTypeEx(long type){

    m_propSetFlags |= UGCELL_CELLTYPEEX_SET;
    m_cellTypeEx = type;

    return UG_SUCCESS;
}

/********************************************
GetCellType
    Purpose
        Returns the cell type that the cell is
        using. This function does not check
        to see if the property is set or not.
    Params
        none
    Return
        cell type that the cell is using
*********************************************/
int CUGCell::GetCellType(){

    if(m_propSetFlags&UGCELL_CELLTYPE_SET)
        return m_cellType;
    else
        return 0;
}

/********************************************
GetCellTypeEx
    Purpose
        Returns the celltype extended style
        property. This function does not check
        to see if the property is set or not.
    Params
        none
    Return
        cell type extended style property value
*********************************************/
long CUGCell::GetCellTypeEx(){

    if(m_propSetFlags&UGCELL_CELLTYPEEX_SET)
        return m_cellTypeEx;
    return 0;
}

/********************************************
SetJoinInfo
    Purpose
        Sets join information for a cell.
        If origin is TRUE then col and row
        point to the last cell in the region.
        If origin is FALSE then col and row
        point to the first cell in the region.
        col and row are relative numbers.
        Ever cell in the joined region need to
        have their join properties set.
    Params
        origin - join region origin flag
        col - relative col to the start or end of join
        row - relative row to the start or end of join
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetJoinInfo(BOOL origin,int col,long row){

    m_joinOrigin  = (short)origin;
    m_joinCol    = col;
    m_joinRow    = row;

    m_propSetFlags |= UGCELL_JOIN_SET;

    return UG_SUCCESS;
}

/********************************************
GetJoinInfo
    Purpose
        Returns the join information for the cell,
        if set. see SetJoinInfo for more information.
        Values are returned in the parameters passed
        in.
    Params
        origin - join region origin flag
        col - relative col to the start or end of join
        row - relative row to the start or end of join
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::GetJoinInfo(BOOL *origin,int *col,long *row){

    if(m_propSetFlags&UGCELL_JOIN_SET){
        *origin = m_joinOrigin;
        *col    = m_joinCol;
        *row    = m_joinRow;

        return UG_SUCCESS;
    }
    return UG_ERROR;
}

/********************************************
ClearExtraMem
    Purpose
        Clears data that was set in the cells
        extra memory property.
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::ClearExtraMem(){

/*  if(m_propSetFlags&UGCELL_EXTRAMEMORY_SET)
        m_propSetFlags -= UGCELL_EXTRAMEMORY_SET;

    if(m_extraMem != NULL){
        delete[] m_extraMem;
        m_extraMem = NULL;
        m_extraMemSize = 0;
    }
*/

    // GHM 20100526 optimization
    if( m_propSetFlags & UGCELL_EXTRAMEMORY_SET )
        m_propSetFlags -= UGCELL_EXTRAMEMORY_SET;

    m_extraMem = NULL;
    m_extraMemSize = 0;

    return UG_SUCCESS;
}

/********************************************
AllocExtraMem
    Purpose
        Allocates a block of data using the
        specified size. This data can hold any
        type of information.
    Params
        len - length of data of allocate, in bytes.
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
/*void *    CUGCell::AllocExtraMem(long len){

    ClearExtraMem();

    try{
        m_extraMem = new BYTE[len];
        if(m_extraMem == NULL)
            return NULL;
    }
    catch( CException* e ){
        delete e;
        return NULL;
    }

    m_propSetFlags |= UGCELL_EXTRAMEMORY_SET;
    m_extraMemSize = len;

    return m_extraMem;
}*/ // GHM 20100526 optimization

/********************************************
GetExtraMemPtr
    Purpose
        Returns a pointer to the extra memory
        that was allocated, using the function
        above. This function does not check to
        see if this property was set or not.
    Params
        none
    Return
        pointer to the extra cell memory
*********************************************/
/*void *    CUGCell::GetExtraMemPtr(){

    if(m_propSetFlags&UGCELL_EXTRAMEMORY_SET)
        return m_extraMem;
    return NULL;
}*/ // GHM 20100526 optimization

/********************************************
GetExtraMemSize
    Purpose
        Returns the number of bytes that were
        allocated for the cells extra memory
        data block. This function does not
        check to see if this property was set
        or not.
    Params
        none
    Return
        extra memory size in bytes.
*********************************************/
long CUGCell::GetExtraMemSize(){

    if(m_propSetFlags&UGCELL_EXTRAMEMORY_SET)
        return m_extraMemSize;
    return 0;
}

/********************************************
StringToNumber
    Purpose
        Converts a string to a number.
        This function will work with strings
        containing characters other than numbers,
        by removing non-digit characters before the
        conversion. Therefore it will work with
        strings such as: "$123,456.00","123-456-789"
    Params
        string - string to convert
        number - resulting number
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::StringToNumber(CString *string,double *number){

    int loop;
    int len = string->GetLength();
    CString tempString = _T("");
    int numdecimals = m_numDecimals;
    int periodfound = FALSE;
    csprochar c;

    for(loop=0;loop <len;loop++){
        c = string->GetAt(loop);
        if(_istdigit(c) || c == _T('.') || c == _T('-')){
            if(c==_T('.')){
                if(periodfound)
                    continue;
                else
                    periodfound=TRUE;
            }
            if(periodfound){
                if(numdecimals <1)
                    continue;
                else
                    numdecimals--;
            }
            tempString += string->Mid(loop,1);
        }

    }
    *number = _ttol(tempString);
    return UG_SUCCESS;
}

/********************************************
StringToBool
    Purpose
        converts the given string to a bool value.
        This function returns strings containing
        characters other than '0' or 'F' as true.
    Params
        string - string to convert
        state - resulting bool value
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::StringToBool(CString *string,BOOL * state){
    if(string->GetAt(0) == _T('0') || string->GetAt(0) == _T('F') ||
      string->GetAt(0) == _T('f'))
        *state = FALSE;
    else
        *state = TRUE;
    return UG_SUCCESS;
}

/********************************************
StringToTime
    Purpose
        converts a string into a time value.
        This function using the current locale
        setting to help perform the conversion.
    Params
        string - string to convert
        second - resulting second
        minute - resulting minute
        hour - resulting hour
        day - resulting day
        month  - resulting month
        year - resulting year
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::StringToTime(CString* string,int* second,int* minute,int* hour,int* day,
                          int* month,int* year){


//only include this function is OLE is enales
#ifdef __AFXOLE_H__

    COleVariant ovStr(*string);

    COleDateTime ovTime(ovStr);

    if(ovTime.GetStatus() != COleDateTime::valid)
        return UG_ERROR;

    *second = ovTime.GetSecond();
    *minute = ovTime.GetMinute();
    *hour = ovTime.GetHour();

    *day = ovTime.GetDay();
    *month = ovTime.GetMonth();
    *year = ovTime.GetYear();

    return UG_SUCCESS;

#else
    return UG_ERROR;
#endif
}

/********************************************
IsPropertySet
    Purpose
        Returns TRUE if the specified property has
        been set or FALSE is it has not been set.
        This function should be called before
        most of the cells Get... functions. Since
        most Get... funtions do not check to see
        if the property has been set.
    Params
        flag - property to check - see the defines in the header file
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
BOOL CUGCell::IsPropertySet(long flag){
    if(flag&m_propSetFlags)
        return TRUE;
    return FALSE;
}

/********************************************
SetPropertyFlags
    Purpose
        Sets the flag contains which properties have
        been set. The flag uses one bit for each property
        so to set multiple properties OR the property set
        defines together. These defines are found in the header
        file for this class.
    Params
        flags - flags containing the properties to be set
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetPropertyFlags(long flags){

    m_propSetFlags = flags;

    return UG_SUCCESS;
}

/********************************************
GetPropertyFlags
    Purpose
        Returns the property flags value.
        Each bit in this value is used for
        one property. See the header file for
        a complete list
    Params
        none
    Return
        property flags value
*********************************************/
long CUGCell::GetPropertyFlags(){
    return m_propSetFlags;
}

/********************************************
ClearProperty
    Purpose
        Removes a property from the cells
        properties flag. Any property that
        does not have its bit set in this flag
        is considered NOT SET.
    Params
        Flags to remove
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::ClearProperty(long propertyFlag){
    return ClearPropertySetFlag(propertyFlag);
}
//this one is kept for backward compat.
int CUGCell::ClearPropertySetFlag(long propertyFlag){
    if(m_propSetFlags&propertyFlag)
        m_propSetFlags -= propertyFlag;

    return UG_SUCCESS;
}

/********************************************
SetCellStyle
    Purpose
        Sets a pointer to another cell object
        that acts as a style for this cell.
        Therefore when properties are being retrieved
        the information found in this cell is used
        first.
    Params
        cell - pointer to a CUGCell object
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetCellStyle(CUGCell *cell){

    m_propSetFlags |= UGCELL_STYLE_SET;

    m_cellStyle = cell;

    return UG_SUCCESS;
}
/********************************************
GetCellStyle
    Purpose
        Returns the pointer to the cells
        style cell object. This cell is
        generally used to give another cell
        a specific style
    Params
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
CUGCell * CUGCell::GetCellStyle(){

    if(m_propSetFlags&UGCELL_STYLE_SET)
        return m_cellStyle;
    return NULL;
}

/********************************************
SetFormatClass
    Purpose
        Sets a pointer to a formating class
        that the cell will using for
        formating data for display and validation
        of its text
    Params
        formt - pointer to a format class
    Return
        UG_SUCCESS  success
        UG_ERROR    fail
*********************************************/
int CUGCell::SetFormatClass(CUGCellFormat *format){

    if(format == NULL)
        return 1;

    m_propSetFlags |= UGCELL_FORMAT_SET;
    m_format = format;

    return UG_SUCCESS;
}

/********************************************
GetFormatClass
    Purpose
        Returns the pointer to the cells format
        class. This funciton does not check to
        see if this property is set or not
    Params
        none
    Return
        pointer to the cells format class
*********************************************/
CUGCellFormat * CUGCell::GetFormatClass(){

    if(m_propSetFlags&UGCELL_FORMAT_SET)
        return m_format;
    return NULL;
}
/********************************************
SetParam
*********************************************/
int CUGCell::SetParam(long param){

    m_param = param;
    m_propSetFlags |= UGCELL_PARAM_SET;

    return UG_SUCCESS;
}
/********************************************
GetParam
*********************************************/
long CUGCell::GetParam(){

    if(m_propSetFlags&UGCELL_PARAM_SET)
        return m_param;
    return 0;
}
