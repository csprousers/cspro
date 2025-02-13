using System;
using System.Collections;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace SaveArrayViewer
{
    class SaveArrayFile
    {
        ArrayList saveArrays;
        Hashtable htArrays;
        string filename;

        public SaveArrayFile()
        {
            saveArrays = new ArrayList();
            htArrays = new Hashtable();
        }

        public string Filename
        {
            get { return filename; }
            set { filename = value; }
        }

        public string DisplayName
        {
            get { return Path.GetFileName(filename); }
        }

        public ArrayList SaveArrays
        {
            get { return saveArrays; }
        }


        public void Open()
        {
            var save_array_file = new CSPro.Engine.SaveArrayFile();

            try
            {
                save_array_file.Load(filename);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            foreach( var save_array_values in save_array_file.GetSaveArrayValues() )
                saveArrays.Add(new SaveArray(save_array_values));

            CreateHashTable();
        }

        public void Save()
        {
            try
            {
                var save_array_file = new CSPro.Engine.SaveArrayFile();

                var save_array_values_query =
                    from SaveArray save_array in saveArrays
                    select save_array.saveArrayValues;

                save_array_file.SetSaveArrayValues(save_array_values_query.ToList());
                save_array_file.Save(filename);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }


        void CreateHashTable()
        {
            IEnumerator itr = saveArrays.GetEnumerator();

            while( itr.MoveNext() )
            {
                SaveArray sa = (SaveArray)itr.Current;
                htArrays.Add(sa.Name,sa);
            }
        }

        public bool SaveArrayExists(string name)
        {
            return htArrays.ContainsKey(name);
        }

        public SaveArray GetSaveArrayByName(string name)
        {
            return (SaveArray) htArrays[name];
        }

        public void RemoveSaveArray(SaveArray sa)
        {
            saveArrays.Remove(sa);
        }
    }
}
