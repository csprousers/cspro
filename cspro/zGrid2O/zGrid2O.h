#pragma once

// zGrid2O.h
// for CSPro 2.1
// chris


#ifdef ZGRID2O_IMPL
    #define CLASS_DECL_ZGRID2O __declspec(dllexport)
#else
    #define CLASS_DECL_ZGRID2O __declspec(dllimport)
#endif


const int GRIDSEP_SIZE = 1;

enum GridObjPosition { posNone, posBottomRight, posTopRight, posCenterRight, posBottomLeft, posTopLeft, posCenterLeft, posAlignRight, posAlignLeft };

// default colors for cells, selected cells, headers, stubs, cell lines, etc.
constexpr COLORREF rgbBSelDefault = rgbGray;
constexpr COLORREF rgbFSelDefault = rgbWhite;
constexpr COLORREF rgbFCellDefault = rgbBlack;
constexpr COLORREF rgbFFieldDefault = rgbBlack;
constexpr COLORREF rgbBFieldDefault = rgbWhite;
constexpr COLORREF rgbFHeaderDefault = rgbBlack;
constexpr COLORREF rgbFStubDefault = rgbBlack;
constexpr COLORREF rgbCellLines = rgbBlack;
