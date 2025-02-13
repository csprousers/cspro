using System;
using System.Collections;
using System.Collections.Generic;

namespace Operator_Statistics_Viewer
{
    class SummaryReport
    {
        public class Statistics
        {
            public DateTime StartDate;
            public DateTime EndDate;
            public int NumberOperators;
            public int CasesEntered;
            public int EntryTime;
            public int Keystrokes;
            public int DaysWorked;
            public double OperatorsPerDay;
            public double HoursPerDay;
            public int WeeksWorked;
            public double OperatorsPerWeek;
            public double HoursPerWeek;

            public double SecondsPerCase { get { return (double)EntryTime / CasesEntered; } }
            public double KeystrokesPerHour { get { return Keystrokes / ( EntryTime / 3600.0 ); } }
            public double KeystrokesPerCase { get { return (double)Keystrokes / CasesEntered; } }

            public double ExpectedDaysRemaining;
            public double ExpectedWeeksRemaining;
            public DateTime ExpectedCompletionDate;
            public double KeyersNeededAtCurrentRate;
            public int KeyingRateNeededWithCurrentKeyers;


            public Statistics()
            {
                StartDate = DateTime.MaxValue;
                EndDate = DateTime.MinValue;
            }

            public void CalculateExpectations(int casesRemaining,DateTime desiredEndDate)
            {
                if( casesRemaining == 0 )
                    return;

                // calculated using the daily rate
                ExpectedDaysRemaining = casesRemaining * SecondsPerCase / 3600.0 / HoursPerDay / OperatorsPerDay;

                // calculated using the weekly rate
                ExpectedWeeksRemaining = casesRemaining * SecondsPerCase / 3600.0 / HoursPerWeek / OperatorsPerWeek;
                ExpectedCompletionDate = DateTime.Now.AddDays(ExpectedWeeksRemaining * 7);

                if( DateTime.Now < desiredEndDate )
                {
                    double weeksBeforeEndDate = desiredEndDate.Subtract(BeginningOfDay(DateTime.Now)).Days / 7.0;

                    double hoursRemainingAtCurrentKeyingRate = ( KeystrokesPerCase * casesRemaining ) / KeystrokesPerHour;
                    KeyersNeededAtCurrentRate = hoursRemainingAtCurrentKeyingRate / ( weeksBeforeEndDate * HoursPerWeek );

                    double hoursAvailableAtCurrentHoursWorked = OperatorsPerWeek * HoursPerWeek * weeksBeforeEndDate;
                    KeyingRateNeededWithCurrentKeyers = (int)(( KeystrokesPerCase * casesRemaining ) / hoursAvailableAtCurrentHoursWorked );
                }
            }
        };

        int casesExpected;
        DateTime desiredEndDate;

        int casesEntered;
        int numberOperators;

        Statistics allTime;
        Statistics lastWeek;


        public int CasesEntered { get { return casesEntered; } }
        public int CasesRemaining { get { return Math.Max(0,casesExpected - casesEntered); } }
        public int NumberOperators { get { return numberOperators; } }
        public Statistics AllTime { get { return allTime; } }
        public Statistics LastWeek { get { return lastWeek; } }


        public SummaryReport(SortedDictionary<string,ArrayList> statisticsByOperator,int casesExpected,DateTime endDate)
        {
            this.casesExpected = casesExpected;
            desiredEndDate = endDate;
            numberOperators = statisticsByOperator.Count;

            if( statisticsByOperator.Count == 0 )
                return;

            // first we'll go through the statistics to get the number of cases entered and the latest date of entry
            DateTime lastDate = DateTime.MinValue;

            foreach( var kvp in statisticsByOperator )
            {
                foreach( OperatorStatistics os in kvp.Value )
                {
                    if( os.Mode == EntryMode.Add )
                    {
                        casesEntered += os.Cases;

                        if( os.StartTime > lastDate )
                            lastDate = os.StartTime;
                    }
                }
            }

            allTime = CreateGeneralStatistics(statisticsByOperator,DateTime.MinValue,EndOfDay(lastDate));
            lastWeek = CreateGeneralStatistics(statisticsByOperator,BeginningOfDay(lastDate).AddDays(-6),EndOfDay(lastDate));
        }

        static DateTime BeginningOfDay(DateTime date)
        {
            return new DateTime(date.Year,date.Month,date.Day,0,0,0);
        }
        
        static DateTime EndOfDay(DateTime date)
        {
            return new DateTime(date.Year,date.Month,date.Day,23,59,59);
        }

        
        class TimeTally
        {
            public int EntryTime;
            public Hashtable Operators;

            public TimeTally()
            {
                Operators = new Hashtable();
            }
        }

        Statistics CreateGeneralStatistics(SortedDictionary<string,ArrayList> statisticsByOperator,DateTime startDate,DateTime endDate)
        {
            Statistics stats = new Statistics();

            Dictionary<DateTime,TimeTally> daysWorked = new Dictionary<DateTime,TimeTally>();
            Dictionary<int,TimeTally> weeksWorked = new Dictionary<int,TimeTally>();

            foreach( var kvp in statisticsByOperator )
            {
                kvp.Value.Sort(new OperatorStatisticsDateSorter());

                bool personKeyedWithinRange = false;

                foreach( OperatorStatistics os in kvp.Value )
                {
                    if( os.StartTime < startDate || os.Mode != EntryMode.Add )
                        continue;

                    else if( os.StartTime > endDate )
                        break;

                    personKeyedWithinRange = true;
                    
                    if( os.StartTime < stats.StartDate )
                        stats.StartDate = os.StartTime;
                    
                    if( os.StartTime > stats.EndDate )
                        stats.EndDate= os.StartTime;
                    
                    stats.CasesEntered += os.Cases;

                    int addTime = os.KeyingTime - os.PauseTime;
                    stats.EntryTime += addTime;

                    stats.Keystrokes += os.Keystrokes;

                    DateTime thisDay = BeginningOfDay(os.StartTime);
                    int thisWeek = endDate.Subtract(thisDay).Days / 7;

                    TimeTally dayTally,weekTally;

                    if( daysWorked.ContainsKey(thisDay) )
                        dayTally = daysWorked[thisDay];

                    else
                    {
                        dayTally = new TimeTally();
                        daysWorked.Add(thisDay,dayTally);
                    }
                    
                    if( weeksWorked.ContainsKey(thisWeek) )
                        weekTally = weeksWorked[thisWeek];

                    else
                    {
                        weekTally = new TimeTally();
                        weeksWorked.Add(thisWeek,weekTally);
                    }

                    dayTally.EntryTime += addTime;
                    dayTally.Operators[kvp.Key] = null;
                    
                    weekTally.EntryTime += addTime;
                    weekTally.Operators[kvp.Key] = null;
                }

                if( personKeyedWithinRange )
                    stats.NumberOperators++;
            }

            // process daysWorked and weeksWorked
            stats.DaysWorked = daysWorked.Count;

            int totalOperators = 0;
            int totalTime = 0;

            foreach( var kvp in daysWorked )
            {
                totalOperators += kvp.Value.Operators.Count;
                totalTime += kvp.Value.EntryTime;
            }

            stats.OperatorsPerDay = (double)totalOperators / stats.DaysWorked;
            stats.HoursPerDay = totalTime / 3600.0 / stats.OperatorsPerDay / stats.DaysWorked;
            
            stats.WeeksWorked = weeksWorked.Count;

            totalOperators = 0;
            totalTime = 0;

            foreach( var kvp in weeksWorked )
            {
                totalOperators += kvp.Value.Operators.Count;
                totalTime += kvp.Value.EntryTime;
            }

            stats.OperatorsPerWeek = (double)totalOperators / stats.WeeksWorked;
            stats.HoursPerWeek = totalTime / 3600.0 / stats.OperatorsPerWeek / stats.WeeksWorked;

            stats.CalculateExpectations(CasesRemaining,desiredEndDate);

            return stats;
        }

        public Statistics CreateOperatorStatistics(ArrayList statistics)
        {
            SortedDictionary<string,ArrayList> sd = new SortedDictionary<string,ArrayList>();
            sd.Add(" ",statistics);
            return CreateGeneralStatistics(sd,DateTime.MinValue,DateTime.MaxValue);
        }

    }
}
