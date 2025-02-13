using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization;

namespace CSDeploy
{
    [DataContract]
    public class FileSpec
    {
        [DataMember(Name = "path", IsRequired = false)]
        public string Path { get; set; }

        [DataMember(Name = "signature", IsRequired = false, EmitDefaultValue = false)]
        public string Signature { get; set; }

        [DataMember(Name = "onlyOnFirstInstall", IsRequired = false, EmitDefaultValue = false)]
        public bool OnlyOnFirstInstall { get; set; }

    }
}
