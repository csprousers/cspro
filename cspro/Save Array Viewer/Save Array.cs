using System;
using System.Collections;

namespace SaveArrayViewer
{
    class SaveArray
    {
        internal CSPro.Engine.SaveArrayValues saveArrayValues;
        string[] rowLabels;
        string[] colLabels;
        string[] layLabels;

        string[] valueSets;
        bool[] spillovers;
        ArrayList procsReferenced;

        public SaveArray(CSPro.Engine.SaveArrayValues save_array_values)
        {
            saveArrayValues = save_array_values;

            valueSets = new string[3];
            spillovers = new bool[3];

            for( int i = 0; i < 3; i++ )
            {
                valueSets[i] = "";
                spillovers[i] = false;
            }

            procsReferenced = new ArrayList();

            rowLabels = new string[Row];
            for( int i = 0; i < Row; i++ )
                rowLabels[i] = String.Format("{0}",i);

            colLabels = new string[Col];
            for( int i = 0; i < Col; i++ )
                colLabels[i] = String.Format("{0}",i);

            layLabels = new string[Lay];
            for( int i = 0; i < Lay; i++ )
                layLabels[i] = String.Format("{0}",i);
        }

        public string Name
        {
            get { return saveArrayValues.Name; }
        }

        public int Dimension
        {
            get { return saveArrayValues.Dimensions.Count; }
        }

        public int Row
        {
            get { return saveArrayValues.Dimensions[0]; }
        }

        public int Col
        {
            get { return ( saveArrayValues.Dimensions.Count > 1 ) ? saveArrayValues.Dimensions[1] : 1; }
        }

        public int Lay
        {
            get { return ( saveArrayValues.Dimensions.Count > 2 ) ? saveArrayValues.Dimensions[2] : 1; }
        }

        public int GetDimSize(int dim)
        {
            return dim == 0 ? Row : dim == 1 ? Col : Lay;
        }

        public string GetRowLabel(int r,bool useVS)
        {
            return useVS ? rowLabels[r] : String.Format("{0}",r);
        }

        public string GetColLabel(int c,bool useVS)
        {
            if( Col > 1 )
                return useVS ? colLabels[c] : String.Format("{0}",c);

            else
                return "";
        }

        public string GetLayLabel(int l,bool useVS)
        {
            return useVS ? layLabels[l] : String.Format("{0}",l);
        }

        public void SetDimLabel(int dim,int idx,string label)
        {
            if( dim == 0 )
                rowLabels[idx] = label;

            else if( dim == 1 )
                colLabels[idx] = label;

            else
                layLabels[idx] = label;
        }

        public bool Numeric
        {
            get { return saveArrayValues.Numeric; }
        }

        public bool Alpha
        {
            get { return !Numeric; }
        }

        int MapCell(int r,int c,int l)
        {
            return l * ( Row * Col ) + c * Row + r;
        }

        public string GetCell(int r,int c,int l)
        {
            return saveArrayValues.Values[MapCell(r,c,l)];
        }

        public void SetCell(string val,int r,int c,int l)
        {
            saveArrayValues.Values[MapCell(r,c,l)] = val;
        }

        public string GetDimensionReference(int dim)
        {
            return valueSets[dim];
        }

        public void AddDimensionReference(int dim,string str)
        {
            valueSets[dim] = str;
        }

        public bool GetUsingSpillover(int dim)
        {
            return spillovers[dim];
        }

        public void SetUsingSpillover(int dim)
        {
            spillovers[dim] = true;
        }

        public void AddProcReference(string proc)
        {
            for( int i = 0; i < procsReferenced.Count; i++ )
            {
                if( String.Equals((string)procsReferenced[i],proc) )
                    return;
            }

            procsReferenced.Add(proc);
        }

        public IEnumerator GetProcReferences()
        {
            return procsReferenced.GetEnumerator();
        }

        public int Runs
        {
            get { return saveArrayValues.Runs; }
            set { saveArrayValues.Runs = value; }
        }

        public int Cases
        {
            get { return saveArrayValues.Cases; }
            set { saveArrayValues.Cases = value; }
        }

        public int GetGetCell(int r,int c,int l)
        {
            return saveArrayValues.Gets[MapCell(r,c,l)];
        }

        public void SetGetCell(int val,int r,int c,int l)
        {
            saveArrayValues.Gets[MapCell(r,c,l)] = val;
        }

        public void SetPutCell(int val,int r,int c,int l)
        {
            saveArrayValues.Puts[MapCell(r,c,l)] = val;
        }

        public int[] GetSortedPuts(bool useZeroIndex)
        {
            return GetSortedValues(saveArrayValues.Puts,useZeroIndex);
        }

        public int[] GetSortedValues(int[] vals,bool useZeroIndex)
        {
            int startingRow = useZeroIndex ? 0 : 1;
            int startingCol = useZeroIndex || Col == 1 ? 0 : 1;
            int startingLay = useZeroIndex || Lay == 1 ? 0 : 1;

            int[] newVals = new int[( Row - startingRow ) * ( Col - startingCol ) * ( Lay - startingLay )];

            int newPos = 0;

            for( int r = startingRow; r < Row; r++ )
            {
                for( int c = startingCol; c < Col; c++ )
                {
                    for( int l = startingLay; l < Lay; l++ )
                        newVals[newPos++] = vals[MapCell(r,c,l)];
                }
            }

            Array.Sort<int>(newVals);

            return newVals;
        }

        public int[] GetSortedGets(bool useZeroIndex)
        {
            return GetSortedValues(saveArrayValues.Gets,useZeroIndex);
        }

        public int GetPutCell(int r,int c,int l)
        {
            return saveArrayValues.Puts[MapCell(r,c,l)];
        }

        public double GetMaxPutGetRatio(bool useZeroIndex)
        {
            double maxRatio = 0;

            int startingRow = useZeroIndex ? 0 : 1;
            int startingCol = useZeroIndex || Col == 1 ? 0 : 1;
            int startingLay = useZeroIndex || Lay == 1 ? 0 : 1;

            for( int r = startingRow; r < Row; r++ )
            {
                for( int c = startingCol; c < Col; c++ )
                {
                    for( int l = startingLay; l < Lay; l++ )
                    {
                        int idx = MapCell(r,c,l);

                        if( saveArrayValues.Gets[idx] > 0 )
                            maxRatio = Math.Max(maxRatio,saveArrayValues.Puts[idx] / (double)saveArrayValues.Gets[idx]);
                    }
                }
            }

            return maxRatio;
        }
    }
}
