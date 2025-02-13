using System.ComponentModel;

namespace DataViewer
{
    class ExportDataWorkerForm : SaveForm
    {
        public ExportDataWorkerForm(CSPro.Data.Exporter exporter)
            :   base("Exporting", "Data")
        {
            _backgroundWorker.DoWork += new DoWorkEventHandler(
                delegate(object o, DoWorkEventArgs args)
                {
                    args.Result = exporter.Export(_backgroundWorker);
                });
        }
    }
}
