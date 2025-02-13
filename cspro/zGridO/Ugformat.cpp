/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class CUGFormat
************************************************/

#include "StdAfx.h"

#include "Ugctrl.h"
//#include "UGFormat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/***********************************************
************************************************/
CUGCellFormat::CUGCellFormat(){
}

/***********************************************
************************************************/
CUGCellFormat::~CUGCellFormat(){
}

/***********************************************
************************************************/
void CUGCellFormat::ApplyDisplayFormat(CUGCell *cell){
}

/***********************************************
return
    0 - information valid
    1 - information invalid
************************************************/
int CUGCellFormat::ValidateCellInfo(CUGCell *cell){
    return 0;
}
