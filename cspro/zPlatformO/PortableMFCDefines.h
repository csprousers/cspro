#pragma once

#include <assert.h>

#ifdef _DEBUG
#define ASSERT(f)           assert(f)
#else
#define ASSERT(f)           ((void)0)
#endif

#define ASSERT_VALID(pOb)   assert(pOb)
#define TRACE(msg, ...)     ((void)0)

#define AFX_EXT_CLASS

// MFC dynamic handling stubbed out in favor of std. C++ dynamic_cast
#define DYNAMIC_DOWNCAST(class_name, object) dynamic_cast<class_name*>(object)

#define DECLARE_DYNAMIC(class_name)

#define IMPLEMENT_DYNAMIC(class_name, base_class_name)


#ifdef _DEBUG
#define DEBUG_NEW new
#endif

// the following are from afx.h
struct __POSITION {};
typedef __POSITION* POSITION;

