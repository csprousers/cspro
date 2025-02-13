using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace ParadataViewer
{
    [Serializable]
    class Settings
    {
        // plugins
        internal List<PluginMetadata> InstalledPlugins { get; private set; }

        // query settings
        internal QueryOptions QueryOptions { get; private set; }

        internal int FilterQueryNumberRows { get; set; }
        internal int GeneralQueryInitialNumberRows { get; set; }
        internal int GeneralQueryMaximumNumberRows { get; set; }
        internal int TabularQueryInitialNumberRows { get; set; }
        internal int TabularQueryMaximumNumberRows { get; set; }

        // display settings
        internal bool ShowSqlStatementsPane { get; set; }
        internal Dictionary<string,int> SplitterStates { get; private set; }
        internal Dictionary<string,bool> CheckBoxStates { get; private set; }
        internal string TimestampFormatter { get; set; }
        internal string TimestampFormatterForFilters { get; set; }


        internal Settings()
        {
            InstalledPlugins = new List<PluginMetadata>();

            QueryOptions = new QueryOptions();

            FilterQueryNumberRows = 250;
            GeneralQueryInitialNumberRows = 100000;
            GeneralQueryMaximumNumberRows = 1000000;
            TabularQueryInitialNumberRows = 1000;
            TabularQueryMaximumNumberRows = 10000;

            ShowSqlStatementsPane = false;
            SplitterStates = new Dictionary<string,int>();
            CheckBoxStates = new Dictionary<string,bool>();
            TimestampFormatter = "%c";
            TimestampFormatterForFilters = "%F";
        }

        [OnDeserialized]
        internal void OnDeserializedMethod(StreamingContext context)
        {
            // some new additions to Settings have to be handled here (as they weren't handled by the
            // constructor since they didn't exist in a previous version of the Paradata Viewer)
            if( SplitterStates == null )
                SplitterStates = new Dictionary<string,int>();

            if( CheckBoxStates == null )
                CheckBoxStates = new Dictionary<string,bool>();
        }

        internal static string SettingsDirectory
        {
            get
            {
                string directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                    "CSPro",Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location));
                Directory.CreateDirectory(directory);
                return directory;
            }
        }

        internal static string PluginsDirectory
        {
            get
            {
                string directory = Path.Combine(SettingsDirectory,"Plugins");
                Directory.CreateDirectory(directory);
                return directory;
            }
        }

        private static string SettingsFilename
        {
            get
            {
                return Path.Combine(SettingsDirectory,"Settings.dat");
            }
        }

        static internal Settings Load()
        {
            try
            {
                using( var fs = new FileStream(SettingsFilename,FileMode.Open,FileAccess.Read) )
                    return (Settings)new BinaryFormatter().Deserialize(fs);
            }

            catch
            {
                return new Settings();
            }
        }

        internal void Save()
        {
            try
            {
                using( var fs = new FileStream(SettingsFilename,FileMode.Create,FileAccess.Write) )
                    new BinaryFormatter().Serialize(fs,this);
            }
            catch { }
        }
    }

    [Serializable]
    public class QueryOptions
    {
        internal bool ViewValueLabels { get; set; }
        internal bool ConvertTimestamps { get; set; }
        internal bool ShowFullyLinkedTables { get; set; }
        internal bool ShowLinkingValues { get; set; }

        internal QueryOptions()
        {
            ViewValueLabels = true;
            ConvertTimestamps = true;
            ShowFullyLinkedTables = true;
            ShowLinkingValues = false;
        }

        internal QueryOptions(QueryOptions rhs)
        {
            ViewValueLabels = rhs.ViewValueLabels;
            ConvertTimestamps = rhs.ConvertTimestamps;
            ShowFullyLinkedTables = rhs.ShowFullyLinkedTables;
            ShowLinkingValues = rhs.ShowLinkingValues;
        }

        internal bool Equals(QueryOptions rhs)
        {
            return ( this == rhs ) || ( ( ViewValueLabels == rhs.ViewValueLabels ) && ( ConvertTimestamps == rhs.ConvertTimestamps ) &&
                ( ShowFullyLinkedTables == rhs.ShowFullyLinkedTables ) && ( ShowLinkingValues == rhs.ShowLinkingValues ) );
        }
    }
}
