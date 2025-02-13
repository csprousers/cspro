#include "MobileStringConversion.h"
#include "PortableMFC.h"
#include "wchar.h"
#include <stdlib.h>
#include <string>


// --------------------------- UTF-8 -> WIDE ---------------------------

// modified from http://gears.googlecode.com/svn/trunk/third_party/convert_utf/ConvertUTF.h and http://gears.googlecode.com/svn/trunk/third_party/convert_utf/ConvertUTF.c

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const unsigned long offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
             0x03C82080UL, 0xFA082080UL, 0x82082080UL };

#define UNI_MAX_BMP             (unsigned long)0xFFFF
#define UNI_MAX_UTF16           (unsigned long)0x10FFFF
#define UNI_SUR_HIGH_START      (unsigned long)0xD800
#define UNI_SUR_HIGH_END        (unsigned long)0xDBFF
#define UNI_SUR_LOW_START       (unsigned long)0xDC00
#define UNI_SUR_LOW_END         (unsigned long)0xDFFF
#define UNI_REPLACEMENT_CHAR    (unsigned long)0xFFFD

static const int halfShift  = 10;
static const unsigned long halfBase = 0x010000;
static const unsigned long halfMask = 0x03FF;


int UTF8ToWide_Worker(const char* paBuffer,int iaLength,wchar_t* pwBuffer,int iwBufferSize,std::wstring* pwstr) // based on ConvertUTF8toUTF16
{
    bool bBufferVersion = ( pwBuffer != NULL );

    const unsigned char* pSource = (unsigned char*)paBuffer;
    int iOutputCounter = 0;

    for( int i = 0; i < iaLength; i++ )
    {
        char extraBytesToRead = trailingBytesForUTF8[*pSource];

        if( i + extraBytesToRead >= iaLength ) // an error ... so we'll ignore this character
            break;

        unsigned long ch = 0;

        switch( extraBytesToRead )
        {
            case 5: ch += *pSource++; ch <<= 6;
            case 4: ch += *pSource++; ch <<= 6;
            case 3: ch += *pSource++; ch <<= 6;
            case 2: ch += *pSource++; ch <<= 6;
            case 1: ch += *pSource++; ch <<= 6;
            case 0: ch += *pSource++;
        }

        ch -= offsetsFromUTF8[extraBytesToRead];

        // to handle surrogates...
        if( ch <= UNI_MAX_BMP )
        {
            if( ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END )
                ch = UNI_REPLACEMENT_CHAR;
        }

        else if( ch > UNI_MAX_UTF16 )
            ch = UNI_REPLACEMENT_CHAR;

        else if( i + 1 < iaLength )
        {
            ch -= halfBase;

            unsigned long ch1 = (unsigned long)( ( ch >> halfShift ) + UNI_SUR_HIGH_START );
            unsigned long ch2 = (unsigned long)( ( ch & halfMask ) + UNI_SUR_LOW_START );

            if( bBufferVersion )
            {
                if( iOutputCounter >= iwBufferSize )
                    return iOutputCounter;

                pwBuffer[iOutputCounter] = (wchar_t)ch1;
                iOutputCounter++;
            }

            else
                pwstr->append(1,(wchar_t)ch1);

            ch = ch2;
        }


        if( bBufferVersion )
        {
            if( iOutputCounter >= iwBufferSize )
                return iOutputCounter;

            pwBuffer[iOutputCounter] = (wchar_t)ch;
            iOutputCounter++;
        }

        else
            pwstr->append(1,(wchar_t)ch);


        i += extraBytesToRead;
    }

    return iOutputCounter;
}

int UTF8BufferToWideBufferAndroid(const char* paBuffer,int iaLength,wchar_t* pwBuffer,int iwBufferSize)
{
    return UTF8ToWide_Worker(paBuffer,iaLength,pwBuffer,iwBufferSize,NULL);
}

std::wstring UTF8ToWideAndroid(const char* pUTF8String,int iStringLen)
{
    if (pUTF8String == NULL)
        return std::wstring();

    if( iStringLen < 0 )
        iStringLen = strlen(pUTF8String);

    std::wstring wstr = L"";
    wstr.reserve(iStringLen);

    UTF8ToWide_Worker(pUTF8String,iStringLen,NULL,0,&wstr);

    return wstr;
}



// --------------------------- WIDE -> UTF-8 ---------------------------

static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };


int WideToUTF8_Worker(const wchar_t* pwBuffer,int iwLength,char* paBuffer,int iaBufferSize,std::string* pstr) // based on ConvertUTF8toUTF16
{
    bool bBufferVersion = ( paBuffer != NULL );

    const wchar_t* pSource = pwBuffer;
    const wchar_t* pSourceEnd = pSource + iwLength;
    int iOutputCounter = 0;

    while( pSource < pSourceEnd )
    {
        unsigned long ch;
        unsigned short bytesToWrite = 0;

        const wchar_t byteMask = 0xBF;
        const wchar_t byteMark = 0x80;

        ch = (unsigned long)*pSource++;

        // to handle surrogates...
        if( ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END )
        {
            if( pSource < pSourceEnd )
            {
                unsigned long ch2 = *pSource;

                // if it's a low surrogate, convert to UTF32
                if( ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END )
                {
                    ch = ( ( ch - UNI_SUR_HIGH_START ) << halfShift ) + ( ch2 - UNI_SUR_LOW_START ) + halfBase;
                    pSource++;
                }
            }
        }

        if( ch < (unsigned long)0x80 )
            bytesToWrite = 1;

        else if( ch < (unsigned long)0x800 )
            bytesToWrite = 2;

        else if( ch < (unsigned long)0x10000 )
            bytesToWrite = 3;

        else if( ch < (unsigned long)0x110000 )
            bytesToWrite = 4;

        else
        {
            bytesToWrite = 3;
            ch = UNI_REPLACEMENT_CHAR;
        }

        if( bBufferVersion )
        {
            if( ( iOutputCounter + bytesToWrite ) >= iaBufferSize )
                return iOutputCounter;

            iOutputCounter += bytesToWrite;
            int iTempPos = iOutputCounter;

            switch( bytesToWrite )
            {
                case 4: paBuffer[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 3: paBuffer[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 2: paBuffer[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 1: paBuffer[--iTempPos] = (unsigned char)( ch | firstByteMark[bytesToWrite] );
            }
        }

        else
        {
            pstr->append(bytesToWrite,' ');
            int iTempPos = pstr->length();

            switch( bytesToWrite )
            {
                case 4: (*pstr)[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 3: (*pstr)[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 2: (*pstr)[--iTempPos] = (unsigned char)( ( ch | byteMark) & byteMask ); ch >>= 6;
                case 1: (*pstr)[--iTempPos] = (unsigned char)( ch | firstByteMark[bytesToWrite] );
            }
        }
    }

    return iOutputCounter;
}

int WideBufferToUTF8BufferAndroid(const wchar_t* pwBuffer,int iwLength,char* paBuffer,int iaBufferSize)
{
    return WideToUTF8_Worker(pwBuffer,iwLength,paBuffer,iaBufferSize,NULL);
}

std::string WideToUTF8Android(const wchar_t* pWideString,int iStringLen)
{
    if( iStringLen < 0 )
        iStringLen = wcslen(pWideString);

    std::string str = "";
    str.reserve(iStringLen);

    WideToUTF8_Worker(pWideString,iStringLen,NULL,0,&str);

    return str;
}


std::wstring TwoByteCharToWide(const uint16_t* text, size_t length/* = SIZE_MAX*/)
{
    ASSERT(text != nullptr);

    if( length == SIZE_MAX )
    {
        const uint16_t* text_itr = text;

        while( *text_itr != 0 )
            ++text_itr;

        length = text_itr - text;
    }

    std::wstring wide_text(length, '\0');
    wchar_t* wide_text_itr = wide_text.data();

    const uint16_t* text_itr = text;
    const uint16_t* text_end = text + length;

    while( text_itr != text_end )
    {
        if( *text == 0 )
        {
            wide_text.resize(text_itr - text);
            break;
        }

        *wide_text_itr = *text_itr;
        ++text_itr;
        ++wide_text_itr;
    }

    return wide_text;
}


int _wtoi(const wchar_t* pwStrNumbers)
{
    return (int) wcstol (pwStrNumbers, NULL, 10);
}


long _wtol(const wchar_t* pwStrNumbers)
{
    return wcstol(pwStrNumbers, NULL, 10);
}


double _wtof(const wchar_t* pwStrNumbers)
{
    return atof(WideToUTF8Android(pwStrNumbers).c_str());
}
