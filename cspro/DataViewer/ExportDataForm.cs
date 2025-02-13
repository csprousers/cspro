using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

namespace DataViewer
{
    partial class ExportDataForm : Form
    {
        private MainForm _mainForm;
        private CSPro.Data.DataViewerSettings _settings;
        private CSPro.Data.Exporter _exporter;

        public ExportDataForm(MainForm main_form, CSPro.Data.DataViewerSettings settings,
            CSPro.Util.ConnectionString connection_string, CSPro.Data.DataRepository repository)
        {
            _mainForm = main_form;
            _settings = settings;
            _exporter = new CSPro.Data.Exporter(repository);

            InitializeComponent();

            textBoxBaseFilename.Text = Path.Combine(Path.GetDirectoryName(connection_string.Filename),
                Path.GetFileNameWithoutExtension(connection_string.Filename));

            // reselect the settings from the last export
            foreach( CheckBox checkBox in groupBoxExportFormats.Controls )
            {
                if( _settings.ExportFormatSelections.Contains(checkBox.Name) )
                    checkBox.Checked = true;
            }

            checkBoxOneFilePerRecord.Checked = _settings.ExportOneFilePerRecord;

            UpdateUI();
        }

        private void linkLabelExportDataTool_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            _mainForm.OpenDataInCSExport();
        }

        private void UpdateUI(object sender = null, EventArgs e = null)
        {
            var exporter_export_types = new HashSet<CSPro.Data.ExporterExportType>();

            foreach( CheckBox checkBox in groupBoxExportFormats.Controls )
            {
                if( checkBox.Checked )
                    exporter_export_types.Add((CSPro.Data.ExporterExportType)checkBox.Tag);
            }

            _exporter.Refresh(textBoxBaseFilename.Text, exporter_export_types, checkBoxOneFilePerRecord.Checked);

            textBoxOutputFilenames.Text = _exporter.GetOutputFilenames();

            buttonExportData.Enabled = ( exporter_export_types.Count > 0 );
        }

        private void buttonExportData_Click(object sender, EventArgs e)
        {
            // save the settings so that they are restored on the next export
            _settings.ExportFormatSelections.Clear();

            foreach( CheckBox checkBox in groupBoxExportFormats.Controls )
            {
                if( checkBox.Checked )
                    _settings.ExportFormatSelections.Add(checkBox.Name);
            }

            _settings.ExportOneFilePerRecord = checkBoxOneFilePerRecord.Checked;

            // run the export
            var export_data_worker_form = new ExportDataWorkerForm(_exporter);
            export_data_worker_form.ShowDialog();

            Close();
        }
    }
}
