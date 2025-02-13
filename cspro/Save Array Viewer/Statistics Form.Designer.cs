namespace SaveArrayViewer
{
    partial class StatisticsForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if( disposing && ( components != null ) )
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StatisticsForm));
            this.statisticsDataGridView = new System.Windows.Forms.DataGridView();
            this.ColumnName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnCells = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnDefaults = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPercentDefault = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnRuns = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnCases = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPuts = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnGets = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPutsGets = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPutsCase = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnGetsCase = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPercentNonZeroPut = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPercentNonZeroGet = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnGetsWithoutPut = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnPutGetAccuracy = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.checkBoxIncludeZeroIndices = new System.Windows.Forms.CheckBox();
            this.linkLabelCopy = new System.Windows.Forms.LinkLabel();
            this.linkLabelCorrespondenceHelp = new System.Windows.Forms.LinkLabel();
            ((System.ComponentModel.ISupportInitialize)(this.statisticsDataGridView)).BeginInit();
            this.SuspendLayout();
            //
            // statisticsDataGridView
            //
            this.statisticsDataGridView.AllowUserToAddRows = false;
            this.statisticsDataGridView.AllowUserToDeleteRows = false;
            this.statisticsDataGridView.AllowUserToOrderColumns = true;
            this.statisticsDataGridView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.statisticsDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleCenter;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.statisticsDataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.statisticsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.statisticsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.ColumnName,
            this.ColumnCells,
            this.ColumnDefaults,
            this.ColumnPercentDefault,
            this.ColumnRuns,
            this.ColumnCases,
            this.ColumnPuts,
            this.ColumnGets,
            this.ColumnPutsGets,
            this.ColumnPutsCase,
            this.ColumnGetsCase,
            this.ColumnPercentNonZeroPut,
            this.ColumnPercentNonZeroGet,
            this.ColumnGetsWithoutPut,
            this.ColumnPutGetAccuracy});
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleCenter;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.statisticsDataGridView.DefaultCellStyle = dataGridViewCellStyle3;
            this.statisticsDataGridView.Location = new System.Drawing.Point(12, 12);
            this.statisticsDataGridView.Name = "statisticsDataGridView";
            this.statisticsDataGridView.ReadOnly = true;
            this.statisticsDataGridView.RowHeadersVisible = false;
            this.statisticsDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.statisticsDataGridView.ShowEditingIcon = false;
            this.statisticsDataGridView.ShowRowErrors = false;
            this.statisticsDataGridView.Size = new System.Drawing.Size(1060, 423);
            this.statisticsDataGridView.TabIndex = 0;
            this.statisticsDataGridView.SelectionChanged += new System.EventHandler(this.statisticsDataGridView_SelectionChanged);
            this.statisticsDataGridView.SortCompare += new System.Windows.Forms.DataGridViewSortCompareEventHandler(this.statisticsDataGridView_SortCompare);
            //
            // ColumnName
            //
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            this.ColumnName.DefaultCellStyle = dataGridViewCellStyle2;
            this.ColumnName.FillWeight = 300F;
            this.ColumnName.HeaderText = "Name";
            this.ColumnName.Name = "ColumnName";
            this.ColumnName.ReadOnly = true;
            //
            // ColumnCells
            //
            this.ColumnCells.HeaderText = "Cells";
            this.ColumnCells.Name = "ColumnCells";
            this.ColumnCells.ReadOnly = true;
            //
            // ColumnDefaults
            //
            this.ColumnDefaults.HeaderText = "Default Cells";
            this.ColumnDefaults.Name = "ColumnDefaults";
            this.ColumnDefaults.ReadOnly = true;
            //
            // ColumnPercentDefault
            //
            this.ColumnPercentDefault.HeaderText = "Default %";
            this.ColumnPercentDefault.Name = "ColumnPercentDefault";
            this.ColumnPercentDefault.ReadOnly = true;
            //
            // ColumnRuns
            //
            this.ColumnRuns.HeaderText = "Program Runs";
            this.ColumnRuns.Name = "ColumnRuns";
            this.ColumnRuns.ReadOnly = true;
            //
            // ColumnCases
            //
            this.ColumnCases.HeaderText = "Cases Processed";
            this.ColumnCases.Name = "ColumnCases";
            this.ColumnCases.ReadOnly = true;
            //
            // ColumnPuts
            //
            this.ColumnPuts.HeaderText = "Puts";
            this.ColumnPuts.Name = "ColumnPuts";
            this.ColumnPuts.ReadOnly = true;
            //
            // ColumnGets
            //
            this.ColumnGets.HeaderText = "Gets";
            this.ColumnGets.Name = "ColumnGets";
            this.ColumnGets.ReadOnly = true;
            //
            // ColumnPutsGets
            //
            this.ColumnPutsGets.HeaderText = "Puts Per Get";
            this.ColumnPutsGets.Name = "ColumnPutsGets";
            this.ColumnPutsGets.ReadOnly = true;
            //
            // ColumnPutsCase
            //
            this.ColumnPutsCase.HeaderText = "Puts Per 100 Cases";
            this.ColumnPutsCase.Name = "ColumnPutsCase";
            this.ColumnPutsCase.ReadOnly = true;
            //
            // ColumnGetsCase
            //
            this.ColumnGetsCase.HeaderText = "Gets Per 100 Cases";
            this.ColumnGetsCase.Name = "ColumnGetsCase";
            this.ColumnGetsCase.ReadOnly = true;
            //
            // ColumnPercentNonZeroPut
            //
            this.ColumnPercentNonZeroPut.HeaderText = "Positive Put Cells %";
            this.ColumnPercentNonZeroPut.Name = "ColumnPercentNonZeroPut";
            this.ColumnPercentNonZeroPut.ReadOnly = true;
            //
            // ColumnPercentNonZeroGet
            //
            this.ColumnPercentNonZeroGet.HeaderText = "Positive Get Cells %";
            this.ColumnPercentNonZeroGet.Name = "ColumnPercentNonZeroGet";
            this.ColumnPercentNonZeroGet.ReadOnly = true;
            //
            // ColumnGetsWithoutPut
            //
            this.ColumnGetsWithoutPut.HeaderText = "Get Without Put Cells";
            this.ColumnGetsWithoutPut.Name = "ColumnGetsWithoutPut";
            this.ColumnGetsWithoutPut.ReadOnly = true;
            //
            // ColumnPutGetAccuracy
            //
            this.ColumnPutGetAccuracy.HeaderText = "Put to Get Match %";
            this.ColumnPutGetAccuracy.Name = "ColumnPutGetAccuracy";
            this.ColumnPutGetAccuracy.ReadOnly = true;
            //
            // checkBoxIncludeZeroIndices
            //
            this.checkBoxIncludeZeroIndices.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.checkBoxIncludeZeroIndices.AutoSize = true;
            this.checkBoxIncludeZeroIndices.Location = new System.Drawing.Point(13, 440);
            this.checkBoxIncludeZeroIndices.Name = "checkBoxIncludeZeroIndices";
            this.checkBoxIncludeZeroIndices.Size = new System.Drawing.Size(123, 17);
            this.checkBoxIncludeZeroIndices.TabIndex = 1;
            this.checkBoxIncludeZeroIndices.Text = "Include Zero Indices";
            this.checkBoxIncludeZeroIndices.UseVisualStyleBackColor = true;
            this.checkBoxIncludeZeroIndices.CheckedChanged += new System.EventHandler(this.checkBoxIncludeZeroIndices_CheckedChanged);
            //
            // linkLabelCopy
            //
            this.linkLabelCopy.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.linkLabelCopy.AutoSize = true;
            this.linkLabelCopy.Location = new System.Drawing.Point(475, 444);
            this.linkLabelCopy.Name = "linkLabelCopy";
            this.linkLabelCopy.Size = new System.Drawing.Size(135, 13);
            this.linkLabelCopy.TabIndex = 2;
            this.linkLabelCopy.TabStop = true;
            this.linkLabelCopy.Text = "Copy Contents to Clipboard";
            this.linkLabelCopy.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelCopy_LinkClicked);
            //
            // linkLabelCorrespondenceHelp
            //
            this.linkLabelCorrespondenceHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelCorrespondenceHelp.AutoSize = true;
            this.linkLabelCorrespondenceHelp.Location = new System.Drawing.Point(882, 444);
            this.linkLabelCorrespondenceHelp.Name = "linkLabelCorrespondenceHelp";
            this.linkLabelCorrespondenceHelp.Size = new System.Drawing.Size(190, 13);
            this.linkLabelCorrespondenceHelp.TabIndex = 3;
            this.linkLabelCorrespondenceHelp.TabStop = true;
            this.linkLabelCorrespondenceHelp.Text = "What does Put to Get Match % Mean?";
            this.linkLabelCorrespondenceHelp.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelCorrespondenceHelp_LinkClicked);
            //
            // StatisticsForm
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1084, 462);
            this.Controls.Add(this.linkLabelCorrespondenceHelp);
            this.Controls.Add(this.linkLabelCopy);
            this.Controls.Add(this.checkBoxIncludeZeroIndices);
            this.Controls.Add(this.statisticsDataGridView);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "StatisticsForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Save Array Statistics";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.StatisticsForm_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.statisticsDataGridView)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView statisticsDataGridView;
        private System.Windows.Forms.CheckBox checkBoxIncludeZeroIndices;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnName;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnCells;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnDefaults;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPercentDefault;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnRuns;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnCases;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPuts;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnGets;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPutsGets;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPutsCase;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnGetsCase;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPercentNonZeroPut;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPercentNonZeroGet;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnGetsWithoutPut;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnPutGetAccuracy;
        private System.Windows.Forms.LinkLabel linkLabelCopy;
        private System.Windows.Forms.LinkLabel linkLabelCorrespondenceHelp;
    }
}