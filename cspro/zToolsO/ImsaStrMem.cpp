#include "StdAfx.h"
#include "Tools.h"


/* Elimina los blancos (y menores) de la derecha. Retorna el numero de */
/* caracteres. */
short trimright(TCHAR* in)
  {
    short    i, len;

    len = (short)_tcslen( in );
    for( i = len - 1; i >= 0; i-- )
        if( in[i] > ' ' )
            break;

    i = ( i < 0 ) ? 0 : i + 1;
    in[i] = 0;

    return (short)_tcslen( in );
  }


/* Elimina los blancos (y menores) de la izquierda. Retorna el numero de */
/* caracteres. */
short trimleft(TCHAR* in)
  {
    short    i, len, lenrest;

    len = (short)_tcslen( in );
    for( i = 0; i < len; i++ )
        if( in[i] > ' ' )
            break;

    /* Puros blancos */
    if( i == len )
        *in = 0;
    else
      {
        lenrest = (short)_tcslen( in + i );
        memmove( in, in + i, lenrest * sizeof(TCHAR) );
        in[lenrest] = 0;
      }

    return (short)_tcslen( in );
  }


short trimall(TCHAR* in)
  {
    short len;

    if( ( len = trimright( in ) ) > 0 )
         len = trimleft( in );

    return len ;
  }


// Copia bufin en bufout alineado a la derecha. Trunca por la izquierda si no cabe
void memcpyright( csprochar *bufout, int lenout, csprochar* bufin, int lenin )
{
    if( lenout >= lenin )
        _tmemcpy( bufout + lenout - lenin, bufin, lenin );
    else
        _tmemcpy( bufout, bufin + lenin - lenout, lenout );
}


void strcpymax( csprochar *pszOut, const csprochar *pszIn, int iMaxLen )
{
    int  iLen = _tcslen( pszIn );
    iLen = std::min( iLen, iMaxLen );

    _tmemcpy( pszOut, pszIn, iLen );
    pszOut[iLen] = 0;
}


/* Retorna el largo que quedara buf despues del reemplazo. -1 si in es 0 */
int strsizereplace( csprochar *buf, csprochar *in, csprochar *out )
  {
    csprochar    *p, *q;
    int     lenbuf, lenout, lenin, maxlenout=0, slotsin, desp;

    lenbuf = _tcslen( buf );
    lenin  = _tcslen( in );
    lenout = _tcslen( out );

    if( lenin == 0 )
        return( -1 ); /* No se permite in de largo 0 */

    slotsin = lenbuf / lenin + 1;

    /* El buffer se ahica. El peor caso es que no hay reemplazo */
    if( lenout <= lenin )
        maxlenout = lenbuf;
    /* el buffer se agranda. El peor caso es que se reemplazan todos los slots */
    else /* if( lenout > lenin ) */
        maxlenout = slotsin * lenout; /* + 1; */


    q = buf;
    desp = 0;
    while( ( p = _tcsstr( q, in ) ) != NULL )
      {
        *p = 0;
        desp += _tcslen(q) + _tcslen(out);
        *p = *in;
        q = p + _tcslen(in);
      }

    desp += _tcslen(q);

/*printf( "lenbuf=%d, lenbufout=%d, normal=%d\n", lenbuf, maxlenout, desp ); */

    return( desp );
  }


/* Reemplaza el string in por out en buf. Si out es mas largo que in el buffer */
/* crece de tamano (CUIDADO CON VIOLACIONES DE MEMORIA). */
short strreplace( csprochar *buf, csprochar *in, csprochar *out )
  {
#ifdef WIN_DESKTOP
    csprochar    *p, *q, *bufaux;
    int     lenout=0, desp;

    /* strlen(in) == 0 */
    if( ( lenout = strsizereplace( buf, in, out ) ) == -1 )
        return( FALSE );

    if( ( bufaux = (csprochar *) calloc( ( lenout + 1 ), sizeof(csprochar) ) ) == NULL )
        return( FALSE );

    q = buf;
    desp = 0;
    while( ( p = _tcsstr( q, in ) ) != NULL )
      {
        *p = 0;
        desp += _stprintf( bufaux + desp, _T("%ls%ls"), q, out );
        *p = *in;
        q = p + _tcslen(in);
      }

    desp += _stprintf( bufaux + desp, _T("%ls"), q );

    _tcscpy( buf, bufaux );

    free( bufaux );

    return( TRUE );
#else
    assert(false); //TODO: port this code??? - use the std version of this code - _stprintf( bufaux + desp, sizeof(bufaux),_T("%ls%ls"), q, out );
    return FALSE;
#endif
  }
