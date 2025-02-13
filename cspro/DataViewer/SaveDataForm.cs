using System.ComponentModel;

namespace DataViewer
{
    class SaveDataForm : SaveForm
    {
        public SaveDataForm(CSPro.Data.DataRepository repository, CSPro.Util.ConnectionString output_connection_string)
            :   base("Saving", "Data")
        {
            _backgroundWorker.DoWork += new DoWorkEventHandler(
                delegate(object o, DoWorkEventArgs args)
                {
                    var repository_converter = new CSPro.Data.RepositoryConverter();
                    args.Result = repository_converter.Convert(_backgroundWorker, repository, output_connection_string);
                });
        }
    }
}
