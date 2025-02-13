using System.ComponentModel;

namespace DataViewer
{
    class SaveNotesForm : SaveForm
    {
        public SaveNotesForm(CSPro.Data.DataRepository repository, CSPro.Util.ConnectionString output_connection_string)
            :   base("Saving", "Notes")
        {
            _backgroundWorker.DoWork += new DoWorkEventHandler(
                delegate(object o, DoWorkEventArgs args)
                {
                    var notes_extractor = new CSPro.Data.NotesExtractor();
                    args.Result = notes_extractor.Extract(_backgroundWorker, repository, output_connection_string);
                });
        }
    }
}
