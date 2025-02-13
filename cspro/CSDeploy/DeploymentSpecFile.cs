using System;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using CSPro.Util;

namespace CSDeploy
{
    [DataContract]
    class DeploymentSpecFile
    {
        [DataMember(Name = "software", Order = 1, IsRequired = false)]
        private string Software { get; set; }

        [DataMember(Name="version", Order = 2, IsRequired = false)]
        private double Version { get; set; }

        [DataMember(Name = "fileType", Order = 3, IsRequired = false)]
        private string FileType { get; set; }

        [DataMember(Name="name", Order = 4, EmitDefaultValue = false)]
        public string Name { get; set; }

        [DataMember(Name="description", Order = 5, EmitDefaultValue = false)]
        public string Description { get; set; }

        [DataMember(Name="buildTime", Order = 6, IsRequired = false, EmitDefaultValue = false)]
        public DateTime? BuildTime { get; set; }

        [DataMember(Name="files", Order = 7, EmitDefaultValue = false)]
        public FileSpec[] Files { get; set; }

        [DataContract]
        public class DictionarySync
        {
            [DataMember(Name = "path", Order = 1, IsRequired = true)]
            public string Path { get; set; }

            [DataMember(Name = "uploadForSync", Order = 2, EmitDefaultValue = false)]
            public bool UploadForSync { get; set; }

        }

        [DataMember(Name = "dictionaries", Order = 8, EmitDefaultValue = false)]
        public DictionarySync[] Dictionaries { get; set; }

        [DataContract]
        public class DeploymentSpec
        {
            [DataMember(Name="type", Order = 1, IsRequired = false)]
            public string Type { get; set; }

            [DataMember(Name = "cswebUrl", Order = 2, EmitDefaultValue = false)]
            public string CSWebUrl { get; set; }

            [DataMember(Name = "ftpUrl", Order = 3, EmitDefaultValue = false)]
            public string FtpUrl { get; set; }

            [DataMember(Name = "localFilePath", Order = 4, EmitDefaultValue = false)]
            public string LocalFilePath { get; set; }

            [DataMember(Name = "localFolderPath", Order = 5, EmitDefaultValue = false)]
            public string LocalFolderPath { get; set; }

        }

        [DataMember(Name="deployment",Order = 9, EmitDefaultValue = false)]
        public DeploymentSpec Deployment { get; set; }

        public DeploymentSpecFile()
        {
            Software = "CSPro";
            Version = Versioning.Number;
            FileType = "deployment";
            Deployment = new DeploymentSpec();
        }

        public void Save(string filename)
        {
            string packageSpecJson = SaveToString();

            // to ensure that the spec file looks similar to the other ones generated in CSPro,
            // use the JsonFileWriter to write out the JSON; give how rare saving this file is,
            // the overhead of this operation doesn't seem overly burdensome/ridiculous
            JsonSaver.SaveInSpecFileFormat(filename, packageSpecJson);
        }

        public string SaveToString()
        {
            var ms = new MemoryStream();
            Save(ms);
            return Encoding.Default.GetString(ms.GetBuffer());
        }

        public void Save(Stream stream)
        {
            using (var writer = JsonReaderWriterFactory.CreateJsonWriter(stream, Encoding.UTF8, false))
            {
                DataContractJsonSerializer serializer =
                    new DataContractJsonSerializer(typeof(DeploymentSpecFile),
                                                   new DataContractJsonSerializerSettings
                                                    {
                                                        DateTimeFormat = new DateTimeFormat("yyyy-MM-dd'T'HH:mm:ssZ")
                                                    });
                serializer.WriteObject(writer, this);
            }
        }

        public DeploymentSpecFile Clone()
        {
            using (var stream = new MemoryStream())
            {
                Save(stream);
                stream.Seek(0, SeekOrigin.Begin);
                return Load(stream);
            }
        }

        public static DeploymentSpecFile Load(string filename)
        {
            using (var stream = new MemoryStream(Encoding.UTF8.GetBytes(File.ReadAllText(filename))))
            {
                return Load(stream);
            }
        }

        public static DeploymentSpecFile Load(Stream stream)
        {
            DataContractJsonSerializer serializer =
                new DataContractJsonSerializer(typeof(DeploymentSpecFile),
                                                new DataContractJsonSerializerSettings
                                                {
                                                    DateTimeFormat = new DateTimeFormat("yyyy-MM-dd'T'HH:mm:ssZ")
                                                });
            DeploymentSpecFile loadedSpec = (DeploymentSpecFile)serializer.ReadObject(stream);
            if( loadedSpec.FileType != "deployment" && loadedSpec.FileType != "Application Deployment Specification" )
                throw new Exception("Not a valid a deployment specification");

            return loadedSpec;
        }
    }
}
