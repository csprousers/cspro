// MsgParam.cpp: implementation of the CMsgParam class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MsgParam.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsgParam::CMsgParam()
{
    this->bParam                                = false;
    this->csParam                               = _T("");
    //this->dwParam                             = 0;
    this->hParam                                = NULL;
    this->iParam                                = -1;
    this->bMustBeDestroyedAfterLastCatchMessage = false;
}

CMsgParam::~CMsgParam()
{
    dwArrayParam.RemoveAll();
}
