#pragma once

#define GPS_INTERVAL_DIALOG_CHECK 50

struct GPSInfo
{
    double time;
    double latitude;
    double longitude;
    double satellites;
    double HDOP;
    double altitude;
};

struct GPSThreadInfo
{
    HANDLE hGPS;
    GPSInfo * gpsInfo;
    bool successfulRead;
    bool cancelRead;
    CString windowText;
    int numReadIntervals;
    bool readlastMode;
};
