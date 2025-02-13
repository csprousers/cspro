#pragma once

#pragma warning(disable:4100)
#pragma warning(disable:4800)

#include <engine/StandardSystemIncludes.h>

#define HINT_CHANGEFONT        1      // hints OnUpate() to recalc after font changes (in CMainFrame::OnOptionsFont)

/*--- global prototype  ---*/
BOOL IsProgressDlgActive(void);       // csc 9/9/96 moved over from IMPSUtil

#define MAXFILTER       60
#define MAXCLIP95  1000000

const int TAB_SPACES = 4; // GHM 20120514


#include <zToolsO/Tools.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProgressDlg.h>

/*--- TextView includes ---*/
#include <TextView/resource.h>
#include <TextView/ChildFrm.h>
#include <TextView/Finddlg.h>
#include <TextView/Fsizedlg.h>
#include <TextView/Gotodlg.h>
#include <TextView/MainFrm.h>
#include <TextView/Seldlg.h>
#include <TextView/TextView.h>
#include <TextView/Tvblock.h>
#include <TextView/TVDoc.h>
#include <TextView/Tvmisc.h>
#include <TextView/Tvruler.h>
#include <TextView/TVView.h>
#include <TextView/UWM.h>
