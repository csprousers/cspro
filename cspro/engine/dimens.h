#pragma once

//---------------------------------------------------------------------------
//  File name: Dimens.h
//
//  Description:
//          Header for dimension-class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Jul 00   RHF     Creation
//
//
//---------------------------------------------------------------------------

#ifndef DIM_MAXDIM_CONSTANT
#define DIM_MAXDIM_CONSTANT
constexpr int DIM_MAXDIM = 3;
#endif//DIM_MAXDIM_CONSTANT

#define DIM_NODIM     -1

#define DIM_SOMEDIM   0
#define DIM_ROW       1
#define DIM_COL       2
#define DIM_LAYER     3

#define DIM_SOMEDEPTH  0
#define DIM_LEFTDEPTH  1
#define DIM_RIGHTDEPTH 2

class CDimension {
    // - VDimType                                       // victor Jul 10, 00
    //   ... this info and is loaded (see DictLoad.cpp) into the array
    //       'm_aDimType' for each VART entry, and helps both compiler
    //       and executors:
    //
    //        - at compiling time: allows for checking fully-subindexed Vars
    //        - at execution time: lets the engine to pass the indexes of
    //              compiled expressions to the proper parent
    //
    //       Under current restriction of 3 dimensions, valid contents for
    //       the 'm_aDimType' array are as follows:
    //
    //        - for 3 dimensions:
    //              Record, Item, SubItem               3
    //        - for 2 dimensions:
    //              Record, Item, VoidDim               2
    //              Record, SubItem, VoidDim            3
    //              Item, SubItem, VoidDim              3
    //        - for 1 dimension:
    //              Record, VoidDim, VoidDim            1
    //              Item, VoidDim, VoidDim              2
    //              SubItem, VoidDim, VoidDim           3
    //        - for 0 dimensions:
    //              VoidDim, VoidDim, VoidDim           1, 2, 3???
    //
    //   ... each one of the dimensions refers to:
public:
    enum    VDimType {                   // --> PLEASE... DON'T CHANGE THESE VALUES!!!
                VoidDim = -1,           // void - can't have any reference
                Record  = 0,            // the Record
                Item    = 1,            // the Item
                SubItem = 2             // the Sub-item
    };
};
