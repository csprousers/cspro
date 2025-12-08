#pragma once

#pragma warning(disable:4100)
#pragma warning(disable:4800)

#include <engine/StandardSystemIncludes.h>
// X64_TODO #include <StandardIncludes/strict_errors.h>

#define HINT_CHANGEFONT        1      // hints OnUpate() to recalc after font changes (in CMainFrame::OnOptionsFont)

/*--- global prototype  ---*/
BOOL IsProgressDlgActive(void);       // csc 9/9/96 moved over from IMPSUtil

#define MAXFILTER       60
#define MAXCLIP95  1000000

const int TAB_SPACES = 4; // 20120514


#include <zToolsO/Tools.h>
#include <zUtilO/ImsaFileDlg.h>
#include <zUtilO/ImsaStr.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProgressDlg.h>

/*--- TextView includes ---*/
#include <TextView/resource.h>
#include <TextView/ChildFrm.h>
#include <TextView/FindDlg.h>
#include <TextView/FontSizeDlg.h>
#include <TextView/GotoDlg.h>
#include <TextView/MainFrm.h>
#include <TextView/SelectionDlg.h>
#include <TextView/TextView.h>
#include <TextView/TvBlock.h>
#include <TextView/TVDoc.h>
#include <TextView/TvMisc.h>
#include <TextView/TvRuler.h>
#include <TextView/TVView.h>
#include <TextView/UWM.h>
