#pragma once

/*  CTAB: masks for environment */
#define ct_BREAK           1    /* Reacts to break by                      */
#define ct_AUTOTALLY       2    /* Automatic tally                         */
#define ct_PRINT           4    /* Generates total printout file .TBL      */
#define ct_FREQ            8    /* Frequencies */


/*  CTAB: masks for options */
const int ct_MISSING   =      1;  // Reserves place for special values
const int ct_DEFAULT   = (1<< 1);
const int ct_NOTAPPL   = (1<< 2);
const int ct_REFUSED   = (1<< 3);
const int ct_UNDEFINED = (1<< 4);
const int ct_ROWZERO   = (1<< 5); // Print empty rows
const int ct_COLZERO   = (1<< 6); //             columns
const int ct_LAYZERO   = (1<< 7); //             layers
const int ct_ROWTOT    = (1<< 8); // Automatic totalling row
const int ct_COLTOT    = (1<< 9); //                     column
const int ct_LAYTOT    = (1<<10); //                     layer
const int ct_ROWPCT    = (1<<11); // Percents over totalling row
const int ct_COLPCT    = (1<<12); //                         column
const int ct_LAYPCT    = (1<<13); //                         layer
const int ct_TOTPCT    = (1<<14); //                         general total
const int ct_CHISQUARE = (1<<15); // Chisquare
const int ct_NO_SPECIAL_VALUE = (1<<16);  // if user specifies 'include' then
                                           // if user does not specify explicit special values
                                           // then no one will be included
                                           // rcl, Aug 2005

const int ct_ALL_SPECIAL_VALUES = (ct_MISSING|ct_DEFAULT|ct_NOTAPPL|ct_REFUSED|ct_UNDEFINED);
const int ct_NOFLAGS = 0;

#define CT_MAXDIM                       3
#define CT_ROW                          0
#define CT_COL                          1
#define CT_LAY                          2
#define CT_BREAKDIM                     3
