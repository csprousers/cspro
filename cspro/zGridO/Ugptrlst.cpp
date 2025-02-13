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

#include "StdAfx.h"

#include <ctype.h>
#include "Ugctrl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/**************************************
***************************************/
CUGPtrList::CUGPtrList(){
    m_elementsUsed = 0;
    m_maxElements = 0;
    m_arrayPtr = NULL;
}
/**************************************
***************************************/
CUGPtrList::~CUGPtrList(){
    EmptyList();
}
/**************************************
returns the 0 based index
-1 on error
***************************************/
int CUGPtrList::AddPointer(void *ptr,long param,UGID*id){


    //check to see if more elements need to be added
    if(m_elementsUsed >= m_maxElements){
        if(AddMoreElements() == UG_ERROR)
            return -1;
    }

    for(int loop = 0;loop <m_maxElements; loop++){

        if(m_arrayPtr[loop].isUsed == FALSE){

            m_arrayPtr[loop].isUsed = TRUE;
            m_arrayPtr[loop].pointer = ptr;
            m_arrayPtr[loop].param = param;
            if(id != NULL)
                memcpy(&m_arrayPtr[loop].id,id,sizeof(UGID));

            m_elementsUsed++;

            return loop;
        }
    }
    return -1;
}
/**************************************
***************************************/
int CUGPtrList::UpdateParam(int index,long param){

    if(index <0 || index >= m_maxElements)
        return UG_ERROR;

    if(m_arrayPtr[index].isUsed == FALSE)
        return 2;

    m_arrayPtr[index].param = param;

    return UG_SUCCESS;
}

/**************************************
***************************************/
long CUGPtrList::GetParam(int index){

    if(index <0 || index >= m_maxElements)
        return NULL;

    return m_arrayPtr[index].param;
}

/**************************************
***************************************/
LPVOID CUGPtrList::GetPointer(int index){

    if(index <0 || index >= m_maxElements)
        return NULL;

    if(m_arrayPtr[index].isUsed == FALSE)
        return NULL;

    return m_arrayPtr[index].pointer;
}

/**************************************
***************************************/
int CUGPtrList::GetPointerIndex(void * ptr){

    for(int loop = 0;loop <m_maxElements; loop++){

        if(m_arrayPtr[loop].isUsed == FALSE)
            continue;

        if(m_arrayPtr[loop].pointer == ptr)
            return loop;
    }
    return -1;
}

/**************************************
***************************************/
int CUGPtrList::DeletePointer(int index){

    if(index <0 || index >= m_maxElements)
        return UG_ERROR;

    if(m_arrayPtr[index].isUsed != FALSE){
        m_arrayPtr[index].isUsed = FALSE;
        m_arrayPtr[index].pointer = NULL;
        m_elementsUsed--;
    }

    return UG_SUCCESS;
}
/**************************************
***************************************/
int CUGPtrList::EmptyList(){

    delete[] m_arrayPtr;

    m_elementsUsed = 0;
    m_maxElements = 0;

    return UG_SUCCESS;
}
/**************************************
***************************************/
int CUGPtrList::GetCount(){
    return m_elementsUsed;
}
/**************************************
***************************************/
int CUGPtrList::GetMaxCount(){
    return m_maxElements;
}
/**************************************
***************************************/
int CUGPtrList::AddMoreElements(){


    UGPtrList* temp;
    int oldMaxElements = m_maxElements;

    try{
        temp = new UGPtrList[m_maxElements+10];
        m_maxElements += 10;
    }
    catch(...){
        return UG_ERROR;
    }

    int loop;

    for(loop = 0;loop <oldMaxElements;loop++){
        temp[loop].isUsed = m_arrayPtr[loop].isUsed;
        temp[loop].pointer = m_arrayPtr[loop].pointer;
        temp[loop].param = m_arrayPtr[loop].param;
        memcpy(&temp[loop].id,&m_arrayPtr[loop].id,sizeof(UGID));
    }

    for(loop = m_elementsUsed;loop <m_maxElements;loop++){
        temp[loop].isUsed = FALSE;
    }
    delete[] m_arrayPtr;

    m_arrayPtr = temp;

    return UG_SUCCESS;
}

