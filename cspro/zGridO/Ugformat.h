#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCellFormat
    Purpose
        This class is used for the formating the
        cells data for display and/or for editing.
    Details
************************************************/

class CLASS_DECL_ZGRIDO CUGCellFormat: public CObject{

public:
    CUGCellFormat();
    virtual ~CUGCellFormat();

    virtual void ApplyDisplayFormat(CUGCell *cell);

    virtual int  ValidateCellInfo(CUGCell *cell);
};
