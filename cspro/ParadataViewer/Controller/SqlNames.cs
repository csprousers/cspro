namespace ParadataViewer
{
    static class N
    {
        internal static class Table
        {
            internal const string Event = "`event`";
            internal const string GpsEvent = "`gps_event`";
            internal const string GpsReadingInstance = "`gps_reading_instance`";
            internal const string GpsReadRequestInstance = "`gps_read_request_instance`";

            internal const string ApplicationInstance = "`application_instance`";
            internal const string DeviceInfo = "`device_info`";
            internal const string SessionInstance = "`session_instance`";
            internal const string SessionInfo = "`session_info`";
            internal const string OperatoridInfo = "`operatorid_info`";
        }

        internal static class Column
        {
            internal const string Id = "`id`";

            internal const string GpsAction = Table.GpsEvent + ".`action`";

            internal const string GpsReadingLatitude = Table.GpsReadingInstance + ".`latitude`";
            internal const string GpsReadingLongitude = Table.GpsReadingInstance + ".`longitude`";
            internal const string GpsReadingAltitude = Table.GpsReadingInstance + ".`altitude`";
            internal const string GpsReadingSatellites = Table.GpsReadingInstance + ".`satellites`";
            internal const string GpsReadingAccuracy = Table.GpsReadingInstance + ".`accuracy`";

            internal const string GpsReadRequestMaxReadDuration = Table.GpsReadRequestInstance + ".`max_read_duration`";
            internal const string GpsReadRequestDesiredAccuracy = Table.GpsReadRequestInstance + ".`desired_accuracy`";
            internal const string GpsReadRequestReadDuration = Table.GpsReadRequestInstance + ".`read_duration`";

            internal const string EventTime = Table.Event + ".`time`";
            internal const string DeviceUsername = Table.DeviceInfo + ".`username`";
            internal const string OperatoridOperatoridInfo = Table.OperatoridInfo + ".`operatorid`";
        }
    }
}
