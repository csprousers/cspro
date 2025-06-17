//-----------------------------------------------------------------------
//
// LABSALO.cpp    load & save labels (only DI, SE & VA supported)
// LOAD LABELS FROM a CSPRO DICTIONARY
//
//-----------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/Engdrv.h>

// ISSA basic sizes - legacy constants for DDCONVRT only
#define N_VALLAB        1000                    // Number of Value/Labels for editing
#define POSLAB_TYPE     unsigned short          // cambiar POSLAB a 4 afecta la estructura del area de labels del diccionario
#define POSLAB          ( 2 / sizeof(TCHAR) )   // Posicion donde empiezan los labels
                                                // en Labtbase. Cambiar a 4 si se quiere
                                                // que el largo de Labtbase sea int

#define MAXUSHORT   65535

const int   _LLABEL    = 256;
const int   _LVALLAB   = 256;
const int   _LLABLAB   = 256;


csprochar *putlabel( csprochar *to, csprochar *from );


/*------------------------------------------------------------------------*/
/* lab_load : loads labels of an object                                   */
/*------------------------------------------------------------------------*/
std::vector<TCHAR> CEngineArea::lab_load(int isym)
{
    int idic = -1;

    if( isym >= 0 ) {
        switch( NPT(isym)->GetType() ) {
            case SymbolType::Pre80Dictionary:
                idic = isym;
                break;

            case SymbolType::Section:
                idic = ownerdic( isym );
                break;

            case SymbolType::Variable:
                idic = ownerdic( isym );
                break;
        }
    }

    if( idic < 0 )
        return { };

    constexpr size_t Labtmxent = 50000;
    std::vector<TCHAR> label_data(Labtmxent);
    TCHAR* Labtbase = label_data.data();

    int          l_size, somelab=FALSE, len;
    csprochar    *to;

    to = Labtbase + POSLAB;

    switch( NPT(isym)->GetType() ) {
    case SymbolType::Pre80Dictionary:
        const CDataDict* pDataDict;
        DICT    *dp;
        csprochar    dlabel[_LLABEL+1], downer[_LLABEL+1], dcountry[_LLABEL+1],
            qlab[4][_LLABLAB+1];

        dp = DPT(isym);
        pDataDict = dp->GetDataDict();

        strcpymax( dlabel, pDataDict->GetLabel().GetString(), _LLABEL );
        strcpymax( downer, _T(""), _LLABEL );
        strcpymax( dcountry, _T("USA"), _LLABEL );

        somelab = _tcslen( dlabel );
        somelab |= _tcslen( downer );
        somelab |= _tcslen( dcountry );

        for( size_t i = 0; i < 4; i++ ) {
            if( i < pDataDict->GetNumLevels() ) {
                const DictLevel& dict_level = pDataDict->GetLevel(i);

                strcpymax( qlab[i], dict_level.GetLabel(), _LLABLAB );
                somelab |= _tcslen( qlab[i] );
            }
            else {
                strcpymax( qlab[i], _T(""), _LLABLAB );
            }
        }

        if( somelab ) {
            to = putlabel( to, dlabel );
            to = putlabel( to, downer );
            to = putlabel( to, dcountry );
            for( int i = 0; i < 4; i++ )
                to = putlabel( to, qlab[i] );
            // RHF COM 19/10/98 *( to++ ) = '\n'; *to = '\0';
            *( to++ ) = _T('\n');*( to++ ) = _T('\n');*( to++ ) = 0; // RHF NEW 19/10/98


            len = to - Labtbase;
            // RHF INIC 21/4/99
            if( len > MAXUSHORT )
                len = MAXUSHORT;
            // RHF END 21/4/99

            *( (POSLAB_TYPE *) Labtbase ) = static_cast<TCHAR>(len);
        }

        break;

    case SymbolType::Section:
        const CDictRecord* pRecord;
        SECT    *sp;
        csprochar    slabel[_LLABEL+1];

        sp = SPT(isym);
        pRecord = sp->GetDictRecord();

        strcpymax( slabel, pRecord->GetLabel().GetString(), _LLABEL );
        somelab = _tcslen(slabel);
        if( somelab ) {
            to = putlabel( to, slabel );
            // RHF COM 19/10/98 *( to++ ) = '\n'; *to = '\0';
            *( to++ ) = _T('\n');*( to++ ) = _T('\n');*( to++ ) = 0; // RHF NEW 19/10/98

            len = to - Labtbase;
            // RHF INIC 21/4/99
            if( len > MAXUSHORT )
                len = MAXUSHORT;
            // RHF END 21/4/99

            *( (POSLAB_TYPE *) Labtbase ) = static_cast<TCHAR>(len);
        }

        break;

    case SymbolType::Variable:
        csprochar vlabel[_LLABEL+1], vval[_LVALLAB+1], vlab[_LLABLAB+1];

        const VART* vp = VPT(isym);
        const CDictItem* pItem = vp->GetDictItem();

        strcpymax( vlabel, pItem->GetLabel().GetString(), _LLABEL );
        trimall( vlabel );
        if( *vlabel ) {
            to = putlabel( to, vlabel );
            somelab = TRUE;
        }

        // Se usa solamente el 1er value-set
        if( pItem->HasValueSets() ) {
            const DictValueSet& dict_value_set = pItem->GetValueSet(0);
            int nvalues = 0;

            for( const auto& dict_value : dict_value_set.GetValues() ) {
                if( !dict_value.HasValuePairs() )
                    continue;

                // Se usa solamente el 1er par
                const auto& dict_value_pair = dict_value.GetValuePair(0);

                if( dict_value_pair.GetTo().GetLength() > 0 ) // Se ignora
                    continue;

                strcpymax( vval, dict_value_pair.GetFrom().GetString(), _LVALLAB );
                strcpymax( vlab, dict_value.GetLabel().GetString(), _LLABLAB );

                trimall( vval ); trimall( vlab );
                if( _tcslen(vval) == 0 || _tcslen(vlab) == 0 )
                    continue;

                to = putlabel( to, vval );
                to = putlabel( to, vlab );
                somelab = TRUE;

                if( ++nvalues >= N_VALLAB ) // Solo hasta N_VALLAB(1000)
                    break;
            }
        }

        if( somelab ) {
            // RHF COM 19/10/98 *( to++ ) = '\n'; *to = '\0';
            *( to++ ) = _T('\n');*( to++ ) = _T('\n');*( to++ ) = 0; // RHF NEW 19/10/98

            //RHF COM 04/01/2000 len = to - Labtbase;
            len = to - Labtbase - 2; // RHF 04/01/2000
            // RHF INIC 21/4/99
            if( len > MAXUSHORT )
                len = MAXUSHORT;
            // RHF END 21/4/99

            *( (POSLAB_TYPE *) Labtbase ) = static_cast<TCHAR>(len);
        }

        break;
      }

      l_size = to - Labtbase - POSLAB;
      if( l_size <= 0 ) {
          label_data.clear();
      }
      else {
          label_data.resize(l_size);
      }

      // Nunca hay truncacion pues Labtmxent es grande
      return label_data;
}



/*------------------------------------------------------------------------*/
/* putlabel : pass a label to label area                                  */
/*------------------------------------------------------------------------*/
void strcvt2( csprochar *out, csprochar *in, int convert )
  {
    ASSERT(convert == 16);
    int     len, lenin, allocated=0;
    csprochar    *p, *pbuf;
    const int strcvt2_max_len = 1024;
    csprochar    buf[strcvt2_max_len+1];

    lenin = _tcslen( in );
    if( lenin >= strcvt2_max_len )
      {
        allocated = 1;
        if( ( pbuf = (csprochar *) calloc( lenin + 1, 1 ) ) == NULL )
            return;
      }
    else
        pbuf = buf;

    if( convert == 16 ) /* Remove trailing BLANKS */
      {
        _tcscpy( pbuf, in );

        p = pbuf + lenin - 1;
        while( p >= pbuf && *p == ' ' )
            p--;

        len = p - pbuf + 1;
        if( len > 0 )
            _tmemcpy( out, pbuf, len );
        *( out + len ) = 0;
      }

    if( allocated )
        free( pbuf );
  }


csprochar *putlabel( csprochar *to, csprochar *from )
  {
    strcvt2( to, from, 16 );
    to += _tcslen( to );
    *to = _T('\n');

    return( to + 1 );
  }
