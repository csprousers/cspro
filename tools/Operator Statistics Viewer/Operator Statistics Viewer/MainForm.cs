// GHM 20130322

using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace Operator_Statistics_Viewer
{

    public partial class MainForm : Form
    {
        const string statisticsByOperatorFilter = "Operator Statistics Log Files (*.log)|*.log|All Files (*.*)|*.*";

        string applicationName;
        SortedDictionary<string,ArrayList> statisticsByOperator;
        Hashtable openedFiles;

        DateTime operationDesiredEndDate;
        int operationExpectedCases;

        ArrayList namesFileHeaders;
        Hashtable operatorsToNamesFileInfo;
        string operatorNamesFilename;

        ViewingConfigurationFile viewingConfig;

        bool disableUpdateView;

        SummaryReport summaryReport;
        Label[,] summaryReportLabels;

        enum Visualization { KeyersTime, TimeTime, RateTime, CasesTime, CumulativeCasesTime, CumulativeVerificationTime, CasesKeyer, TimeKeyer, RateKeyer };
        Visualization visualization;
        Chart chart;


        public MainForm()
        {
            InitializeComponent();
            //this.WindowState = FormWindowState.Maximized;

            disableUpdateView = true;

            applicationName = this.Text;
            comboBoxTimePeriod.SelectedIndex = 0; // all time
            tabControl_SelectedIndexChanged(null,null);

            statisticsByOperator = new SortedDictionary<string,ArrayList>();
            openedFiles = new Hashtable();

            operationExpectedCases = 0;

            namesFileHeaders = null;
            operatorsToNamesFileInfo = null;
            operatorNamesFilename = null;

            viewingConfig = new ViewingConfigurationFile();

            comboBoxInterval.SelectedIndex = 1; // weekly

            summaryReport = null;
            summaryReportLabels = SetupSummaryReportLabels();

            disableUpdateView = false;

            visualization = Visualization.RateTime;
        }

        string PluralizeWord(int val)
        {
            return val != 1 ? "s" : "";
        }

        void UpdateApplicationTitle()
        {
            if( openedFiles.Count == 0 )
                this.Text = applicationName;

            else
                this.Text = String.Format("{0} - {1} Log File{2}",applicationName,openedFiles.Count,PluralizeWord(openedFiles.Count));
        }

        string GetOperatorName(string logFileName)
        {
            if( operatorsToNamesFileInfo != null && displayOperatorIDsInsteadOfNameToolStripMenuItem.Checked && operatorsToNamesFileInfo.ContainsKey(logFileName.ToUpper()) )
                return (string)( (ArrayList)operatorsToNamesFileInfo[logFileName.ToUpper()] )[1];

            else
                return logFileName;
        }

        void UpdateOperators()
        {
            string selectedOperator = operatorsTreeView.SelectedNode != null ? (string)operatorsTreeView.SelectedNode.Tag : null;

            operatorsTreeView.Nodes.Clear();

            if( statisticsByOperator.Count > 0 )
            {
                TreeNode tn = new TreeNode("All Keyers");
                tn.Tag = tn.Text;
                operatorsTreeView.Nodes.Add(tn);

                tn = new TreeNode("Currently Active Keyers");
                tn.Tag = tn.Text;
                operatorsTreeView.Nodes.Add(tn);
            }

            foreach( var kvp in statisticsByOperator )
            {
                TreeNode tn = new TreeNode();

                tn.Tag = kvp.Key;
                tn.Text = GetOperatorName(kvp.Key);
                operatorsTreeView.Nodes.Add(tn);
            }

            if( statisticsByOperator.Count > 0 )
            {
                bool selected = false;

                if( selectedOperator != null )
                {
                    for( int i = 0; !selected && i < operatorsTreeView.Nodes.Count; i++ )
                    {
                        if( selectedOperator.Equals(operatorsTreeView.Nodes[i].Tag) )
                        {
                            operatorsTreeView.SelectedNode = operatorsTreeView.Nodes[i];
                            selected = true;
                        }
                    }
                }

                if( !selected )
                    operatorsTreeView.SelectedNode = operatorsTreeView.Nodes[0];
            }
        }

        void UpdateView()
        {
            if( disableUpdateView )
                return;

            if( tabControl.SelectedIndex == 0 )
                CreateSummaryReport();

            else if( tabControl.SelectedIndex == 1 )
                CreateLogsReport();

            else if( tabControl.SelectedIndex >= 2 && tabControl.SelectedIndex <= 4 )
                CreateReportByTime();

            else if( tabControl.SelectedIndex == 5 )
                CreateReportByOperator();

            else if( tabControl.SelectedIndex == 6 )
                CreatePayrollReport();

            else if( tabControl.SelectedIndex == 7 )
                CreateVisualization();
        }

        string SecondsToDisplayableTime(int seconds)
        {
            if( displayTimeInDecimalFormatToolStripMenuItem.Checked )
                return String.Format("{0:N2}",seconds / 3600.0);

            else
            {
                int minutes = seconds / 60;
                int hours = minutes / 60;
                minutes %= 60;

                //if( hours < 24 )
                    return String.Format("{0}:{1:D2}",hours,minutes);

                /*int days = hours / 24;
                hours -= days * 24;

                return String.Format("{0} d {1}:{2:D2}",days,hours,minutes);*/
            }
        }

        string HoursToDisplayableTime(double hours)
        {
            if( displayTimeInDecimalFormatToolStripMenuItem.Checked )
                return String.Format("{0:N2}",hours);

            else
            {
                int intHours = (int)hours;
                int minutes = (int)(( hours - intHours ) * 60);
                return String.Format("{0}:{1:D2}",intHours,minutes);
            }
        }

        string DateToDisplayableTime(DateTime time,bool monthOnly)
        {
            string str = String.Format("{0:D4}-{1:D2}",time.Year,time.Month);

            if( !monthOnly )
                str += String.Format("-{0:D2}",time.Day);

            return str;
        }

        string FormatPercent(double percent)
        {
            return String.Format(percent != 0 && percent < 10 ? "{0:F1}%" : "{0:F0}%",percent);
        }

        DataGridView CreateDataGridView()
        {
            DataGridView dgv = new DataGridView();
            dgv.Size = new System.Drawing.Size(tabControl.Size.Width - 10,tabControl.Size.Height);
            dgv.AllowUserToAddRows = false;
            dgv.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            dgv.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
            dgv.ClipboardCopyMode = DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            dgv.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dgv.ScrollBars = ScrollBars.Both;
            dgv.ReadOnly = true;
            dgv.RowHeadersVisible = false;
            dgv.ShowEditingIcon = false;
            dgv.ShowRowErrors = false;
            dgv.SortCompare += new DataGridViewSortCompareEventHandler(dataGridView_SortCompare);
            return dgv;
        }

        void CreateReportByTime()
        {
            TabPage tabPage = tabControl.TabPages[tabControl.SelectedIndex];
            tabPage.Controls.Clear();

            DataGridView dgv = CreateDataGridView();
            dgv.ColumnCount = 16;

            dgv.Columns[0].Name =   tabControl.SelectedIndex == 2 ? "Day" :
                                    tabControl.SelectedIndex == 3 ? "Week Starting" :
                                    "Month";

            dgv.Columns[1].Name = "Total Entry Time";
            dgv.Columns[2].Name = "Operators";
            dgv.Columns[3].Name = "Time Per Operator";
            dgv.Columns[4].Name = "Add Time";
            dgv.Columns[5].Name = "Modify Time";
            dgv.Columns[6].Name = "Verify Time";
            dgv.Columns[7].Name = "Pause Time";
            dgv.Columns[8].Name = "Cases Added";
            dgv.Columns[9].Name = "Records Added";
            dgv.Columns[10].Name = "Cases Verified";
            dgv.Columns[11].Name = "Records Verified";
            dgv.Columns[12].Name = "Case Verification %";
            dgv.Columns[13].Name = "Total Keystrokes";
            dgv.Columns[14].Name = "Keystrokes Per Hour";
            dgv.Columns[15].Name = "Errors Per 1000 Keystrokes";

            ArrayList stats = GetAllSortedOperatorStatistics();

            stats = CondenseStatistics(stats,tabControl.SelectedIndex);

            if( stats.Count == 0 )
                return;

            if( checkBoxHideZeroRows.Checked )
            {
                for( int i = stats.Count - 1; i >= 0; i-- )
                {
                    if( ( (ReportStatistics)stats[i] ).TotalTime == 0 )
                        stats.RemoveAt(i);
                }
            }

            int rowNum;

            if( stats.Count > 1 ) // we'll add a summary row at the top
            {
                dgv.RowCount = stats.Count + 1;
                rowNum = 1;
            }

            else
            {
                dgv.RowCount = stats.Count;
                rowNum = 0;
            }

            int totalTime = 0,
                totalOperators = 0,
                totalAddTime = 0,
                totalModifyTime = 0,
                totalVerifyTime = 0,
                totalPauseTime = 0,
                totalCasesAdded = 0,
                totalRecordsAdded = 0,
                totalCasesVerified = 0,
                totalRecordsVerified = 0,
                totalKeystrokes = 0,
                totalBadKeystrokes = 0;

            foreach( ReportStatistics rs in stats )
            {
                totalTime += rs.TotalTime;
                totalOperators = Math.Max(totalOperators,rs.Operators);
                totalAddTime += rs.AddTime;
                totalModifyTime += rs.ModifyTime;
                totalVerifyTime += rs.VerifyTime;
                totalPauseTime += rs.PauseTime;
                totalCasesAdded += rs.CasesAdded;
                totalRecordsAdded += rs.RecordsAdded;
                totalCasesVerified += rs.CasesVerified;
                totalRecordsVerified += rs.RecordsVerified;
                totalKeystrokes += rs.Keystrokes;
                totalBadKeystrokes += rs.BadKeystrokes;


                dgv.Rows[rowNum].Cells[0].Value = DateToDisplayableTime(rs.StartTime,tabControl.SelectedIndex == 4);
                dgv.Rows[rowNum].Cells[1].Value = SecondsToDisplayableTime(rs.TotalTime);
                dgv.Rows[rowNum].Cells[2].Value = rs.Operators;
                
                if( rs.Operators > 0 )
                    dgv.Rows[rowNum].Cells[3].Value = SecondsToDisplayableTime(rs.TotalTime / rs.Operators);

                dgv.Rows[rowNum].Cells[4].Value = SecondsToDisplayableTime(rs.AddTime);
                dgv.Rows[rowNum].Cells[5].Value = SecondsToDisplayableTime(rs.ModifyTime);
                dgv.Rows[rowNum].Cells[6].Value = SecondsToDisplayableTime(rs.VerifyTime);
                dgv.Rows[rowNum].Cells[7].Value = SecondsToDisplayableTime(rs.PauseTime);
                dgv.Rows[rowNum].Cells[8].Value = rs.CasesAdded;
                dgv.Rows[rowNum].Cells[9].Value = rs.RecordsAdded;
                dgv.Rows[rowNum].Cells[10].Value = rs.CasesVerified;
                dgv.Rows[rowNum].Cells[11].Value = rs.RecordsVerified;

                if( rs.CasesAdded > 0 )
                    dgv.Rows[rowNum].Cells[12].Value = FormatPercent(rs.CasesVerified * 100.0 / rs.CasesAdded);
                
                dgv.Rows[rowNum].Cells[13].Value = rs.Keystrokes;
                
                if( rs.TotalTime > 0 )
                    dgv.Rows[rowNum].Cells[14].Value = (int)(rs.Keystrokes / ( rs.TotalTime / 3600.0 ));

                if( rs.Keystrokes > 0 )
                    dgv.Rows[rowNum].Cells[15].Value = rs.BadKeystrokes * 1000 / rs.Keystrokes;

                rowNum++;
            }

            if( stats.Count > 1 ) // fill out the total row
            {
                dgv.Rows[0].Cells[0].Value = "Total";
                dgv.Rows[0].Cells[1].Value = SecondsToDisplayableTime(totalTime);
                dgv.Rows[0].Cells[2].Value = totalOperators;

                if( totalOperators > 0 )
                    dgv.Rows[0].Cells[3].Value = SecondsToDisplayableTime(totalTime / totalOperators);

                dgv.Rows[0].Cells[4].Value = SecondsToDisplayableTime(totalAddTime);
                dgv.Rows[0].Cells[5].Value = SecondsToDisplayableTime(totalModifyTime);
                dgv.Rows[0].Cells[6].Value = SecondsToDisplayableTime(totalVerifyTime);
                dgv.Rows[0].Cells[7].Value = SecondsToDisplayableTime(totalPauseTime);
                dgv.Rows[0].Cells[8].Value = totalCasesAdded;
                dgv.Rows[0].Cells[9].Value = totalRecordsAdded;
                dgv.Rows[0].Cells[10].Value = totalCasesVerified;
                dgv.Rows[0].Cells[11].Value = totalRecordsVerified;

                if( totalCasesAdded > 0 )
                    dgv.Rows[0].Cells[12].Value = FormatPercent(totalCasesVerified * 100.0 / totalCasesAdded);

                dgv.Rows[0].Cells[13].Value = totalKeystrokes;

                if( totalTime > 0 )
                    dgv.Rows[0].Cells[14].Value = (int)( totalKeystrokes / ( totalTime / 3600.0 ) );

                if( totalKeystrokes > 0 )
                    dgv.Rows[0].Cells[15].Value = totalBadKeystrokes * 1000 / totalKeystrokes;

                for( int j = 0; j < dgv.Columns.Count; j++ )
                    dgv.Rows[0].Cells[j].Style.BackColor = Color.Bisque;
            }

            tabPage.Controls.Add(dgv);
        }

        string RemoveNonStringChars(string number)
        {
            string newNumber = "";

            for( int i = 0; i < number.Length; i++ )
            {
                if( char.IsDigit(number[i]) || number[i] == '.' )
                    newNumber = newNumber + number[i];
            }

            return newNumber;
        }
        
        private void dataGridView_SortCompare(object sender,DataGridViewSortCompareEventArgs e)
        {
            if( e.CellValue1 == null || e.CellValue2 == null )
                return;

            // do a natural sort if the first character is a digit
            string str1 = e.CellValue1.ToString();
            string str2 = e.CellValue2.ToString();

            if( str1.Length == 0 || !char.IsNumber(str1[0]) || str2.Length == 0 || !char.IsNumber(str2[0]) )
                return; // we won't handle the sort

            if( str1.IndexOf('-') >= 0 ) // we won't handle dates
                return;

            int colonPos1 = str1.IndexOf(':');
            int colonPos2 = str2.IndexOf(':');

            double dbl1,dbl2;

            if( colonPos1 >= 0 && colonPos2 >= 0 ) // we're sorting a time
            {
                dbl1 = double.Parse(str1.Substring(0,colonPos1)) + double.Parse(str1.Substring(colonPos1 + 1)) / 60;
                dbl2 = double.Parse(str2.Substring(0,colonPos2)) + double.Parse(str2.Substring(colonPos2 + 1)) / 60;
            }

            else
            {
                dbl1 = double.Parse(RemoveNonStringChars(str1));
                dbl2 = double.Parse(RemoveNonStringChars(str2));
            }

            if( dbl1 > dbl2 )
                e.SortResult = 1;

            else if( dbl1 < dbl2 )
                e.SortResult = -1;

            else
                e.SortResult = 0;

            e.Handled = true;
        }


        bool IsKeyerActive(ArrayList stats)
        {
            DateTime lastWeek = DateTime.Now.AddDays(-7);

            for( int i = stats.Count - 1; i >= 0; i-- )
            {
                if( ( (OperatorStatistics)stats[i] ).EndTime >= lastWeek )
                    return true;
            }

            return false;
        }

        ArrayList GetAllSortedOperatorStatistics()
        {
            ArrayList stats = new ArrayList();

            foreach( var kvp in statisticsByOperator )
            {
                if( operatorsTreeView.SelectedNode.Index == 0 ) // all keyers
                {
                }

                else if( operatorsTreeView.SelectedNode.Index == 1 ) // currently active keyers only
                {
                    if( !IsKeyerActive(kvp.Value) )
                        continue;
                }

                else if( !kvp.Key.Equals(operatorsTreeView.SelectedNode.Tag) )
                    continue;

                stats.AddRange(kvp.Value);
            }

            stats.Sort(new OperatorStatisticsDateSorter());

            return stats;
        }

        ArrayList CondenseStatistics(ArrayList stats,int condenseType) // 2 = day, 3 = week, 4 = month
        {
            ArrayList condensedStats = new ArrayList();

            if( stats.Count == 0 )
                return condensedStats;

            DateTime startTime = ((OperatorStatistics)stats[0]).StartTime;
            DateTime startPeriod;
            DateTime endPeriod;

            if( condenseType == 2 )
                startPeriod = new DateTime(startTime.Year,startTime.Month,startTime.Day);

            else if( condenseType == 3 )
            {
                startPeriod = new DateTime(startTime.Year,startTime.Month,startTime.Day);
                
                while( startPeriod.DayOfWeek != DayOfWeek.Sunday )
                    startPeriod = startPeriod.AddDays(-1);
            }

            else
                startPeriod = new DateTime(startTime.Year,startTime.Month,1);

            endPeriod = GetNextPeriod(startPeriod,condenseType);

            ReportStatistics reportStats = new ReportStatistics(startPeriod);
            condensedStats.Add(reportStats);

            foreach( OperatorStatistics os in stats )
            {
                while( os.StartTime > endPeriod )
                {
                    startPeriod = endPeriod;
                    endPeriod = GetNextPeriod(startPeriod,condenseType);

                    reportStats = new ReportStatistics(startPeriod);
                    condensedStats.Add(reportStats);
                }

                reportStats.AddStatistics(os);
            }

            return condensedStats;
        }

        DateTime GetNextPeriod(DateTime startPeriod,int condenseType)
        {
            switch( condenseType )
            {
                case 2:
                    return startPeriod.AddDays(1);
                
                case 3:
                    return startPeriod.AddDays(7);

                default:
                    return startPeriod.AddMonths(1);
            }
        }

        void ApplyLogFilters(ArrayList stats,bool timeOnly)
        {
            DateTime endDate = dateTimePickerEnd.Value.AddDays(1); // because there is a time component of the date

            for( int i = stats.Count - 1; i >= 0; i-- )
            {
                OperatorStatistics os = (OperatorStatistics)stats[i];

                if( !timeOnly )
                {
                    if( ( os.Mode == EntryMode.Add && !checkBoxAdd.Checked ) ||
                        ( os.Mode == EntryMode.Modify && !checkBoxModify.Checked ) ||
                        ( os.Mode == EntryMode.Verify && !checkBoxVerify.Checked ) )
                    {
                        stats.RemoveAt(i);
                        continue;
                    }
                }

                if( os.StartTime < dateTimePickerStart.Value || os.StartTime > endDate )
                    stats.RemoveAt(i);
            }
        }

        void CreateLogsReport()
        {
            TabPage tabPage = tabControl.TabPages[tabControl.SelectedIndex];
            tabPage.Controls.Clear();

            DataGridView dgv = CreateDataGridView();
            dgv.ColumnCount = 10;

            dgv.Columns[0].Name = "Entry Mode";
            dgv.Columns[1].Name = "Operator ID";
            dgv.Columns[2].Name = "Start Date";
            dgv.Columns[3].Name = "Entry Time";
            dgv.Columns[4].Name = "Pause Time";
            dgv.Columns[5].Name = "Cases";
            dgv.Columns[6].Name = "Records";
            dgv.Columns[7].Name = "Keystrokes";
            dgv.Columns[8].Name = "Keystrokes Per Hour";
            dgv.Columns[9].Name = "Errors Per 1000 Keystrokes";

            ArrayList stats = GetAllSortedOperatorStatistics();

            ApplyLogFilters(stats,false);

            if( stats.Count == 0 )
                return;

            dgv.RowCount = stats.Count;

            for( int rowNum = 0; rowNum < stats.Count; rowNum++ )
            {
                OperatorStatistics os = (OperatorStatistics)stats[rowNum];

                int keyingTime = os.KeyingTime - os.PauseTime;

                dgv.Rows[rowNum].Cells[0].Value = os.ModeString;
                dgv.Rows[rowNum].Cells[1].Value = os.ID;
                dgv.Rows[rowNum].Cells[2].Value = DateToDisplayableTime(os.StartTime,false) + " " + os.StartTime.ToShortTimeString();
                dgv.Rows[rowNum].Cells[3].Value = SecondsToDisplayableTime(keyingTime);
                dgv.Rows[rowNum].Cells[4].Value = SecondsToDisplayableTime(os.PauseTime);
                dgv.Rows[rowNum].Cells[5].Value = os.Cases;
                dgv.Rows[rowNum].Cells[6].Value = os.Records;
                dgv.Rows[rowNum].Cells[7].Value = os.Keystrokes;

                if( keyingTime > 0 )
                    dgv.Rows[rowNum].Cells[8].Value = (int)( os.Keystrokes / ( keyingTime / 3600.0 ) );

                if( os.Keystrokes > 0 )
                    dgv.Rows[rowNum].Cells[9].Value = os.BadKeystrokes * 1000 / os.Keystrokes;
            }

            tabPage.Controls.Add(dgv);
        }

        void CreateReportByOperator()
        {
            TabPage tabPage = tabControl.TabPages[tabControl.SelectedIndex];
            tabPage.Controls.Clear();

            DataGridView dgv = CreateDataGridView();
            dgv.ColumnCount = 13;

            dgv.Columns[0].Name = "Operator ID";
            dgv.Columns[1].Name = "Total Entry Time";
            dgv.Columns[2].Name = "Add Time";
            dgv.Columns[3].Name = "Modify Time";
            dgv.Columns[4].Name = "Verify Time";
            dgv.Columns[5].Name = "Pause Time";
            dgv.Columns[6].Name = "Cases Added";
            dgv.Columns[7].Name = "Records Added";
            dgv.Columns[8].Name = "Cases Verified";
            dgv.Columns[9].Name = "Records Verified";
            dgv.Columns[10].Name = "Total Keystrokes";
            dgv.Columns[11].Name = "Keystrokes Per Hour";
            dgv.Columns[12].Name = "Errors Per 1000 Keystrokes";

            SortedDictionary<string,ArrayList> relevantStatsByOper = GetStatisticsByOperator();

            if( statisticsByOperator.Count == 0 )
                return;

            int rowNum;

            if( relevantStatsByOper.Count > 1 ) // we'll add a summary row at the top
            {
                dgv.RowCount = relevantStatsByOper.Count + 1;
                rowNum = 1;
            }

            else
            {
                dgv.RowCount = relevantStatsByOper.Count;
                rowNum = 0;
            }

            int totalTime = 0,
                totalAddTime = 0,
                totalModifyTime = 0,
                totalVerifyTime = 0,
                totalPauseTime = 0,
                totalCasesAdded = 0,
                totalRecordsAdded = 0,
                totalCasesVerified = 0,
                totalRecordsVerified = 0,
                totalKeystrokes = 0,
                totalBadKeystrokes = 0;

            foreach( var kvp in relevantStatsByOper )
            {
                ArrayList stats = kvp.Value;

                int operTime = 0,
                    operAddTime = 0,
                    operModifyTime = 0,
                    operVerifyTime = 0,
                    operPauseTime = 0,
                    operCasesAdded = 0,
                    operRecordsAdded = 0,
                    operCasesVerified = 0,
                    operRecordsVerified = 0,
                    operKeystrokes = 0,
                    operBadKeystrokes = 0;

                foreach( OperatorStatistics os in stats )
                {
                    if( os.Mode == EntryMode.Add )
                    {
                        operAddTime += os.KeyingTime - os.PauseTime;
                        operCasesAdded += os.Cases;
                        operRecordsAdded += os.Records;
                    }
                    
                    else if( os.Mode == EntryMode.Modify )
                        operModifyTime += os.KeyingTime - os.PauseTime;

                    else
                    {
                        operVerifyTime += os.KeyingTime - os.PauseTime;
                        operCasesVerified+= os.Cases;
                        operRecordsVerified += os.Records;
                    }

                    operPauseTime += os.PauseTime;
                    operKeystrokes += os.Keystrokes;
                    operBadKeystrokes += os.BadKeystrokes;
                }

                operTime = operAddTime + operModifyTime + operVerifyTime;

                totalTime += operTime;
                totalAddTime += operAddTime;
                totalModifyTime += operModifyTime;
                totalVerifyTime += operVerifyTime;
                totalPauseTime += operPauseTime;
                totalCasesAdded += operCasesAdded;
                totalRecordsAdded += operRecordsAdded;
                totalCasesVerified += operCasesVerified;
                totalRecordsVerified += operRecordsVerified;
                totalKeystrokes += operKeystrokes;
                totalBadKeystrokes += operBadKeystrokes;

                string operatorName = GetOperatorName(kvp.Key);

                dgv.Rows[rowNum].Cells[0].Value = operatorName;
                dgv.Rows[rowNum].Cells[1].Value = SecondsToDisplayableTime(operTime);
                dgv.Rows[rowNum].Cells[2].Value = SecondsToDisplayableTime(operAddTime);
                dgv.Rows[rowNum].Cells[3].Value = SecondsToDisplayableTime(operModifyTime);
                dgv.Rows[rowNum].Cells[4].Value = SecondsToDisplayableTime(operVerifyTime);
                dgv.Rows[rowNum].Cells[5].Value = SecondsToDisplayableTime(operPauseTime);
                dgv.Rows[rowNum].Cells[6].Value = operCasesAdded;
                dgv.Rows[rowNum].Cells[7].Value = operRecordsAdded;
                dgv.Rows[rowNum].Cells[8].Value = operCasesVerified;
                dgv.Rows[rowNum].Cells[9].Value = operRecordsVerified;
                dgv.Rows[rowNum].Cells[10].Value = operKeystrokes;

                if( operTime > 0 )
                    dgv.Rows[rowNum].Cells[11].Value = (int)( operKeystrokes / ( operTime / 3600.0 ) );

                if( operKeystrokes > 0 )
                    dgv.Rows[rowNum].Cells[12].Value = operBadKeystrokes * 1000 / operKeystrokes;

                rowNum++;
            }

            if( relevantStatsByOper.Count > 1 ) // fill out the total row
            {
                dgv.Rows[0].Cells[0].Value = "Total";
                dgv.Rows[0].Cells[1].Value = SecondsToDisplayableTime(totalTime);
                dgv.Rows[0].Cells[2].Value = SecondsToDisplayableTime(totalAddTime);
                dgv.Rows[0].Cells[3].Value = SecondsToDisplayableTime(totalModifyTime);
                dgv.Rows[0].Cells[4].Value = SecondsToDisplayableTime(totalVerifyTime);
                dgv.Rows[0].Cells[5].Value = SecondsToDisplayableTime(totalPauseTime);
                dgv.Rows[0].Cells[6].Value = totalCasesAdded;
                dgv.Rows[0].Cells[7].Value = totalRecordsAdded;
                dgv.Rows[0].Cells[8].Value = totalCasesVerified;
                dgv.Rows[0].Cells[9].Value = totalRecordsVerified;
                dgv.Rows[0].Cells[10].Value = totalKeystrokes;

                if( totalTime > 0 )
                    dgv.Rows[0].Cells[11].Value = (int)( totalKeystrokes / ( totalTime / 3600.0 ) );

                if( totalKeystrokes > 0 )
                    dgv.Rows[0].Cells[12].Value = totalBadKeystrokes * 1000 / totalKeystrokes;

                for( int j = 0; j < dgv.Columns.Count; j++ )
                    dgv.Rows[0].Cells[j].Style.BackColor = Color.Bisque;
            }

            tabPage.Controls.Add(dgv);
        }

        SortedDictionary<string,ArrayList> GetStatisticsByOperator()
        {
            SortedDictionary<string,ArrayList> relevantStatsByOper = new SortedDictionary<string,ArrayList>();

            foreach( var kvp in statisticsByOperator )
            {
                if( operatorsTreeView.SelectedNode.Index == 0 ) // all keyers
                {
                }

                else if( operatorsTreeView.SelectedNode.Index == 1 ) // currently active keyers only
                {
                    if( !IsKeyerActive(kvp.Value) )
                        continue;
                }

                else if( !kvp.Key.Equals(operatorsTreeView.SelectedNode.Tag) )
                    continue;

                ArrayList stats = new ArrayList();
                stats.AddRange(kvp.Value);
                ApplyLogFilters(stats,true);

                if( stats.Count > 0 )
                    relevantStatsByOper.Add(kvp.Key,stats);
            }

            return relevantStatsByOper;
        }


        int ModifyDatesAndGetIntervals(ref DateTime earliestDate,ref DateTime latestDate)
        {
            if( comboBoxInterval.SelectedIndex == 0 ) // monthly
            {
                earliestDate = new DateTime(earliestDate.Year,earliestDate.Month,1);
                latestDate = GetEndOfMonth(new DateTime(latestDate.Year,latestDate.Month,1));
                return ( latestDate.Year * 12 + latestDate.Month ) - ( earliestDate.Year * 12 + earliestDate.Month ) + 1;
            }

            
            earliestDate = new DateTime(earliestDate.Year,earliestDate.Month,earliestDate.Day); // modify the times
            latestDate = new DateTime(latestDate.Year,latestDate.Month,latestDate.Day,23,59,59);

            if( comboBoxInterval.SelectedIndex == 1 ) // weekly
            {
                while( earliestDate.DayOfWeek != DayOfWeek.Sunday )
                    earliestDate = earliestDate.AddDays(-1);

                while( latestDate.DayOfWeek != DayOfWeek.Sunday )
                    latestDate = latestDate.AddDays(-1);

                return ( latestDate.Subtract(earliestDate).Days / 7 ) + 1;
            }

            else // daily
                return latestDate.Subtract(earliestDate).Days + 1;
        }

        int GetDateIntervalNumber(DateTime earliestDate,DateTime date)
        {
            if( comboBoxInterval.SelectedIndex == 0 ) // monthly
                return ( date.Year * 12 + date.Month ) - ( earliestDate.Year * 12 + earliestDate.Month );

            else if( comboBoxInterval.SelectedIndex == 1 ) // weekly
                return date.Subtract(earliestDate).Days / 7;

            else
                return date.Subtract(earliestDate).Days;
        }

        void CreatePayrollReport()
        {
            TabPage tabPage = tabControl.TabPages[tabControl.SelectedIndex];
            tabPage.Controls.Clear();

            SortedDictionary<string,ArrayList> relevantStatsByOper = GetStatisticsByOperator();

            if( relevantStatsByOper.Count == 0 )
                return;

            DateTime earliestDate = DateTime.MaxValue;
            DateTime latestDate = DateTime.MinValue;

            // sort each operator's statistics
            foreach( var kvp in relevantStatsByOper )
            {
                kvp.Value.Sort(new OperatorStatisticsDateSorter());

                if( ( (OperatorStatistics)kvp.Value[0] ).StartTime < earliestDate )
                    earliestDate = ( (OperatorStatistics)kvp.Value[0] ).StartTime;

                if( ( (OperatorStatistics)kvp.Value[kvp.Value.Count - 1] ).StartTime > latestDate )
                    latestDate = ( (OperatorStatistics)kvp.Value[kvp.Value.Count - 1] ).StartTime;
            }

            int numColumns = ModifyDatesAndGetIntervals(ref earliestDate,ref latestDate);

            int[,] workArray = new int[relevantStatsByOper.Count + 1,numColumns]; // + 1 for the total row
            int rowNum = 1;

            foreach( var kvp in relevantStatsByOper )
            {
                foreach( OperatorStatistics os in kvp.Value )
                {
                    int interval = GetDateIntervalNumber(earliestDate,os.StartTime);
                    int actualKeyingTime = os.KeyingTime - os.PauseTime;
                    workArray[0,interval] += actualKeyingTime;
                    workArray[rowNum,interval] += actualKeyingTime;
                }

                rowNum++;
            }

            int zeroRows = 0;

            for( int i = 0; i < numColumns; i++ )
            {
                if( workArray[0,i] == 0 )
                    zeroRows++;
            }

            DataGridView dgv = CreateDataGridView();
            dgv.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.ColumnHeader; // Fill is crazy with many columns
            dgv.ColumnCount = 2 + ( checkBoxHideZeroRows.Checked ? ( numColumns - zeroRows ) : numColumns );
            dgv.SetBounds(0,0,tabPage.Width,tabPage.Height);  // adds a horizontal scrollbar if needed

            dgv.Columns[0].Name = "Operator ID";
            dgv.Columns[1].Name = "Total Entry Time";

            bool useTotalRow = relevantStatsByOper.Count > 1 ? true : false; // total, average, and number active keyer rows
            
            dgv.RowCount = ( useTotalRow ? 3 : 0 ) + relevantStatsByOper.Count;

            rowNum = 0;

            if( useTotalRow )
            {
                dgv.Rows[0].Cells[0].Value = "Total";
                dgv.Rows[1].Cells[0].Value = "Average";
                dgv.Rows[2].Cells[0].Value = "Active Keyers";
                rowNum += 3;
            }

            foreach( var kvp in relevantStatsByOper )
                dgv.Rows[rowNum++].Cells[0].Value = GetOperatorName(kvp.Key);

            DateTime columnDate = earliestDate;
            int columnNum = 2;

            // GetNextPeriod, 2 = days, 3 = weeks, 4 = months, so it equals 4 - comboBoxInterval.SelectedIndex
            for( int c = 0; c < numColumns; c++, columnDate = GetNextPeriod(columnDate,4 - comboBoxInterval.SelectedIndex) )
            {
                if( checkBoxHideZeroRows.Checked && workArray[0,c] == 0 )
                    continue;

                dgv.Columns[columnNum].Name = DateToDisplayableTime(columnDate,comboBoxInterval.SelectedIndex == 0);
            
                rowNum = 0;

                int activeKeyers = 0;

                for( int r = ( useTotalRow ? 0 : 1 ); r < relevantStatsByOper.Count + 1; r++ )
                {
                    dgv.Rows[rowNum].Cells[columnNum].Value = SecondsToDisplayableTime(workArray[r,c]);

                    if( useTotalRow )
                    {
                        if( rowNum == 0 ) // skip past the average and number of keyers rows
                            rowNum = 2;

                        else if( workArray[r,c] > 0 )
                            activeKeyers++;
                    }

                    rowNum++;
                }

                if( useTotalRow )
                {
                    if( activeKeyers > 0 )
                        dgv.Rows[1].Cells[columnNum].Value = SecondsToDisplayableTime(workArray[0,c] / activeKeyers);

                    dgv.Rows[2].Cells[columnNum].Value = activeKeyers;
                }

                columnNum++;
            }

            // get the total counts per operator
            rowNum = 0;
            int allTotalTime = 0;

            for( int r = ( useTotalRow ? 0 : 1 ); r < relevantStatsByOper.Count + 1; r++ )
            {
                int totalTime = 0;

                for( int c = 0; c < numColumns; c++ )
                    totalTime += workArray[r,c];

                if( !useTotalRow || r > 0 ) // don't add in the total row to these counts
                    allTotalTime += totalTime;

                dgv.Rows[rowNum].Cells[1].Value = SecondsToDisplayableTime(totalTime);

                if( useTotalRow )
                {
                    if( rowNum == 0 ) // skip past the average and number of keyers rows
                    {
                        dgv.Rows[2].Cells[1].Value = relevantStatsByOper.Count;
                        rowNum = 2;
                    }
                }

                rowNum++;
            }

            if( useTotalRow )
            {
                dgv.Rows[1].Cells[1].Value = SecondsToDisplayableTime(allTotalTime / relevantStatsByOper.Count);

                for( int j = 0; j < dgv.Columns.Count; j++ )
                {
                    dgv.Rows[0].Cells[j].Style.BackColor = Color.Bisque;
                    dgv.Rows[1].Cells[j].Style.BackColor = Color.Cornsilk;
                    dgv.Rows[2].Cells[j].Style.BackColor = Color.Linen;
                }
            }

            tabPage.Controls.Add(dgv);
        }

        void CreateSummaryReport()
        {
            if( summaryReport == null ) // generate the overall statistics
                summaryReport = new SummaryReport(statisticsByOperator,operationExpectedCases,operationDesiredEndDate);

            bool someCasesExist = statisticsByOperator.Count > 0;
            bool futurePredictionsPossible = someCasesExist && operationExpectedCases > 0;
            bool futureDetailedPredictionsPossible = futurePredictionsPossible && operationExpectedCases > summaryReport.CasesEntered;
            bool futureDetailedDatePredictionsPossible = futureDetailedPredictionsPossible && operationDesiredEndDate > DateTime.Now;
            bool showIndividualOperatorStatistics = someCasesExist && operatorsTreeView.SelectedNode.Index > 0;

            SummaryReport.Statistics summaryReportOperator = null;

            if( showIndividualOperatorStatistics )
            {
                ArrayList stats = GetAllSortedOperatorStatistics();

                if( stats.Count > 0 )
                    summaryReportOperator =  summaryReport.CreateOperatorStatistics(stats);

                else
                    showIndividualOperatorStatistics = false;
            }

            // show / hide the values first
            progressBarSummary.Value = 0;
            progressBarSummary.Visible = someCasesExist;            
            progressBarSummary.Enabled = futurePredictionsPossible;
            labelCasesEntered.Visible = someCasesExist;
            labelCasesRemaining.Visible = futurePredictionsPossible;
            labelExpectedCases.Visible = futurePredictionsPossible;

            label2.Visible = someCasesExist; // All Time
            label18.Visible = someCasesExist; // Last Week of Keying
            label19.Visible = showIndividualOperatorStatistics; // Selected Operator(s)

            label6.Visible = someCasesExist; // First date of data entry:
            label4.Visible = someCasesExist; // Total number of keyers:
            label20.Visible = someCasesExist; // Last date of data entry:
            label17.Visible = someCasesExist; // Cases entered:
            label3.Visible = someCasesExist; // Days that data was entered:
            label5.Visible = someCasesExist; // Average number of hours spent in add mode in a day:
            label9.Visible = someCasesExist; // Average number of keyers working during a day:
            label10.Visible = someCasesExist; // Weeks that data was entered:
            label8.Visible = someCasesExist; // Average number of hours spent in add mode in a week:
            label7.Visible = someCasesExist; // Average number of keyers working during a week:
            label11.Visible = someCasesExist; // Average time needed to enter a case (in minutes):
            label12.Visible = someCasesExist; // Average keying rate (keystrokes per hour):
            label13.Visible = futureDetailedPredictionsPossible; // Expected number of keying days remaining:
            label14.Visible = futureDetailedPredictionsPossible; // Expected completion date at current rates:
            label15.Visible = futureDetailedDatePredictionsPossible; // Keyers needed to complete on time using current keying rate:
            label16.Visible = futureDetailedDatePredictionsPossible; // Keying rate needed to complete on time using current number of keyers:

            for( int i = 0; i < 16; i++ )
            {
                bool show = i < 12 || ( i < 14 && futureDetailedPredictionsPossible ) || futureDetailedDatePredictionsPossible;
                summaryReportLabels[i,0].Visible = show && someCasesExist;
                summaryReportLabels[i,1].Visible = show && someCasesExist;
                summaryReportLabels[i,2].Visible = show && showIndividualOperatorStatistics &&  ( i < 12 || i == 14 );
            }

            if( someCasesExist ) // fill out the report
            {
                labelCasesEntered.Text = "Cases Entered: " + summaryReport.CasesEntered;

                if( futurePredictionsPossible )
                {
                    labelCasesEntered.Text = String.Format("{0} ({1:N1}%)",labelCasesEntered.Text,100.0 * summaryReport.CasesEntered / operationExpectedCases);

                    progressBarSummary.Value = Math.Min(100,(int)( 100.0 * summaryReport.CasesEntered / operationExpectedCases ));

                    labelExpectedCases.Text = "Expected Number of Cases: " + operationExpectedCases;
                    labelExpectedCases.Location = new Point(progressBarSummary.Location.X + progressBarSummary.Size.Width - labelExpectedCases.Size.Width,labelExpectedCases.Location.Y);

                    labelCasesRemaining.Text = String.Format("Cases Remaining: {0} ({1:N1}%)",summaryReport.CasesRemaining,100.0 * summaryReport.CasesRemaining / operationExpectedCases); ;
                    labelCasesRemaining.Location = new Point( ( progressBarSummary.Location.X + progressBarSummary.Size.Width - labelExpectedCases.Size.Width ) / 2,labelCasesRemaining.Location.Y);
                }

                for( int c = ( showIndividualOperatorStatistics ? 2 : 1 ); c >= 0; c-- )
                {
                    SummaryReport.Statistics stats = c == 0 ? summaryReport.AllTime : c == 1 ? summaryReport.LastWeek : summaryReportOperator;

                    int r = futureDetailedDatePredictionsPossible ? 15 : futureDetailedDatePredictionsPossible ? 13 : 11;

                    for( ; r >= 0; r-- )
                    {
                        switch( r )
                        {
                            case 0:
                                summaryReportLabels[r,c].Text = DateToDisplayableTime(stats.StartDate,false);
                                break;

                            case 1:
                                summaryReportLabels[r,c].Text = DateToDisplayableTime(stats.EndDate,false);
                                break;

                            case 2:
                                summaryReportLabels[r,c].Text = stats.NumberOperators.ToString();
                                break;

                            case 3:
                                summaryReportLabels[r,c].Text = String.Format("{0:N0}",stats.CasesEntered);
                                break;

                            case 4:
                                summaryReportLabels[r,c].Text = stats.DaysWorked.ToString();
                                break;

                            case 5:
                                summaryReportLabels[r,c].Text = HoursToDisplayableTime(stats.HoursPerDay);
                                break;

                            case 6:
                                summaryReportLabels[r,c].Text = String.Format("{0:N1}",stats.OperatorsPerDay);
                                break;

                            case 7:
                                summaryReportLabels[r,c].Text = stats.WeeksWorked.ToString();
                                break;

                            case 8:
                                summaryReportLabels[r,c].Text = HoursToDisplayableTime(stats.HoursPerWeek);
                                break;

                            case 9:
                                summaryReportLabels[r,c].Text = String.Format("{0:N1}",stats.OperatorsPerWeek);
                                break;

                            case 10:
                                summaryReportLabels[r,c].Text = HoursToDisplayableTime(stats.SecondsPerCase / 60); // even though the functions is Hours..., this is a minutes measure
                                break;

                            case 11:
                                summaryReportLabels[r,c].Text = String.Format("{0:N0}",stats.KeystrokesPerHour);
                                break;

                            case 12:
                                summaryReportLabels[r,c].Text = String.Format("{0:N1}",stats.ExpectedDaysRemaining);
                                break;

                            case 13:
                                summaryReportLabels[r,c].Text = DateToDisplayableTime(stats.ExpectedCompletionDate,false);
                                break;

                            case 14:
                                summaryReportLabels[r,c].Text = String.Format("{0:N1}",stats.KeyersNeededAtCurrentRate);
                                break;

                            case 15:
                                summaryReportLabels[r,c].Text = String.Format("{0:N0}",stats.KeyingRateNeededWithCurrentKeyers);
                                break;
                        }
                    }
                }
            }
        }

        Label[,] SetupSummaryReportLabels() // add the labels to the summary page
        {
            int[] yPos = new int[16] {    label6.Location.Y,
                                            label20.Location.Y,
                                            label4.Location.Y,
                                            label17.Location.Y,
                                            label3.Location.Y,
                                            label5.Location.Y,
                                            label9.Location.Y,
                                            label10.Location.Y,
                                            label8.Location.Y,
                                            label7.Location.Y,
                                            label11.Location.Y,
                                            label12.Location.Y,
                                            label13.Location.Y,
                                            label14.Location.Y,
                                            label15.Location.Y,
                                            label16.Location.Y
                                        };

            Label[,] labels = new Label[16,3];

            for( int c = 0; c < 3; c++ )
            {
                int xPos = c == 0 ? label2.Location.X : c == 1 ? label18.Location.X : label19.Location.X;

                for( int r = 0; r < 16; r++ )
                {
                    Label label = new Label();
                    label.Location = new Point(xPos,yPos[r]);
                    label.Size = new System.Drawing.Size(80,13);
                    tabControl.TabPages[0].Controls.Add(label);
                    labels[r,c] = label;
                }
            }

            return labels;
        }

        void AddFiles(string[] filenames,out int fileErrors,out int filesAlreadyAdded)
        {
            fileErrors = 0;
            filesAlreadyAdded = 0;

            System.Windows.Forms.Cursor prevCursor = System.Windows.Forms.Cursor.Current;
            System.Windows.Forms.Cursor.Current = Cursors.WaitCursor;

            foreach( string filename in filenames )
            {
                if( openedFiles.Contains(filename) )
                    filesAlreadyAdded++;

                else
                {
                    try
                    {
                        OperatorStatisticsFile osf = new OperatorStatisticsFile(filename);
                        openedFiles.Add(filename,null);

                        foreach( OperatorStatistics os in osf.Statistics )
                        {
                            ArrayList stats;

                            if( statisticsByOperator.ContainsKey(os.ID) )
                                stats = statisticsByOperator[os.ID];

                            else
                            {
                                stats = new ArrayList();
                                statisticsByOperator.Add(os.ID,stats);
                            }

                            stats.Add(os);
                        }
                    }

                    catch( Exception e )
                    {
                        fileErrors++;
                        MessageBox.Show("Could not open " + filename.Substring(filename.LastIndexOf('\\') + 1) + " due to the following error:\n\n" + e.Message,applicationName);
                    }
                }
            }

            System.Windows.Forms.Cursor.Current = prevCursor;

            summaryReport = null; // the old report is no longer valid
        }

        private void MainForm_Shown(object sender,EventArgs e)
        {
            Array commandArgs = Environment.GetCommandLineArgs();

            if( commandArgs.Length >= 2 )
            {
                string filename = (string)commandArgs.GetValue(1);

                if( Path.GetExtension(filename).Equals(".osvlog",StringComparison.CurrentCultureIgnoreCase) )
                    OpenViewingConfiguration(filename);

                else
                {
                    int fileErrors,filesAlreadyAdded;
                    AddFiles(new string[] { filename },out fileErrors,out filesAlreadyAdded);
                    viewingConfig.AddSingle(filename);

                    if( fileErrors == 0 )
                    {
                        UpdateApplicationTitle();
                        UpdateOperators();
                        UpdateView();
                    }
                }
            }
        }

        void AddLogs(string[] filenames)
        {
            int fileErrors,filesAlreadyAdded;

            AddFiles(filenames,out fileErrors,out filesAlreadyAdded);

            string errorMessage = "";

            if( fileErrors > 0 )
                errorMessage = String.Format("{0} file{1} not processed because there were file errors.",fileErrors,PluralizeWord(fileErrors));

            if( filesAlreadyAdded > 0 )
                errorMessage += String.Format("{0}{1} file{2} not processed because {3} had already been added.",errorMessage.Length > 0 ? " " : "",filesAlreadyAdded,PluralizeWord(filesAlreadyAdded),filesAlreadyAdded > 1 ? "they" : "it");

            if( errorMessage.Length > 0 )
                MessageBox.Show(errorMessage,applicationName);

            if( filenames.Length != ( fileErrors + filesAlreadyAdded ) )
            {
                UpdateApplicationTitle();
                UpdateOperators();
                UpdateView();
            }
        }

        // File -> Add Log
        private void addLogToolStripMenuItem_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = statisticsByOperatorFilter;
            ofd.Multiselect = true;

            if( ofd.ShowDialog() == DialogResult.OK )
                AddLogs(ofd.FileNames);

            foreach( string filename in ofd.FileNames )
                viewingConfig.AddSingle(filename);
        }

        void BulkAddLogsProcessor(string pathname,bool useSubfolders,ref List<string> filenames)
        {
            DirectoryInfo dir = new DirectoryInfo(pathname);

            if( useSubfolders )
            {
                DirectoryInfo[] dirs = dir.GetDirectories();

                foreach( DirectoryInfo di in dirs )
                    BulkAddLogsProcessor(di.FullName,useSubfolders,ref filenames);
            }

            foreach( FileInfo fi in dir.GetFiles("*.log") )
                filenames.Add(fi.FullName);
        }

        void BulkAddLogs(string pathname,bool useSubfolders)
        {
            // construct a string array of the file names
            List<string> filenames = new List<string>();
            BulkAddLogsProcessor(pathname,useSubfolders,ref filenames);
            AddLogs(filenames.ToArray());
            viewingConfig.AddMultiple(pathname,useSubfolders);
        }

        // File -> Bulk Add Logs
        private void bulkAddLogsToolStripMenuItem_Click(object sender,EventArgs e)
        {
            BulkAddForm form = new BulkAddForm();

            if( form.ShowDialog() == DialogResult.OK )
                BulkAddLogs(form.FolderName,form.Subfolders);
        }

        // File -> Close All
        private void closeAllToolStripMenuItem_Click(object sender,EventArgs e)
        {
            viewingConfig.Clear();
            statisticsByOperator.Clear();
            openedFiles.Clear();
            summaryReport = null; // the old report is no longer valid
            UpdateApplicationTitle();
            UpdateOperators();
            UpdateView();
        }

        // File -> Close
        private void exitToolStripMenuItem_Click(object sender,EventArgs e)
        {
            Close();
        }

        private void checkBoxAdd_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void checkBoxModify_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void checkBoxVerify_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        DateTime GetEndOfMonth(DateTime start)
        {
            DateTime end = start.AddDays(31);

            while( end.Day != 1 )
                end = end.AddDays(-1);

            return end.AddDays(-1);
        }

        private void timePeriodComboBox_SelectedIndexChanged(object sender,EventArgs e)
        {
            bool oldDisableVal = disableUpdateView;
            disableUpdateView = true;

            if( comboBoxTimePeriod.SelectedIndex == 0 ) // all time
            {
                dateTimePickerStart.Value = new DateTime(2000,1,1);
                dateTimePickerEnd.Value = DateTime.Now;
            }

            else if( comboBoxTimePeriod.SelectedIndex == 1 ) // last month
            {
                if( DateTime.Now.Month == 1 )
                    dateTimePickerStart.Value = new DateTime(DateTime.Now.Year - 1,12,1);

                else
                    dateTimePickerStart.Value = new DateTime(DateTime.Now.Year,DateTime.Now.Month - 1,1);

                dateTimePickerEnd.Value = GetEndOfMonth(dateTimePickerStart.Value);
            }

            else if( comboBoxTimePeriod.SelectedIndex == 2 ) // this month
            {
                dateTimePickerStart.Value = new DateTime(DateTime.Now.Year,DateTime.Now.Month,1);
                dateTimePickerEnd.Value = GetEndOfMonth(dateTimePickerStart.Value);
            }
            
            else if( comboBoxTimePeriod.SelectedIndex == 3 ) // last week
            {
                dateTimePickerEnd.Value = DateTime.Now.AddDays(-7);

                while( dateTimePickerEnd.Value.DayOfWeek != DayOfWeek.Saturday )
                    dateTimePickerEnd.Value = dateTimePickerEnd.Value.AddDays(1);

                dateTimePickerStart.Value = dateTimePickerEnd.Value.AddDays(-6);
            }
            
            else if( comboBoxTimePeriod.SelectedIndex == 4 ) // this week
            {
                dateTimePickerEnd.Value = DateTime.Now;
                
                dateTimePickerStart.Value = dateTimePickerEnd.Value;
                while( dateTimePickerStart.Value.DayOfWeek != DayOfWeek.Sunday )
                    dateTimePickerStart.Value = dateTimePickerStart.Value.AddDays(-1);
            }



            else if( comboBoxTimePeriod.SelectedIndex == 5 ) // last seven days
            {
                dateTimePickerEnd.Value = DateTime.Now.AddDays(-1);
                dateTimePickerStart.Value = dateTimePickerEnd.Value.AddDays(-6);
            }

            else if( comboBoxTimePeriod.SelectedIndex == 6 ) // yesterday
            {
                dateTimePickerStart.Value = DateTime.Now.AddDays(-1);
                dateTimePickerEnd.Value = dateTimePickerStart.Value;
            }

            else if( comboBoxTimePeriod.SelectedIndex == 7 ) // today
            {
                dateTimePickerStart.Value = DateTime.Now;
                dateTimePickerEnd.Value = dateTimePickerStart.Value;
            }

            dateTimePickerStart.Enabled = comboBoxTimePeriod.SelectedIndex == 8;
            dateTimePickerEnd.Enabled = comboBoxTimePeriod.SelectedIndex == 8;

            disableUpdateView = oldDisableVal;
            UpdateView();
        }

        private void dateTimePickerStart_ValueChanged(object sender,EventArgs e)
        {
            bool oldDisableVal = disableUpdateView;
            disableUpdateView = true;

            if( dateTimePickerStart.Value > DateTime.Now )
                dateTimePickerStart.Value = DateTime.Now;

            if( dateTimePickerStart.Value > dateTimePickerEnd.Value )
                dateTimePickerEnd.Value = dateTimePickerStart.Value;

            disableUpdateView = oldDisableVal;
            UpdateView();
        }

        private void dateTimePickerEnd_ValueChanged(object sender,EventArgs e)
        {
            bool oldDisableVal = disableUpdateView;
            disableUpdateView = true;

            if( dateTimePickerEnd.Value > DateTime.Now )
                dateTimePickerEnd.Value = DateTime.Now;

            if( dateTimePickerStart.Value > dateTimePickerEnd.Value )
                dateTimePickerStart.Value = dateTimePickerEnd.Value;

            disableUpdateView = oldDisableVal;
            UpdateView();
        }

        private void operatorsTreeView_AfterSelect(object sender,TreeViewEventArgs e)
        {
            UpdateView();
        }

        private void checkBoxHideZeroRows_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void tabControl_SelectedIndexChanged(object sender,EventArgs e)
        {
            checkBoxAdd.Visible = tabControl.SelectedIndex == 1;
            checkBoxModify.Visible = tabControl.SelectedIndex == 1;
            checkBoxVerify.Visible = tabControl.SelectedIndex == 1;
            labelEntry.Visible = tabControl.SelectedIndex == 1;

            checkBoxHideZeroRows.Visible = ( tabControl.SelectedIndex >= 2 && tabControl.SelectedIndex <= 4 ) || tabControl.SelectedIndex == 6;

            bool showTimePeriod = tabControl.SelectedIndex == 1 || tabControl.SelectedIndex >= 5;
            comboBoxTimePeriod.Visible = showTimePeriod;
            dateTimePickerStart.Visible = showTimePeriod;
            dateTimePickerEnd.Visible = showTimePeriod;
            labelTimePeriod.Visible = showTimePeriod;

            labelInterval.Visible = tabControl.SelectedIndex == 6 || tabControl.SelectedIndex == 7;
            comboBoxInterval.Visible = tabControl.SelectedIndex == 6 || tabControl.SelectedIndex == 7;

            linkLabelCopyChart.Visible = tabControl.SelectedIndex == 7;
            checkBoxVisualizeShowLabels.Visible = tabControl.SelectedIndex == 7;
            checkBoxVisualizeSortXAxis.Visible = tabControl.SelectedIndex == 7;

            UpdateView();
        }

        // View -> Display Time in Decimal Format
        private void displayTimeInDecimalFormatToolStripMenuItem_Click(object sender,EventArgs e)
        {
            displayTimeInDecimalFormatToolStripMenuItem.Checked = !displayTimeInDecimalFormatToolStripMenuItem.Checked;
            UpdateView();
        }

        // View -> Display Operator IDs Instead of Names
        private void displayOperatorIDsInsteadOfNameToolStripMenuItem_Click(object sender,EventArgs e)
        {
            displayOperatorIDsInsteadOfNameToolStripMenuItem.Checked = !displayOperatorIDsInsteadOfNameToolStripMenuItem.Checked;
            UpdateOperators();
            UpdateView();
        }

        // Specify -> Operation Statistics
        private void operationStatisticsToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SpecifyOperationStatisticsForm form = new SpecifyOperationStatisticsForm(operationDesiredEndDate,operationExpectedCases);

            if( form.ShowDialog() == DialogResult.OK )
            {
                operationExpectedCases = form.UseParameters ? form.ExpectedCases : 0;
                operationDesiredEndDate = form.DesiredEndDate;
                summaryReport = null; // the old report is no longer valid
                UpdateView();
            }
        }

        // Specify -> Operator Names File
        private void operatorNamesFileToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SpecifyOperatorNamesFile form = new SpecifyOperatorNamesFile(operatorNamesFilename);

            if( form.ShowDialog() == DialogResult.OK )
            {
                operatorNamesFilename = form.UseNamesFile ? form.NamesFile : null;
                ParseOperatorNamesFile();
                UpdateOperators();
                UpdateView();
            }
        }

        void ParseOperatorNamesFile()
        {
            if( operatorNamesFilename == null )
                operatorsToNamesFileInfo = null;

            else
            {
                operatorsToNamesFileInfo = new Hashtable();

                try
                {
                    string[] fileContents = File.ReadAllLines(operatorNamesFilename);

                    if( fileContents.Length < 2 )
                        throw new Exception("The file does not have enough rows!");
                    
                    for( int i = 0; i < fileContents.Length; i++ )
                    {
                        if( i != 0 && fileContents[i].Trim().Length == 0 ) // the first row must not be blank
                            continue;

                        ArrayList thisLine = ParseCSV(fileContents[i]);

                        if( thisLine.Count < 2 )
                            throw new Exception("The file does not have enough columns at line " + ( i + 1 ) + "!");

                        if( i == 0 )
                        {
                            if( !( (string)thisLine[0] ).Equals("Operator ID",StringComparison.CurrentCultureIgnoreCase) )
                                throw new Exception("The first column of the header must be: Operator ID");
                            
                            else if( !( (string)thisLine[1] ).Equals("Name",StringComparison.CurrentCultureIgnoreCase) )
                                throw new Exception("The first column of the header must be: Name");

                            namesFileHeaders = thisLine;
                        }

                        else
                        {
                            if( ((string)thisLine[0]).Length == 0 || ((string)thisLine[1]).Length == 0 )
                                throw new Exception("The ID or name of an operator is blank at line " + ( i + 1 ) + "!");

                            thisLine[0] = ((string)thisLine[0]).ToUpper();

                            if( operatorsToNamesFileInfo.ContainsKey(thisLine[0]) )
                                throw new Exception(String.Format("The operator ID {0} exists in the file more than once; repeated at line {1}!",(string)thisLine[0],i + 1));

                            operatorsToNamesFileInfo.Add(thisLine[0],thisLine);
                        }
                    }

                    if( operatorsToNamesFileInfo.Count == 0 )
                        throw new Exception("The file does not contain information on any operators!");
                }

                catch( Exception exception )
                {
                    operatorsToNamesFileInfo = null;
                    MessageBox.Show("The names file will not be used because:\n\n" + exception.Message,applicationName);
                }
            }
        }


        string GetCSVCell(string line,ref int startPos)
        {
            if( startPos == line.Length )
                return null;

            string retVal = null;
            int commaPos = line.IndexOf(',',startPos);

            if( commaPos <= 0 ) // no more commas
            {
                retVal = line.Substring(startPos).Trim();
                startPos = line.Length;

                return retVal.Length == 0 ? null : retVal;
            }

            retVal = line.Substring(startPos,commaPos - startPos).Trim();
            startPos = commaPos + 1;

            return retVal;
        }

        ArrayList ParseCSV(string line)
        {
            ArrayList cells = new ArrayList();

            string cell;
            int startPos = 0;

            while( ( cell = GetCSVCell(line,ref startPos) ) != null )
                cells.Add(cell);

            return cells;
        }


        const string OPEN_VIEWING_CONFIG_FILTER = "Viewing Configuration File (*.osvlog)|*.osvlog|All Files (*.*)|*.*";

        public void OpenViewingConfiguration(string filename)
        {
            int expectedCases;
            DateTime endDate;
            string operatorNamesFile;

            try
            {
                ViewingConfigurationFile vcf = ViewingConfigurationFile.Load(filename,out expectedCases,out endDate,out operatorNamesFile);

                if( expectedCases > 0 && endDate != DateTime.MinValue )
                {
                    operationExpectedCases = expectedCases;
                    operationDesiredEndDate = endDate;
                }

                if( operatorNamesFile != null )
                {
                    operatorNamesFilename = operatorNamesFile;
                    ParseOperatorNamesFile();
                }

                List<string> filenames = new List<string>();

                // go through the add actions
                foreach( ViewingConfigurationFile.AddAction aa in vcf.AddActions )
                {
                    if( aa.addType == ViewingConfigurationFile.AddType.Single )
                    {
                        filenames.Add(aa.filename);
                        viewingConfig.AddSingle(aa.filename);
                    }

                    else if( aa.addType == ViewingConfigurationFile.AddType.Multiple )
                    {
                        BulkAddLogsProcessor(aa.filename,false,ref filenames);
                        viewingConfig.AddMultiple(aa.filename,false);
                    }

                    else
                    {
                        BulkAddLogsProcessor(aa.filename,true,ref filenames);
                        viewingConfig.AddMultiple(aa.filename,true);
                    }
                }

                AddLogs(filenames.ToArray());
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message,applicationName);
            }
        }

        // File -> Open Viewing Configuration
        private void openViewingConfigurationToolStripMenuItem_Click(object sender,EventArgs e)
        {
            if( openedFiles.Count > 0 )
            {
                DialogResult dr = MessageBox.Show("Do you want to clear the already open logs?",applicationName,MessageBoxButtons.YesNoCancel);

                if( dr == DialogResult.Cancel )
                    return;

                else if( dr == DialogResult.Yes )
                    closeAllToolStripMenuItem_Click(null,null);
            }

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = OPEN_VIEWING_CONFIG_FILTER;

            if( ofd.ShowDialog() == DialogResult.OK )
                OpenViewingConfiguration(ofd.FileName);
        }

        // File -> Save Viewing Configuration
        private void saveViewingConfigurationToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = OPEN_VIEWING_CONFIG_FILTER;

            if( sfd.ShowDialog() == DialogResult.OK )
            {
                try
                {
                    viewingConfig.Save(sfd.FileName,operationExpectedCases,operationDesiredEndDate,operatorNamesFilename);
                }

                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message,applicationName);
                }
            }

        }

        private void comboBoxInterval_SelectedIndexChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void tabControl_DragDrop(object sender,DragEventArgs e)
        {
            Array filenames = (Array)e.Data.GetData(DataFormats.FileDrop);

            List<string> logFilenames = new List<string>();
            
            foreach( string filename in filenames )
            {
                if( Directory.Exists(filename) ) // in subdirectories we will only process log files, not osvlog files
                {
                    BulkAddLogsProcessor(filename,true,ref logFilenames);
                    viewingConfig.AddMultiple(filename,true);
                }

                else
                {
                    string extension = Path.GetExtension(filename);

                    if( extension.Equals(".osvlog",StringComparison.CurrentCultureIgnoreCase) )
                        OpenViewingConfiguration(filename);

                    else if( extension.Equals(".log",StringComparison.CurrentCultureIgnoreCase) )
                    {
                        logFilenames.Add(filename);
                        viewingConfig.AddSingle(filename);
                    }
                }
            }

            if( logFilenames.Count > 0 )
                AddLogs(logFilenames.ToArray());
        }

        private void tabControl_DragEnter(object sender,DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void ChangeVisualization(Visualization newVis)
        {
            visualization = newVis;

            if( tabControl.SelectedIndex == 7 )
                CreateVisualization();
            
            else
                tabControl.SelectedIndex = 7;
        }

        // Visualize -> Number of Keyers by Time
        private void numberOfKeyersByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.KeyersTime);
        }

        // Visualize -> Keying Time by Time
        private void keyingTimeByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.TimeTime);
        }

        // Visualize -> Keying Rate by Time
        private void keyingRateByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.RateTime);
        }

        // Visualize -> Cases by Time
        private void casesByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.CasesTime);
        }

        // Visualize -> Cumulative Cases by Time
        private void cumulativeCasesByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.CumulativeCasesTime);
        }

        // Visualize -> Cumulative Percent of Cases Verified by Time
        private void cumulativePercentOfCasesVerifiedByDayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.CumulativeVerificationTime);
        }

        // Visualize -> Total Cases by Keyer
        private void cumulativeCasesByKeyerToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.CasesKeyer);
        }

        // Visualize -> Total Time by Keyer
        private void keyingTimeByKeyerToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.TimeKeyer);
        }

        // Visualize -> Total Keying Rate by Keyer
        private void keyingRateByKeyerToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ChangeVisualization(Visualization.RateKeyer);
        }

        private void checkBoxVisualizeSortXAxis_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void checkBoxVisualizeShowLabels_CheckedChanged(object sender,EventArgs e)
        {
            UpdateView();
        }

        private void linkLabelCopyChart_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            using( MemoryStream ms = new MemoryStream() )
            {
                chart.SaveImage(ms,ChartImageFormat.Bmp);
                Bitmap bm = new Bitmap(ms);
                Clipboard.SetImage(bm);
            }
        }

        void CreateVisualization()
        {
            bool byKeyer = visualization == Visualization.CasesKeyer || visualization == Visualization.TimeKeyer || visualization == Visualization.RateKeyer;

            checkBoxVisualizeSortXAxis.Visible = byKeyer;
            labelInterval.Visible = !byKeyer;
            comboBoxInterval.Visible = !byKeyer;

            TabPage tabPage = tabControl.TabPages[tabControl.SelectedIndex];
            tabPage.Controls.Clear();

            chart = new Chart();
            chart.Size = new System.Drawing.Size(tabControl.Size.Width,tabControl.Size.Height);
            chart.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            chart.ChartAreas.Add(new ChartArea());

            string[] titles = {
                                        "Number of Keyers by ",
                                        "Keying Time by ",
                                        "Keying Rate by ",
                                        "Cases Added by ",
                                        "Cumulative Cases Added by ",
                                        "Cumulative Percent of Cases Verified by ",
                                        "Total Cases Added / Verified by Keyer",
                                        "Total Keying Time by Keyer",
                                        "Total Keying Rate by Keyer"
                                    };

            chart.Titles.Add(titles[(int)visualization]);
            chart.Series.Clear();

            var series = new System.Windows.Forms.DataVisualization.Charting.Series
            {
                Name = "Series",
                Color = System.Drawing.Color.SteelBlue,
                IsVisibleInLegend = false,
                IsXValueIndexed = true,
                ChartType = SeriesChartType.Column
            };

            chart.Series.Add(series);


            if( byKeyer )
            {
                SortedDictionary<string,ArrayList> relevantStatsByOper = GetStatisticsByOperator();

                int numOpers = 0;

                foreach( var kvp in relevantStatsByOper )
                {
                    int casesAddedVerified = 0;
                    int keyingTime = 0;
                    int keystrokes = 0;

                    ArrayList stats = kvp.Value;

                    foreach( OperatorStatistics os in stats )
                    {
                        if( os.Mode != EntryMode.Modify )
                            casesAddedVerified += os.Cases;

                        keyingTime += os.KeyingTime - os.PauseTime;
                        keystrokes += os.Keystrokes;
                    }

                    int pos;

                    if( visualization == Visualization.CasesKeyer )
                    {
                        pos = series.Points.AddXY(numOpers++,casesAddedVerified);
                        series.Points[pos].AxisLabel = GetOperatorName(kvp.Key);
                    }

                    else if( visualization == Visualization.TimeKeyer )
                    {
                        pos = series.Points.AddXY(numOpers++,(int)( keyingTime / 3600.0 ));
                        series.Points[pos].AxisLabel = GetOperatorName(kvp.Key);
                    }

                    else
                    {
                        if( keyingTime > 0 )
                        {
                            pos = series.Points.AddXY(numOpers++,(int)( keystrokes / ( keyingTime / 3600.0 ) ));
                            series.Points[pos].AxisLabel = GetOperatorName(kvp.Key);
                        }
                    }
                }

                chart.ChartAreas[0].AxisX.Title = "Keyer";
                chart.ChartAreas[0].AxisX.Interval = 1;
                chart.ChartAreas[0].AxisX.MajorGrid.Enabled = false;

                chart.ChartAreas[0].AxisY.Title = visualization == Visualization.CasesKeyer ? "Cases" : visualization == Visualization.TimeKeyer ? "Hours" : "Keystrokes / Hour";

                if( checkBoxVisualizeSortXAxis.Checked )
                    series.Sort(PointSortOrder.Ascending,"Y");
            }


            else // by time
            {
                int timeInterval = comboBoxInterval.SelectedIndex == 0 ? 4 : comboBoxInterval.SelectedIndex == 1 ? 3 : 2;

                chart.ChartAreas[0].AxisX.Title = timeInterval == 2 ? "Day" : timeInterval == 3 ? "Week" : "Month";
                chart.ChartAreas[0].AxisX.MajorGrid.Enabled = false;

                chart.Titles[0].Text = chart.Titles[0].Text + chart.ChartAreas[0].AxisX.Title;

                string[] yLabels = { "Keyers", "Hours", "Keystrokes / Hour", "Cases", "Cases (Cumulative)", "Percent of Cases Verified (Cumulative)" };
                chart.ChartAreas[0].AxisY.Title = yLabels[(int)visualization];
                

                ArrayList stats = CondenseStatistics(GetAllSortedOperatorStatistics(),timeInterval);

                int totalCases = 0;
                int totalCasesVerified = 0;

                foreach( ReportStatistics rs in stats )
                {
                    totalCases += rs.CasesAdded;
                    totalCasesVerified += rs.CasesVerified;

                    int pos = -1;
                    double xAxisDate = rs.StartTime.ToOADate();

                    switch( visualization )
                    {
                        case Visualization.KeyersTime:

                            if( rs.Operators > 0 )
                                pos = series.Points.AddXY(xAxisDate,rs.Operators);
                        
                            break;

                        case Visualization.TimeTime:

                            if( rs.TotalTime > 0 )
                                pos = series.Points.AddXY(xAxisDate,rs.TotalTime / 3600);
                        
                            break;

                        case Visualization.RateTime:

                            if( rs.TotalTime > 0 )
                                pos = series.Points.AddXY(xAxisDate,(int)(rs.Keystrokes / ( rs.TotalTime / 3600.0 )));
                        
                            break;

                        case Visualization.CasesTime:

                            if( rs.CasesAdded > 0 )
                                pos = series.Points.AddXY(xAxisDate,rs.CasesAdded);
                        
                            break;

                        case Visualization.CumulativeCasesTime:
                            
                            if( rs.CasesAdded > 0 )
                                pos = series.Points.AddXY(xAxisDate,totalCases);
                        
                            break;

                        case Visualization.CumulativeVerificationTime:

                            if( totalCases > 0 && ( rs.CasesAdded > 0 || rs.CasesVerified > 0 ) )
                            {
                                double percent = (int)(1000.0 * totalCasesVerified / totalCases) / 10.0;
                                pos = series.Points.AddXY(xAxisDate,percent);
                            }

                            break;
                    }

                    if( pos >= 0 )
                        series.Points[pos].AxisLabel = DateToDisplayableTime(rs.StartTime,timeInterval == 4);
                }
            }


            if( checkBoxVisualizeShowLabels.Checked )
            {
                series.IsValueShownAsLabel = true;
                series.CustomProperties = "LabelStyle=Bottom";
            }

            chart.Invalidate();

            tabPage.Controls.Add(chart);
        }

    }
}
