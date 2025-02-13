#pragma once

// LocalGetParent can be used with VART* and GROUPT* to
// get Parent [or Owner, when parent is null]
//
template <class T> GROUPT* LocalGetParent( T pWhat )
{
    if( pWhat == 0 ) return 0;

    T pAux = pWhat;
    GROUPT* pRet = pWhat->GetParentGPT();
    if( pRet == 0 )
        pRet = pAux->GetOwnerGPT();

    return pRet;
}
