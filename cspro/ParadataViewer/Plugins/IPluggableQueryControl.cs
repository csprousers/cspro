using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ParadataViewer
{
    public interface IPluggableQueryControl
    {
        /// <summary>
        /// Returns the minimum version number of CSPro required to run the plugin.
        /// </summary>
        double MinimumCSProVersionRequired { get; }

        /// <summary>
        /// The title of the query control.
        /// </summary>
        string WindowTitle { get; }

        /// <summary>
        /// Returns the control to be hosted in the Paradata Viewer.
        /// </summary>
        Control CreatePluginControl(IPluggableOwner pluggableOwner);

        /// <summary>
        /// The link text to save the contents of the control. Return null if unused.
        /// </summary>
        string SaveContentsText { get; }

        /// <summary>
        /// A handler to save the contents of the control.
        /// </summary>
        Task SaveContentsAsync();

        /// <summary>
        /// Returns whether or not the SQLite query is based on the event table, in which case
        /// filters will apply to it.
        /// </summary>
        bool IsSqlQueryEventBased { get; }

        /// <summary>
        /// Returns the SQLite query to execute on the paradata log.
        /// </summary>
        string GetSqlQuery();

        /// <summary>
        /// Processes the results of the query.
        /// </summary>
        void ProcessResults(List<object[]> rows,int startingRow);
    }
}
