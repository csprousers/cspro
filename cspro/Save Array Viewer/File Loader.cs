using System;
using System.Collections;
using System.Linq;

namespace SaveArrayViewer
{
    class FileLoader
    {
        public static SaveArrayFile LoadFile(string filename, out bool readExtendedInformation)
        {
            SaveArrayFile saf = new SaveArrayFile();
            saf.Filename = filename;

            readExtendedInformation = false;

            saf.Open();

            // from here on, everything we do is optional (e.g., if the dictionary has errors we still have a successful load of the save array file)
            try
            {
                var save_array_names = saf.SaveArrays.Cast<SaveArray>().Select(x => x.Name).ToArray();

                var worker = new CSPro.Engine.SaveArrayViewerWorker(filename, save_array_names);
                readExtendedInformation = true;

                Hashtable htValuesets = worker.ValueSets;

                foreach( SaveArray sa in saf.SaveArrays )
                {
                    // get the details from logic
                    var dimensions = new ArrayList();
                    var proc_references = new ArrayList();
                    worker.GetLogicDetails(sa.Name, dimensions, proc_references);

                    for( int i = 0; i < dimensions.Count; ++i )
                    {
                        if( i >= sa.Dimension )
                            break;

                        string dimension = (string)dimensions[i];
                        int spillover_pos = dimension.IndexOf("(+)");

                        if( spillover_pos >= 0 )
                        {
                            sa.SetUsingSpillover(i);
                            dimension = dimension.Substring(0, spillover_pos).Trim();
                        }

                        sa.AddDimensionReference(i, dimension.ToUpper());
                    }

                    foreach( string proc_reference in proc_references )
                        sa.AddProcReference(proc_reference);

                    // try to link up the arrays with the appropriate value sets
                    for( int i = 0; i < sa.Dimension; ++i )
                    {
                        int expectedDims = sa.GetDimSize(i);
                        string vsName = sa.GetDimensionReference(i);
                        bool usingSpillover = sa.GetUsingSpillover(i);

                        if( htValuesets.Contains(vsName) )
                        {
                            var vs = (CSPro.Dictionary.ValueSet)htValuesets[vsName];

                            if( expectedDims == ( 1 + vs.Values.Length + ( usingSpillover ? 1 : 0 ) ) ) // only add labels for valid value sets
                            {
                                int j;

                                for( j = 0; j < vs.Values.Length; j++ )
                                    sa.SetDimLabel(i,j + 1,vs.Values[j].Label);

                                if( usingSpillover )
                                    sa.SetDimLabel(i,j + 1,"(+)");
                            }
                        }
                    }
                }
            }

            catch( Exception )
            {
            }

            return saf;
        }
    }
}
