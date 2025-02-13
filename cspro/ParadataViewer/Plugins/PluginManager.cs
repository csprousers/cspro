using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reflection;

namespace ParadataViewer
{
    [Serializable]
    class PluginMetadata
    {
        public string DisplayName { get; set; }
        public string ClassName { get; set; }
        public List<string> Filenames { get; set; }
        public string DllFilename { get { return Filenames.FirstOrDefault(); } }
        public bool Enabled { get; set; }
    }

    [Serializable]
    class OnlinePluginMetadataLink : PluginMetadata
    {
        public string Url { get; set; }

        public OnlinePluginMetadataLink(string displayName,string url)
        {
            DisplayName = displayName;
            Url = url;
            Enabled = true;
        }
    }

    class PluginManager
    {
        public static List<PluginMetadata> LoadPluginsFromFile(string filename)
        {
            var plugins = new List<PluginMetadata>();

            var assembly = Assembly.LoadFrom(filename);

            foreach( Type type in assembly.GetExportedTypes() )
            {
                if( !type.IsAbstract && typeof(IPluggableQueryControl).IsAssignableFrom(type) )
                {
                    try
                    {
                        var plugin = (IPluggableQueryControl)Activator.CreateInstance(type);

                        if( plugin.MinimumCSProVersionRequired <= CSPro.Util.Versioning.Number )
                        {
                            var pluginMetadata = new PluginMetadata()
                            {
                                DisplayName = plugin.WindowTitle,
                                ClassName = type.FullName,
                                Filenames = new List<string>(),
                                Enabled = true
                            };

                            pluginMetadata.Filenames.Add(filename);

                            plugins.Add(pluginMetadata);
                        }

                    }
                    catch { }
                }
            }

            return plugins;
        }

        public static IPluggableQueryControl GetQueryControl(PluginMetadata plugin)
        {
            var assembly = Assembly.LoadFrom(plugin.DllFilename);
            Type type = assembly.GetType(plugin.ClassName);
            return (IPluggableQueryControl)Activator.CreateInstance(type);
        }
    }
}
