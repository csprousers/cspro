/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  VARFUNCS.cpp                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "VarFuncs.h"
#include "Special.h"
#include <sstream>
#include "math.h"

static double chgetdec (csprochar *p, int mindig);

#include <ctype.h>
#define is_digit _istdigit

#define BLANK ' '

void dvaltochar_original( double value, csprochar *buf, int len, int dec, bool bLeadingZeros, bool bExplicitDecimals )
{
    if( len == 1 )
    {
        if( value == MASKBLK ) {
            *buf = BLANK;
        }
        else if( value < 0 || value > 9.5 ) {
            *buf = '*';
        }
        else {
            *buf = static_cast<csprochar>(value + 0.5) + '0';
        }
        return;
    }

    // RHF INIC May 19, 2000
    if( bLeadingZeros == true && value != MASKBLK ) {
        _tmemset( buf, '0', len );
    }
    else {
        _tmemset( buf, BLANK, len );
    }

    if( value == MASKBLK )
        return;
    // RHF END May 19, 2000

    bool is_negative;

    if( !IsSpecial(value) )
    {
        is_negative = value < 0; // ( value >= 0 ) ? 0 : 1; // RCH
        if( is_negative )
            value = -value;
        if( dec > 0 )
            value *= Power10[dec];
    }
    else
    {
        _tmemset( buf, '*', len );
        return;
    }

    csprochar *pbuf;
    if( value < 0.5 )
    {
        pbuf = buf + len - 1;
        if( dec > 0 )
        {
            for( int i = 0; i < dec; i++ )
                *pbuf-- = '0';
            if( bExplicitDecimals ) // RCH
                *pbuf-- = '.';
            if( ( len - dec > 1 ) || ( !bExplicitDecimals && len - dec == 1 ) ) // GHM 20120312 added second clause so that DecChar=No fields zero fields wouldn't look like: ,00
                //*(--pbuf) = '0';
                *pbuf-- = '0'; // RCH
        }
        else
        {
            *pbuf = '0';
        }

        return;
    }

    if( len <= dec || ( len - dec == 1 && is_negative ) )
    {
        _tmemset( buf, '*', len );
        return;
    }

    value = floor( value + MAGICROUND );

    int l = ( dec > 0 ? len - 1 : len );
    if( !bExplicitDecimals ) // RCH
        l++;

    if( ( !is_negative && value >= Power10[l] ) ||
        ( is_negative  && value >= Power10[l - 1] ) )
    {
        _tmemset( buf, '*', len );
        return;
    }

    int dpos;
    int sign;

    //csprochar *p = _ecvt( value, len, &dpos, &sign );
#ifdef _MFC_VER
    //Savy for unicode http://msdn.microsoft.com/en-us/library/87zae4a3%28v=VS.100%29.aspx
     CA2T p(_ecvt( value, len, &dpos, &sign ));
#else
        //For android port
           /*http://man7.org/linux/man-pages/man3/ecvt.3.html
            * The ecvt() function converts number to a null-terminated string of
           ndigits digits (where ndigits is reduced to a system-specific limit
           determined by the precision of a double), and returns a pointer to
           the string.  The high-order digit is nonzero, unless number is zero.
           The low order digit is rounded.  The string itself does not contain a
           decimal point; however, the position of the decimal point relative to
           the start of the string is stored in *decpt.  A negative value for
           *decpt means that the decimal point is to the left of the start of
           the string.  If the sign of number is negative, *sign is set to a
           nonzero value, otherwise it is set to 0.  If number is zero, it is
           unspecified whether *decpt is 0 or 1.*/
         std::ostringstream oss;
         oss.precision(len);
         oss.setf(std::ostringstream::showpoint);
         oss << value;
         std::string s =  oss.str();
        std::wstring wstr(s.begin(),s.end());
        dpos = wstr.find('.');
        sign = wstr.find('-');
        wstr.erase(dpos,1);//remove the decimal from the string to emulate ecvt;
        value == 0? dpos =0 : dpos =dpos;
        sign >= 0 ? sign = -1 : sign =0; //If the sign of number is negative, *sign is set to a
           //nonzero value, otherwise it is set to 0
        csprochar *p = const_cast<csprochar*>(wstr.c_str());
#endif

    if( dpos > len || ( is_negative && dpos == len ) )
    {
        _tmemset( buf, '*', len );
        return;
    }

    // Copy from right to left
    csprochar *pdig = p + dpos - 1;
    pbuf = buf + len - 1;

    if( dec > 0 )
    {
        for( int i = 0; i < dec; i++ )
            if( dpos-- > 0 )
                *pbuf-- = *pdig--;
            else
                *pbuf-- = '0';
            if( bExplicitDecimals )
                *pbuf-- = '.';
    }

    if( dpos > 0 ) {
        for( ; dpos > 0; dpos-- )
            *pbuf-- = *pdig--;
    }
    else {
        *pbuf-- = '0';
    }

    if( is_negative ) {
        if( dec > 0 && len - dec == 2 ) { /* no space for 0 before "." */
            *(++pbuf) = '-';
        }
        else if( bLeadingZeros ) {
            buf[0] = '-';
        }
        else {
            *pbuf = '-';
        }
    }
}


double chartodval_original( const csprochar *buf, int len, int dec )
{
    double  val, valdec;

    if( *buf == 0 )return MASKBLK;   // RHF Jul 27, 2000

    if( len == 1 )
    {
        if( *buf == BLANK )
            return( MASKBLK );
        if( is_digit( *buf ) )
            return( (double) ( *buf - '0' ) );
        return( DEFAULT );
    }

    if( len == 2 )
    {
        csprochar c1 = *buf;  csprochar c2 = *(buf+1);

        if( is_digit( c1 ) )
        {
            if( is_digit( c2 ) )
            {
                val = (double) ( ( c1 - '0' ) * 10 + ( c2 - '0' ) );
                if( dec == 1 )
                    return( val / 10.0 );
                else
                    return( val );
            }

            if( c2 == BLANK )
                return( (double) ( c1 - '0' ) );

            if( c2 == '.'  || c2 == ',')
                return( (double) ( c1 - '0' ) );
        }

        if( c1 == BLANK )
        {
            if( is_digit( c2 ) )
                return( (double) ( c2 - '0' ) );
            if( c2 == BLANK )
                return( MASKBLK );
        }

        if( c1 == '-' && is_digit( c2 ) )
            return( - ( (double) ( c2 - '0' ) ) );

        if( (c1 == '.' || c1 == ',')&& is_digit( c2 ) )
            return( ( c2 - '0' ) / 10.0 );

        return( DEFAULT );
    }

    csprochar number[256];
    int    l = len;
    csprochar  *p = number;
    int sign = 1;

    // RHF INIC 13/12/99
    // tostr: builds an output string from a source (excluding BLANKS)
    auto tostr = []( csprochar* pszTo, const csprochar* pFrom, int iLen, bool bTrimBlanks ) -> csprochar*
    {
        csprochar*   pszToAux=pszTo;

        if( pFrom ) {
            csprochar c;

            for( int i = 0; i < iLen && ( c = *pFrom) != 0; pFrom++, i++ ) {
                if( bTrimBlanks && c <= BLANK )
                    continue;
                *pszToAux++ = c;
            }
        }
        *pszToAux = 0;

        return pszTo;
    };


    tostr( number, buf, len, false ); // RHF Mar 01, 2000 Add false

    // Do not consider leading BLANKS
    while( *p == BLANK )
    {
        l--;
        p++;
    }

    if( *p == '\0' )
        return( MASKBLK );

    // RCH 29/07/1999
    // Accept both '-' and '+'
    // Change sign only if '-' is used
    if( *p == '-' || *p == '+' )
    {
        if( *p == '-' )
            sign = -1;
        p++;
        l--;
    }

    // Do not consider leading BLANKS after sign // RCH 29/07/1999
    while( *p == BLANK )
    {
        l--;
        p++;
    }

    if( *p == '.' || *p == ',' )
    {
        p++;
        l--;
        val = chgetdec( p, 1 );
        return( !IsSpecial(val) ? sign * val : val );
    }

    if( is_digit( *p ) )
    {
        val = 0.0;

        while( is_digit( *p ) )
        {
            val = val * 10 + ( *p++ - '0' );
            l--;
        }

        if( *p == '\0' )
        {
            if( l > dec ) // && dec > 1, not to divide by 1
                return( sign * val );
            else
                return( sign * val / Power10[dec - l] );
        }

        if( *p == '.' || *p == ',' )
        {
            p++;
            valdec = chgetdec( p, 0 );
            if( !IsSpecial(valdec) )
                return( sign * ( val + valdec ) );
            else
                return( valdec );
        }

        return( DEFAULT );
    }

    return( DEFAULT );
  }


  static double chgetdec( csprochar *p, int mindig )
  {
      double  val;
      int     l;

      val = 0;
      l = 0;

      while( is_digit( *p ) && ( l <= MAX_NUMLEN ) ) // GHM 20120405 added MAX_NUMLEN clause because values with really long decimals were getting evaluated to invalid doubles
      {
          l++;
          val = val +  ( *p++ - '0' ) / Power10[l];
      }

      if( ( *p == '\0' && l >= mindig ) || ( l > MAX_NUMLEN ) )
          return( val );

      return( DEFAULT );
  }


  /*------------------------------------------------------------------------*/
  /*  hard_bounds : provides minimum and maximum values for len/dec         */
  /*------------------------------------------------------------------------*/
  bool hard_bounds( int len, int dec, double *min_bound, double *max_bound )
  {
      double  lowest_dec;

      // bound values initialized to impossible!
      *min_bound = 0;
      *max_bound = *min_bound - 1;

      if( len <= 0 || ( dec > 0 && len <= dec + 1 ) )
          return false;

      // big lengths: no values allowed!
      if( len > ( MAX_NUMLEN + ( dec != 0 ) ) ) // GHM 20140520 added a condition so it's MAX_NUMLEN + 1 when there's a decimal value
          return true;

      if( dec <= 0 )
      {
          *max_bound = Power10[len] - Power10[0];
          if( len == 1 )
              *min_bound = 0;
          else
              *min_bound = -( Power10[len - 1] - Power10[0] );
      }
      else
      {
          lowest_dec = Power10[0] / Power10[dec];
          *max_bound = Power10[len - (dec + 1)] - lowest_dec;
          if( len - (dec + 1) <= 1 )
              *min_bound = 0;
          else
              *min_bound = -( Power10[len - (dec + 1) - 1] - lowest_dec );
      }

      return true;
  }



  // CR_TODO get rid of the above functions ... should be able to if none of the asserts below get hit
#include "NumberConverter.h"

void dvaltochar( double value, csprochar *buf, int len, int dec, bool bLeadingZeros, bool bExplicitDecimals )
{
    dvaltochar_original(value, buf, len, dec, bLeadingZeros, bExplicitDecimals);

#ifdef _DEBUG

    // test against the new routines
    double no_mask_val = ( value == MASKBLK ) ? NOTAPPL : value;
    CString cstring_temp_buffer;
    TCHAR* temp_buffer = cstring_temp_buffer.GetBufferSetLength(len);

    if( len <= 9 && dec == 0 )
    {
        NumberConverter::IntegerDoubleToText<int>(no_mask_val, temp_buffer, len, bLeadingZeros);
        ASSERT(_tmemcmp(buf, temp_buffer, len) == 0);
    }

    if( dec == 0 )
    {
        NumberConverter::IntegerDoubleToText<int64_t>(no_mask_val, temp_buffer, len, bLeadingZeros);
        ASSERT(_tmemcmp(buf, temp_buffer, len) == 0);
    }

    {
        NumberConverter::DoubleToText(no_mask_val, temp_buffer, len, dec, bLeadingZeros, bExplicitDecimals);
        ASSERT(_tmemcmp(buf, temp_buffer, len) == 0);
        // if only wanting to test everything up to the last character of the fractional part, comment above and uncomment below
        //ASSERT(_tmemcmp(buf, temp_buffer, len - ( ( dec > 0 ) ? 1 : 0 )) == 0);
    }
	
#endif
}


std::wstring dvaltochar(double dValue, int iLen, int iDec, bool bLeadingZeros/* = false*/, bool bExplicitDecimals/* = true*/)
{
    std::wstring text(iLen, '\0');
    dvaltochar(dValue, text.data(), iLen, iDec, bLeadingZeros, bExplicitDecimals);
    ASSERT80(_tcslen(text.c_str()) == static_cast<size_t>(iLen));
    return text;
}


double chartodval( const csprochar *buf, int len, int dec )
{
    double value = chartodval_original(buf, len, dec);

#ifdef _DEBUG

    // test against the new routines
    double no_mask_val = ( value == MASKBLK ) ? NOTAPPL : value;

    if( len <= 9 && dec == 0 )
    {
        double test_value = NumberConverter::TextToIntegerDouble<int>(buf, len);
        ASSERT(test_value == no_mask_val);
    }

    if( dec == 0 )
    {
        double test_value = NumberConverter::TextToIntegerDouble<int64_t>(buf, len);
        ASSERT(test_value == no_mask_val);
    }

    {
        double test_value = NumberConverter::TextToDouble(buf, len, dec);
        double diff = test_value - no_mask_val;
        ASSERT(fabs(diff) < 0.00000001);
    }

#endif

    return value;
}
