#pragma once

#if defined(USE_BINARY) // IGNORE_CTAB

#else

typedef enum { CTSTAT_NONE=0,
               CTSTAT_OVERLAPPED,  // Intersection of stats

               // Valid Stats
               CTSTAT_FREQ,
               CTSTAT_TOTAL,
               CTSTAT_PERCENT,
               CTSTAT_PROP,
               CTSTAT_MIN,
               CTSTAT_MAX,
               CTSTAT_MODE,
               CTSTAT_MEAN,
               CTSTAT_MEDIAN,
               CTSTAT_PTILE,
               CTSTAT_STDEV,
               CTSTAT_VARIANCE,
               CTSTAT_STDERR,
               CTSTAT_VPCT
             } CStatType;

#endif
