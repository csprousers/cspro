#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class
        CUGPrtList
    Purpose
        This is a specialized pointer list
        that also has extra fields for storing
        information on each pointer, such as
        a ID number a GUID and even a string name.
    Details
        -this class will not delete the pointer
         attached to it when the item is being
         deleted and/or destroyed. Therefore
         it is the responsibilty of the calling
         program to destroy its own pointers.
************************************************/

#include <zGridO/zGridO.h>


class CLASS_DECL_ZGRIDO CUGPtrList: public CObject{

protected:

    typedef struct UGPtrListTag{
        BOOL    isUsed;
        LPVOID  pointer;
        long    param;
        UGID    id;
    }UGPtrList;

    UGPtrList * m_arrayPtr;

    int     m_maxElements;
    int     m_elementsUsed;

    int AddMoreElements();

public:

    CUGPtrList();
    ~CUGPtrList();

    int AddPointer(void *ptr,long param = 0,UGID *id = NULL);

    LPVOID GetPointer(int index);
    long GetParam(int index);
    UGID* GetUGID(int index);

    int GetPointerIndex(void * ptr);

    int UpdateParam(int index,long param);

    int DeletePointer(int index);
    int EmptyList();

    int GetCount();
    int GetMaxCount();

    int InitEnum();
    void *EnumPointer();
};
