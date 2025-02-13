#pragma once

#ifdef  ZEXTAB_IMPL
    #define CLASS_DECL_ZEXTAB __declspec(dllexport)
#else
    #define CLASS_DECL_ZEXTAB __declspec(dllimport)
#endif


#include <zExTab/RunTab.h>
#include "SelDlg.h"
