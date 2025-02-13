#include "StandardSystemIncludes.h"
#include "RangeFunctions.h"

const double SMALL_NUMBER = 1e-5;

// use double options
bool areEqual( double a, double b )
{
    double dDiff = a - b;
    if( dDiff < 0 )
        dDiff = -dDiff;
    return (double) dDiff < SMALL_NUMBER;
}

bool areDifferent( double a, double b )
{
    return !areEqual(a,b);
}

bool lowerOrEqual( double a, double b ) // a <= b?
{
    double dDiff = a - b;
    return areEqual(a,b) || (dDiff < 0);
}

bool greaterOrEqual( double a, double b ) // a >= b?
{
    double dDiff = a - b;
    return areEqual(a,b) || (dDiff > 0);
}

bool isSmaller( double a, double b ) // a < b?
{
    double dDiff = a - b;
    return (dDiff < 0) && areDifferent(a,b);
}

bool isBigger( double a, double b ) // a > b?
{
    double dDiff = a - b;
    return (dDiff > 0) && areDifferent(a,b);
}
