using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DataViewer
{
    class FileFilters
    {
        public const string DataFileFilter = "CSPro Data Files (*.csdb;*.csdbe;*.dat)|*.csdb;*.csdbe;*.dat|All Files (*.*)|*.*";
        public const string CsdbFileFilter = "CSPro DB Files (*.csdb)|*.csdb|Encrypted CSPro DB Files (*.csdbe)|*.csdbe";
        public const string DataAndPffFileFilter = "CSPro Data Files (*.csdb;*.csdbe;*.dat)|*.csdb;*.csdbe;*.dat|PFF Files (*.pff)|*.pff|All Files (*.*)|*.*";
    }
}
