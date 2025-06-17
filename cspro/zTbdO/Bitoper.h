#pragma once

//---------------------------------------------------------------------------
//  File name: BitOper.h
//
//  Description:
//          Header for CBitOper class
//          This class define a bitmap operation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              28 Jul 01   DVB     Created
//              09 Oct 02   RHF     Add more methods
//
//---------------------------------------------------------------------------

/*******************************************************
BitMaskSet:      bit           BitMaskUnSet:      bit

128    10000000    0            127    01111111    0
64     01000000    1            191    10111111    1
32     00100000    2            223    11011111    2
16     00010000    3            239    11101111    3
8      00001000    4            247    11110111    4
4      00000100    5            251    11111011    5
2      00000010    6            253    11111101    6
1      00000001    7            254    11111110    7
*******************************************************/
static _TUCHAR BitMaskSet[]   = { 128,  64,  32,  16,   8,   4,   2,   1 };
static _TUCHAR BitMaskUnSet[] = { 127, 191, 223, 239, 247, 251, 253, 254 };


class CBitoper
{
private:
    long m_lNumBits;
    std::vector<_TUCHAR> m_cArea;

public:
    CBitoper(long lNumBits)
        :   m_lNumBits(lNumBits)
    {
        long lLenArea = lNumBits / 8 + ( (lNumBits % 8) ? 1 : 0 );
        m_cArea.resize(lLenArea, 0);
    }

    void BitPut(long lBitNum, bool bValue)
    {
        if( lBitNum >= m_lNumBits ) // RHF Sep 24, 2002 Replace > by >=
            return;

        long byte_number = lBitNum / 8;
        short bit_number = (short)(lBitNum % 8);

        _TUCHAR& c = m_cArea[ byte_number ];

        if( bValue ) {
            c |= BitMaskSet[ bit_number ];
        }
        else {
            c &= BitMaskUnSet[ bit_number ];
        }
    }

    bool BitGet(long lBitNum)
    {
        if( lBitNum >= m_lNumBits )
            return false;

        long byte_number = lBitNum / 8;
        short bit_number = (short)(lBitNum % 8);

        _TUCHAR c = m_cArea[ byte_number ];

        c &= BitMaskSet[ bit_number ];

        return ( c != 0 );
    }
};
