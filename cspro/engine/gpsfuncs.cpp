#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Nodes/GPS.h>
#include <zPlatformO/PlatformInterface.h>
#include <zMapping/DefaultBaseMapEvaluator.h>
#include <zMapping/GreatCircle.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>


#ifdef WIN_DESKTOP // 20110524 adding GPS functionality for desktops

#include "GPSThreadInfo.h"


LPSTR GetNextToken(LPSTR lpSentence,LPSTR lpToken)
{
    lpToken[0] = '\0';

    if( lpSentence == NULL || lpSentence[0] == '\0' ) // empty sentence
        return NULL;

    if( lpSentence[0] == ',' ) // empty token
        return lpSentence + 1;

    while( *lpSentence != ',' && *lpSentence != '\0' && *lpSentence != '*' )
    {
        *lpToken = *lpSentence;
        lpToken++;
        lpSentence++;
    }

    lpSentence++;  // skip over comma
    *lpToken = '\0';
    return lpSentence;
}


// Parses a GGA sentence which has the format:
// $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
bool ParseGGA(char* szSentence, GPSInfo& gps)
{
    char szToken[30];
    int calculatedCheckSum = 0;

    for( unsigned int i = 1; i < strlen(szSentence) && szSentence[i] != '*'; i++ )
        calculatedCheckSum ^= szSentence[i];

    szSentence = GetNextToken(szSentence,szToken); // GPGGA tag

    szSentence = GetNextToken(szSentence,szToken);
    gps.time = atoi(szToken);

    szSentence = GetNextToken(szSentence,szToken);

    if( strlen(szToken) == 0 )
        return false;

    gps.latitude = atof(szToken);

    int intPart = int(gps.latitude / 100);
    gps.latitude = intPart + ( gps.latitude - intPart * 100 ) / 60;

    szSentence = GetNextToken(szSentence,szToken);

    if( szToken[0] == 'S' )
        gps.latitude *= -1;

    szSentence = GetNextToken(szSentence,szToken);

    if( strlen(szToken) == 0 )
        return false;

    gps.longitude = atof(szToken);

    intPart = int(gps.longitude / 100);
    gps.longitude = intPart + ( gps.longitude - intPart * 100 ) / 60;

    szSentence = GetNextToken(szSentence,szToken);

    if( szToken[0] == 'W' )
        gps.longitude *= -1;

    szSentence = GetNextToken(szSentence,szToken);

    //if( szToken[0] != '1' ) // no fix
    if( szToken[0] == '0' ) // no fix (changed 20110527)
        return false;

    szSentence = GetNextToken(szSentence,szToken);
    gps.satellites = strlen(szToken) > 0 ? atoi(szToken) : DEFAULT;

    szSentence = GetNextToken(szSentence,szToken);
    gps.HDOP = strlen(szToken) > 0 ? atof(szToken) : DEFAULT;

    szSentence = GetNextToken(szSentence,szToken);
    gps.altitude = strlen(szToken) > 0 ? atof(szToken) : DEFAULT;

    szSentence = GetNextToken(szSentence,szToken);

    if( szToken[0] != 'M' )
        gps.altitude = DEFAULT; // reading is only valid if in meters

    szSentence = GetNextToken(szSentence,szToken); // height of geoid
    szSentence = GetNextToken(szSentence,szToken); // meters

    szSentence = GetNextToken(szSentence,szToken); // unused

    char* asterixPos = strstr(szSentence,"*");

    if( !asterixPos )
        return false;

    int checkSumNotHex;
    sscanf_s(asterixPos + 1,"%x",&checkSumNotHex);

    if( calculatedCheckSum != checkSumNotHex )
        return false;

    return true;
}


DWORD WINAPI ReadGPSThread(LPVOID pParam)
{
    GPSThreadInfo* gpsTI = (GPSThreadInfo *)pParam;

    unsigned long bytesRead;
    char gpsSentence[1000],c;

    int nc = 0;

    bool keepReadingMode = gpsTI->readlastMode;
    GPSInfo keepReadingGPSInfo;

    COMMTIMEOUTS comTimeout;
    SecureZeroMemory(&comTimeout,sizeof(COMMTIMEOUTS));
    comTimeout.ReadIntervalTimeout = MAXDWORD;
    //comTimeout.ReadTotalTimeoutConstant = 0; // they're already 0
    //comTimeout.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(gpsTI->hGPS,&comTimeout); // ReadFile will only read data from the buffer

    if( !keepReadingMode )
        PurgeComm(gpsTI->hGPS,PURGE_RXCLEAR);

    while( ( !gpsTI->successfulRead || keepReadingMode ) && !gpsTI->cancelRead )
    {
        if( !ReadFile(gpsTI->hGPS,&c,1,&bytesRead,NULL) && GetLastError() != ERROR_IO_PENDING )
        {
            gpsTI->cancelRead = true;
        }

        else if( bytesRead == 1 )
        {
            if( c == '\n' && nc != 0 ) // end of sentence
            {
                gpsSentence[nc - 1] = '\0';
                nc = 0;

                if( strlen(gpsSentence) >= 6 && gpsSentence[0] == '$' )
                {
                    if( strncmp(&gpsSentence[3],"GGA",3) == 0 )
                    {
                        if( !keepReadingMode )
                        {
                            if( ParseGGA(gpsSentence,*gpsTI->gpsInfo) )
                                gpsTI->successfulRead = true;
                        }

                        else
                        {
                            if( ParseGGA(gpsSentence,keepReadingGPSInfo) )
                            {
                                gpsTI->gpsInfo->altitude = keepReadingGPSInfo.altitude;
                                gpsTI->gpsInfo->HDOP = keepReadingGPSInfo.HDOP;
                                gpsTI->gpsInfo->latitude = keepReadingGPSInfo.latitude;
                                gpsTI->gpsInfo->longitude = keepReadingGPSInfo.longitude;
                                gpsTI->gpsInfo->satellites = keepReadingGPSInfo.satellites;
                                gpsTI->gpsInfo->time = keepReadingGPSInfo.time;
                                gpsTI->successfulRead = true;
                            }
                        }
                    }
                }
            }

            else
            {
                gpsSentence[nc++] = c;
            }
        }

        if( keepReadingMode && bytesRead == 0 ) // we've reached the end of the buffer
            gpsTI->cancelRead = true;
    }

    return 0;
}


double CIntDriver::exgps(int iExpr)
{
    static HANDLE hGPS = INVALID_HANDLE_VALUE;
    static GPSInfo gpsRead;
    static bool validGPSRead = false;
    DWORD returnCode;

    const auto& gps_node = GetNode<Nodes::GPS>(iExpr);

    switch( gps_node.command )
    {
        ///////////////////////////////////
        case Nodes::GPS::Command::Open:
        {
            if( hGPS != INVALID_HANDLE_VALUE )
                return 1; // no reason to open the GPS again

            int comPort = ( gps_node.options[0] == -1 ) ? 3 : evalexpr<int>(gps_node.options[0]);
            int baudRate = ( gps_node.options[1] == -1 ) ? 4800 : evalexpr<int>(gps_node.options[1]);

            if( comPort < 0 || comPort > 256 || baudRate < 110 || baudRate > 256000 )
                return 0; // invalid port settings

            CString portName;

            portName.Format(_T("\\\\.\\COM%d"),comPort);

            hGPS = CreateFile(portName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,0);

            DCB dcb;
            SecureZeroMemory(&dcb,sizeof(DCB));
            dcb.DCBlength = sizeof(DCB);

            if( hGPS == INVALID_HANDLE_VALUE || !GetCommState(hGPS,&dcb) )
                return 0;

            dcb.BaudRate = baudRate;
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;

            if( !SetCommState(hGPS,&dcb) )
            {
                CloseHandle(hGPS);
                hGPS = INVALID_HANDLE_VALUE;
                return 0;
            }

            return 1;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Close:
        {
            if( hGPS == INVALID_HANDLE_VALUE )
                return 0; // no GPS connection open

            returnCode = CloseHandle(hGPS);
            hGPS = INVALID_HANDLE_VALUE;
            return returnCode;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Status:
        {
            return DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Read:
        {
            validGPSRead = false;

            if( hGPS == INVALID_HANDLE_VALUE )
                return 0; // no GPS connection open

            // default to 3 seconds if the user hasn't specified a wait time (in seconds)
            double maxWaitTime = ( gps_node.options[0] == 0 ) ? 3 : evalexpr(gps_node.options[0]);

            maxWaitTime = 1000 * std::min(maxWaitTime,600.0); // max time is ten minutes

            GPSThreadInfo gpsTI;

            gpsTI.hGPS = hGPS;
            gpsTI.gpsInfo = &gpsRead;
            gpsTI.successfulRead = false;
            gpsTI.cancelRead = false;
            gpsTI.numReadIntervals = int(maxWaitTime / GPS_INTERVAL_DIALOG_CHECK);
            gpsTI.readlastMode = false;

            CDialog* pDlg = NULL;

            if( gps_node.options[2] != 0 )
            {
                gpsTI.windowText = EvalAlphaExpr<CString>(gps_node.options[2]);
                pDlg = (CDialog*)AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GPS_DIALOG,0,(LPARAM)&gpsTI);
            }

            ::CreateThread(NULL,0,ReadGPSThread,&gpsTI,0,NULL);

            if( pDlg )
            {
                pDlg->DoModal();
            }

            else
            {
                while( !gpsTI.successfulRead && !gpsTI.cancelRead && gpsTI.numReadIntervals > 0 ) // wait for the gps thread to finish
                {
                    ::Sleep(GPS_INTERVAL_DIALOG_CHECK);
                    gpsTI.numReadIntervals--;
                }

                gpsTI.cancelRead = true; // set in the case of a time out so the gps thread finishes
            }

            validGPSRead = gpsTI.successfulRead;

            return validGPSRead;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::ReadLast:
        {
            validGPSRead = false;

            if( hGPS == INVALID_HANDLE_VALUE )
                return 0; // no GPS connection open

            GPSThreadInfo gpsTI;

            gpsTI.hGPS = hGPS;
            gpsTI.gpsInfo = &gpsRead;
            gpsTI.successfulRead = false;
            gpsTI.cancelRead = false;
            gpsTI.readlastMode = true;

            ReadGPSThread(&gpsTI);

            validGPSRead = gpsTI.successfulRead;

            return validGPSRead;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Latitude:
        {
            return validGPSRead ? gpsRead.latitude : DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Longitude:
        {
            return validGPSRead ? gpsRead.longitude : DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Altitude:
        {
            return validGPSRead ? gpsRead.altitude : DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Satellites:
        {
            return validGPSRead ? gpsRead.satellites : DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Accuracy:
        {
            return validGPSRead ? gpsRead.HDOP : DEFAULT;
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::ReadTime:
        {
            if( !validGPSRead  )
                return DEFAULT;

            // read time is in UTC, convert to local time
            SYSTEMTIME time;
            FILETIME fileTime,adjustedFileTime;

            CTime::GetCurrentTime().GetAsSystemTime(time);

            int readTime = int(gpsRead.time);
            time.wHour = static_cast<WORD>(readTime / 10000);
            time.wMinute = ( readTime % 10000 ) / 100;
            time.wSecond = readTime % 100;

            SystemTimeToFileTime(&time,&fileTime);
            FileTimeToLocalFileTime(&fileTime,&adjustedFileTime);
            FileTimeToSystemTime(&adjustedFileTime,&time);

            return DateHelper::ToYYYYMMDD(time.wHour, time.wMinute, time.wSecond);
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::Distance:
        {
            double lat1 = evalexpr(gps_node.options[0]);
            double long1 = evalexpr(gps_node.options[1]);
            double lat2 = evalexpr(gps_node.options[2]);
            double long2 = evalexpr(gps_node.options[3]);
            return GreatCircle::Distance(lat1, long1, lat2, long2);
        }


        ///////////////////////////////////
        case Nodes::GPS::Command::ReadInteractive:
        case Nodes::GPS::Command::Select:
        {
            return DEFAULT;
        }
    }

    ASSERT(false);
    return DEFAULT; // we shouldn't get here
}



#else // 20140129 adding GPS functionality for Android devices

struct GPSInfo
{
    double latitude;
    double longitude;
    double altitude;
    double satellites;
    double accuracy;
    double readtime;
};


bool TranslateGPSReading(GPSInfo* pGPSInfo, const CString& gpsString)
{
    if( gpsString.IsEmpty() )
        return false;

    // the string will be in a format like this: 39.212526000;-77.345438000;620.000000000;-1;205.000000000;224344
    int semicolonPositions[5];

    semicolonPositions[0] = gpsString.Find(L';');

    for( int i = 1; i < _countof(semicolonPositions); ++i )
        semicolonPositions[i] = gpsString.Find(L';', semicolonPositions[i - 1] + 1);

    pGPSInfo->latitude = StringToNumber(gpsString.Mid(0, semicolonPositions[0]));
    pGPSInfo->longitude = StringToNumber(gpsString.Mid(semicolonPositions[0] + 1, semicolonPositions[1] - semicolonPositions[0] - 1));
    pGPSInfo->altitude = StringToNumber(gpsString.Mid(semicolonPositions[1] + 1, semicolonPositions[2] - semicolonPositions[1] - 1));
    pGPSInfo->satellites = StringToNumber(gpsString.Mid(semicolonPositions[2] + 1, semicolonPositions[3] - semicolonPositions[2] - 1));
    pGPSInfo->accuracy = StringToNumber(gpsString.Mid(semicolonPositions[3] + 1, semicolonPositions[4] - semicolonPositions[3] - 1));
    pGPSInfo->readtime = StringToNumber(gpsString.Mid(semicolonPositions[4] + 1));

    if( pGPSInfo->altitude == -1 )
        pGPSInfo->altitude = DEFAULT;

    if( pGPSInfo->satellites == -1 )
        pGPSInfo->satellites = DEFAULT;

    if( pGPSInfo->accuracy == -1 )
        pGPSInfo->accuracy = DEFAULT;

    return true;
}


Paradata::GpsReadingInstance GPSInfoToGpsReadingInstance(bool record_coordinates, const GPSInfo& gps_info)
{
    auto conditionally_assign = [](double value, bool assign = true) -> std::optional<double>
    {
        return ( assign && value != DEFAULT ) ? std::make_optional(value) : std::nullopt;
    };

    return Paradata::GpsReadingInstance
    {
        conditionally_assign(gps_info.latitude, record_coordinates),
        conditionally_assign(gps_info.longitude, record_coordinates),
        conditionally_assign(gps_info.altitude, record_coordinates),
        conditionally_assign(gps_info.satellites),
        conditionally_assign(gps_info.accuracy),
        conditionally_assign(gps_info.readtime)
    };
}


double CIntDriver::exgps(int iExpr)
{
    static GPSInfo gpsInfo;
    static bool validGPSRead = false;

    const auto& gps_node = GetNode<Nodes::GPS>(iExpr);
    double dRetVal = DEFAULT;
    CString gpsReturnValue;

    auto process_read_return_value = [&]()
    {
        if( gpsReturnValue == _T("cancel") )
        {
            dRetVal = -1;
        }

        else
        {
            validGPSRead = TranslateGPSReading(&gpsInfo, gpsReturnValue);
            dRetVal = validGPSRead ? 1 : 0;
        }
    };

    std::shared_ptr<Paradata::GpsEvent> gps_event;

    if( Paradata::Logger::IsOpen() )
    {
        // before processing any new GPS events, log any cached events
        Paradata::Logger::SendPortableMessage(Paradata::PortableMessage::QueryCachedEvents);
    }

    switch( gps_node.command )
    {
        ///////////////////////////////////
        case Nodes::GPS::Command::Open:
        {
            if( Paradata::Logger::IsOpen() )
                gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::Open);

            dRetVal = PlatformInterface::GetInstance()->GetApplicationInterface()->GpsOpen();
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Close:
        {
            if( Paradata::Logger::IsOpen() )
                gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::Close);

            dRetVal = PlatformInterface::GetInstance()->GetApplicationInterface()->GpsClose();
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Status:
        {
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Read:
        {
            validGPSRead = false;

            // default to 3 seconds if the user hasn't specified a wait time (in seconds)
            double maxWaitTime = ( gps_node.options[0] == 0 ) ? 3 : evalexpr(gps_node.options[0]);

            maxWaitTime = 1000 * std::min(maxWaitTime,600.0); // max time is ten minutes

            int wait_time = (int)maxWaitTime;
            int desired_accuracy = 0;
            CString gpsDialogText;

            if( gps_node.options[1] != 0 ) // desired accuracy
                desired_accuracy = evalexpr<int>(gps_node.options[1]);

            if( gps_node.options[2] != 0 )
                gpsDialogText = EvalAlphaExpr<CString>(gps_node.options[2]);

            if( Paradata::Logger::IsOpen() )
            {
                gps_event = std::make_shared<Paradata::GpsReadRequestEvent>(
                    Paradata::GpsEvent::Action::Read,
                    wait_time,
                    ( gps_node.options[1] != 0 ) ? std::make_optional(desired_accuracy) : std::nullopt,
                    ( gps_node.options[2] != 0 ) ? std::make_optional(gpsDialogText) : std::nullopt);
            }

            gpsReturnValue = PlatformInterface::GetInstance()->GetApplicationInterface()->GpsRead(wait_time, desired_accuracy, gpsDialogText);

            process_read_return_value();

            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::ReadLast:
        {
            if( Paradata::Logger::IsOpen() )
                gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::ReadLast);

            gpsReturnValue = PlatformInterface::GetInstance()->GetApplicationInterface()->GpsReadLast();
            validGPSRead = TranslateGPSReading(&gpsInfo, gpsReturnValue);
            dRetVal = validGPSRead ? 1 : 0;

            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Latitude:
        {
            dRetVal = validGPSRead ? gpsInfo.latitude : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Longitude:
        {
            dRetVal = validGPSRead ? gpsInfo.longitude : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Altitude:
        {
            dRetVal = validGPSRead ? gpsInfo.altitude : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Satellites:
        {
            dRetVal = validGPSRead ? gpsInfo.satellites : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Accuracy:
        {
            dRetVal = validGPSRead ? gpsInfo.accuracy : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::ReadTime:
        {
            dRetVal = validGPSRead ? gpsInfo.readtime : DEFAULT;
            break;
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::Distance:
        {
            double lat1 = evalexpr(gps_node.options[0]);
            double long1 = evalexpr(gps_node.options[1]);
            double lat2 = evalexpr(gps_node.options[2]);
            double long2 = evalexpr(gps_node.options[3]);
            return GreatCircle::Distance(lat1, long1, lat2, long2);
        }

        ///////////////////////////////////
        case Nodes::GPS::Command::ReadInteractive:
        case Nodes::GPS::Command::Select:
        {
            bool read_interactive_mode = ( gps_node.command == Nodes::GPS::Command::ReadInteractive );

            const int& base_map_type = gps_node.options[0];
            const int& base_map_filename_expression = gps_node.options[1];
            BaseMapSelection base_map_selection;

            if( base_map_type == -1 )
            {
                base_map_selection = GetDefaultBaseMapSelection(*m_pEngineDriver->m_pPifFile);
            }

            else if( base_map_type == 0 )
            {
                CString base_map_text = EvalAlphaExpr<CString>(base_map_filename_expression);
                base_map_selection = FromString(base_map_text, m_pEngineDriver->m_pPifFile->GetAppFName());
            }

            else
            {
                base_map_selection = (BaseMap)base_map_type;
            }

            CString message = ( gps_node.options[2] != -1 ) ? EvalAlphaExpr<CString>(gps_node.options[2]) :
                                                              CString();

            // if the read duration wasn't specified, default to 15 seconds
            double read_duration = ( gps_node.command == Nodes::GPS::Command::ReadInteractive && gps_node.options[3] != -1 ) ?
                evalexpr(gps_node.options[3]) : 15;

            if( Paradata::Logger::IsOpen() )
            {
                gps_event = std::make_shared<Paradata::GpsReadRequestEvent>(
                    read_interactive_mode ? Paradata::GpsEvent::Action::ReadInteractive : Paradata::GpsEvent::Action::Select ,
                    read_duration,
                    std::nullopt,
                    ( gps_node.options[1] != -1 ) ? std::make_optional(message) : std::nullopt);
            }

            gpsReturnValue = PlatformInterface::GetInstance()->GetApplicationInterface()->GpsReadInteractive(read_interactive_mode,
                base_map_selection, message, read_duration);

            process_read_return_value();

            break;
        }
    }


    if( gps_event != nullptr )
    {
        std::optional<Paradata::GpsReadingInstance> gps_reading_instance;

        if( dRetVal == 1 && ( gps_node.command == Nodes::GPS::Command::Read ||
                              gps_node.command == Nodes::GPS::Command::ReadLast ||
                              gps_node.command == Nodes::GPS::Command::ReadInteractive ||
                              gps_node.command == Nodes::GPS::Command::Select ) )
        {
            bool record_coordinates = m_pEngineDriver->GetPifFile()->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordCoordinates();
            gps_reading_instance = GPSInfoToGpsReadingInstance(record_coordinates, gpsInfo);
        }

        gps_event->SetPostExecutionValues(dRetVal, gps_reading_instance);
        m_pParadataDriver->RegisterAndLogEvent(gps_event);
    }


    return dRetVal;
}

#endif


std::shared_ptr<Paradata::Event> CIntDriver::CreateParadataGpsEvent(const CString& event_type, const CString& event_information)
{
    std::shared_ptr<Paradata::GpsEvent> gps_event;

#ifndef WIN_DESKTOP

    if( event_type.Compare(_T("gps_background_open")) == 0 )
    {
        gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::BackgroundOpen);
        gps_event->SetPostExecutionValues(( event_information.Compare(_T("1")) == 0 ) ? 1 : 0);
    }

    else if( event_type.Compare(_T("gps_background_close")) == 0 )
    {
        gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::BackgroundClose);
        gps_event->SetPostExecutionValues(1);
    }

    else if( event_type.Compare(_T("gps_background_read")) == 0 )
    {
        GPSInfo gps_info;
        TranslateGPSReading(&gps_info, event_information);
        bool record_coordinates = m_pEngineDriver->GetPifFile()->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordCoordinates();
        gps_event = std::make_shared<Paradata::GpsEvent>(Paradata::GpsEvent::Action::BackgroundReading,
            GPSInfoToGpsReadingInstance(record_coordinates, gps_info));
    }

    else
    {
        ASSERT(false);
    }

#endif

    return gps_event;
}
