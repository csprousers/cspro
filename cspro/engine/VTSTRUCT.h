#pragma once

#include <engine/dimens.h>

const int MAX_TALLYCELLS_V = 10100; // local definition for vector

class VTSTRUCT
{
public:
    int         ctvector[DIM_MAXDIM][MAX_TALLYCELLS_V+1];
    bool        bUse4Processing;
    int         iUseDim;
    bool        bUseV1;
    int         iUseIndex;

    int         iV1Index;
    int         iV2Index;

    std::vector<std::shared_ptr<std::vector<int>>> arrV1s;
    std::vector<std::shared_ptr<std::vector<int>>> arrV2s;
    //due to the limitation of vector<bool> ...http://stackoverflow.com/questions/670308/alternative-to-vectorbool
    std::vector<unsigned char> arrbUseV1;
    CSubTable*  pSubTable;
    //For computation
    bool   bKind[4];
    int    iNumSubCells[TBD_MAXDIM];//={1,1,1};
    int iReUseIndex;
public:
    void Init(){
        iUseIndex = iV1Index = iV2Index = -1;
        iReUseIndex = -1;
        bKind[0] =false;bKind[1] =false;bKind[2] =false;bKind[3] =false;
        iNumSubCells[0] = 1; iNumSubCells[1] =1;iNumSubCells[2]= 1;
        for(int i = 0; i < DIM_MAXDIM; i++ ) {
            ctvector[i][0] = 1;
            ctvector[i][1] = 0;
        }
    }
    void AddUseFlag(bool bFlag) {
        iUseIndex++;
        if( iUseIndex >= (int)arrbUseV1.size() ) {
            arrbUseV1.resize(iUseIndex + 1);
        }
        arrbUseV1[iUseIndex] = bFlag ? 1 : 0;
    }
    void RemoveUseFlag(){iUseIndex--;}

    void AddV1Arr(std::shared_ptr<std::vector<int>> pArrV1) {
        iV1Index++;
        if( iV1Index >= (int)arrV1s.size() ) {
            arrV1s.resize(iV1Index + 1);
        }
        arrV1s[iV1Index] = std::move(pArrV1);
    }
    void RemoveArrV1() {
        arrV1s[iV1Index].reset();
        iV1Index--;
    }

    void AddV2Arr(std::shared_ptr<std::vector<int>> pArrV2) {
        iV2Index++;
        if( iV2Index >= (int)arrV2s.size() ) {
            arrV2s.resize(iV2Index + 1);
        }
        arrV2s[iV2Index] = std::move(pArrV2);
    }
    void RemoveArrV2() {
        arrV2s[iV2Index].reset();
        iV2Index--;
    }
};
