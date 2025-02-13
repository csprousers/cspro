using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    class DateFilter : Filter
    {
        private const string ApplicationEventTableName = "application_event";
        private const string TimeColumnName = "time";

        private class DateRange
        {
            public string DisplayText;
            public double StartTimestamp;
            public double EndTimestampExclusive;
        }

        internal enum DateRangeType { Day, Week, Month, Year };

        private Controller _controller;
        private HashSet<DateTime> _localDaysSet;
        private List<DateRange>[] _dateRangesByType;

        internal DateRangeType SelectedDateRangeType { get; set; }

        internal DateFilter(Controller controller)
            : base(FilterType.Event)
        {
            _controller = controller;
            _localDaysSet =  new HashSet<DateTime>();
            _dateRangesByType = new List<DateRange>[Enum.GetNames(typeof(DateRangeType)).Length];
            SelectedDateRangeType  = DateRangeType.Week;
        }

        internal override bool ForceRetrieveAllRows { get { return true; } }

        internal override string GetPossibleValuesSqlQuery()
        {
            return  $"SELECT `{EventTableName}`.`{TimeColumnName}` FROM `{ApplicationEventTableName}` " +
                    $"JOIN `{EventTableName}` ON `{ApplicationEventTableName}`.`id` = `{EventTableName}`.`id` " +
                    $"ORDER BY`{EventTableName}`.`{TimeColumnName}`;";
        }

        // from https://stackoverflow.com/questions/249760/how-can-i-convert-a-unix-timestamp-to-datetime-and-vice-versa
        // when updated to .NET 4.6, can use DateTimeOffset.FromUnixTimeSeconds(xxx).UtcDateTime
        private static DateTime UnixEpoch = new DateTime(1970,1,1,0,0,0,0,DateTimeKind.Utc);

        private static DateTime TimestampToLocalDateTime(double unixTimeStamp)
        {
            return UnixEpoch.AddSeconds(unixTimeStamp).ToLocalTime();
        }

        private static double LocalDateTimeToTimestamp(DateTime localDate)
        {
            return localDate.ToUniversalTime().Subtract(UnixEpoch).TotalSeconds;
        }

        internal override List<string> ProcessPossibleValues(List<object[]> values)
        {
            // process the timestamps
            foreach( var row in values )
            {
                object value = row.Last();

                if( value is double )
                {
                    DateTime localDate = TimestampToLocalDateTime((double)value);
                    _localDaysSet.Add(localDate.Date);
                }
            }

            return GetPossibleValues();
        }

        internal List<string> GetPossibleValues()
        {
            if( _dateRangesByType[(int)SelectedDateRangeType] == null )
            {
                // group the days into the appropriate categories
                _dateRangesByType[(int)SelectedDateRangeType] =
                    ( SelectedDateRangeType == DateRangeType.Day ) ? ConstructDayRanges() :
                    ( SelectedDateRangeType == DateRangeType.Week ) ? ConstructWeekRanges() :
                    ( SelectedDateRangeType == DateRangeType.Month ) ? ConstructMonthRanges() :
                    ( SelectedDateRangeType == DateRangeType.Year ) ? ConstructYearRanges() :
                    throw new InvalidOperationException();
            }

            return _dateRangesByType[(int)SelectedDateRangeType].Select(x => x.DisplayText).ToList();
        }

        private List<DateRange> ConstructDayRanges()
        {
            var dateRanges = new List<DateRange>();

            foreach( var localDate in _localDaysSet )
            {
                var startLocalDate = localDate;
                var endLocalDateExclusive = startLocalDate.AddDays(1);

                var startTimestamp = LocalDateTimeToTimestamp(startLocalDate);

                dateRanges.Add(new DateRange()
                {
                    DisplayText = Helper.FormatTimestamp(_controller.Settings.TimestampFormatterForFilters,startTimestamp),
                    StartTimestamp = startTimestamp,
                    EndTimestampExclusive = LocalDateTimeToTimestamp(endLocalDateExclusive)
                });
            }

            return dateRanges;
        }

        private static DateTime GetFirstDayOfWeek(DateTime date)
        {
            // start week ranges on Sundays
            return date.AddDays(-1 * (int)date.DayOfWeek);
        }

        private List<DateRange> ConstructWeekRanges()
        {
            var dateRanges = new List<DateRange>();

            DateTime lastFirstDayOfWeek = UnixEpoch;

            foreach( var localDate in _localDaysSet )
            {
                if( localDate.Subtract(lastFirstDayOfWeek).TotalDays >= 7 )
                {
                    var startLocalDate = GetFirstDayOfWeek(localDate);
                    var endLocalDateExclusive = startLocalDate.AddDays(7);

                    var startTimestamp = LocalDateTimeToTimestamp(startLocalDate);
                    var endTimestampInclusive = LocalDateTimeToTimestamp(endLocalDateExclusive.AddMilliseconds(-1));

                    dateRanges.Add(new DateRange()
                    {
                        DisplayText = Helper.FormatTimestamp(_controller.Settings.TimestampFormatterForFilters,startTimestamp) +
                            " – " + Helper.FormatTimestamp(_controller.Settings.TimestampFormatterForFilters,endTimestampInclusive),
                        StartTimestamp = startTimestamp,
                        EndTimestampExclusive = LocalDateTimeToTimestamp(endLocalDateExclusive)
                    });

                    lastFirstDayOfWeek = startLocalDate;
                }
            }

            return dateRanges;
        }

        private List<DateRange> ConstructMonthRanges()
        {
            var dateRanges = new List<DateRange>();

            int lastYear = 0;
            int lastMonth = 0;

            foreach( var localDate in _localDaysSet )
            {
                if( localDate.Year != lastYear || localDate.Month != lastMonth )
                {
                    var startLocalDate = new DateTime(localDate.Year,localDate.Month,1);
                    var endLocalDateExclusive = startLocalDate.AddMonths(1);

                    dateRanges.Add(new DateRange()
                    {
                        DisplayText = startLocalDate.ToString("MMMM yyyy"),
                        StartTimestamp = LocalDateTimeToTimestamp(startLocalDate),
                        EndTimestampExclusive = LocalDateTimeToTimestamp(endLocalDateExclusive)
                    });

                    lastYear = localDate.Year;
                    lastMonth = localDate.Month;
                }
            }

            return dateRanges;
        }

        private List<DateRange> ConstructYearRanges()
        {
            var dateRanges = new List<DateRange>();

            int lastYear = 0;

            foreach( var localDate in _localDaysSet )
            {
                if( localDate.Year != lastYear )
                {
                    var startLocalDate = new DateTime(localDate.Year,1,1);
                    var endLocalDateExclusive = startLocalDate.AddYears(1);

                    dateRanges.Add(new DateRange()
                    {
                        DisplayText = startLocalDate.Year.ToString(),
                        StartTimestamp = LocalDateTimeToTimestamp(startLocalDate),
                        EndTimestampExclusive = LocalDateTimeToTimestamp(endLocalDateExclusive)
                    });

                    lastYear = localDate.Year;
                }
            }

            return dateRanges;
        }

        internal override string GetWhereSql(int index)
        {
            var dateRange = _dateRangesByType[(int)SelectedDateRangeType][index];

            return $"( `{EventTableName}`.`{TimeColumnName}` >= {dateRange.StartTimestamp} AND " +
                $"`{EventTableName}`.`{TimeColumnName}` < {dateRange.EndTimestampExclusive} )";
        }
    }

    class FilterControlDate : FilterControl
    {
        private DateFilter _dateFilter;
        private ComboBox _comboBoxDateRangeType;

        internal FilterControlDate(Controller controller)
            : base(controller,new DateFilter(controller))
        {
            _dateFilter = (DateFilter)Filter;

            // add the date range type combo box
            _comboBoxDateRangeType = new ComboBox()
            {
                DropDownStyle = ComboBoxStyle.DropDownList
            };

            _comboBoxDateRangeType.Items.AddRange(Enum.GetNames(typeof(DateFilter.DateRangeType)));
            _comboBoxDateRangeType.SelectedIndex = (int)_dateFilter.SelectedDateRangeType;

            _comboBoxDateRangeType.SelectedIndexChanged += comboBoxDateRangeType_SelectedIndexChanged;

            AddCustomControl(_comboBoxDateRangeType);
        }

        private void comboBoxDateRangeType_SelectedIndexChanged(object sender,EventArgs e)
        {
            _dateFilter.SelectedDateRangeType = (DateFilter.DateRangeType)_comboBoxDateRangeType.SelectedIndex;
            ReplaceResults(_dateFilter.GetPossibleValues());
        }
    }
}
