using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace DataViewer
{
    class InterProcessCommunication
    {
        // useful information here: https://www.codeproject.com/Articles/19570/Inter-Process-Communication-with-C

        public const int WM_COPYDATA = 0x004A;

#pragma warning disable 0649
        struct CopyDataStruct
        {
            public IntPtr dwData;
            public int cbData;
            public IntPtr lpData;
        }
#pragma warning restore 0649

        public static string[] GetCopyDataStrings(ref Message message)
        {
            const int NumberStrings = 3;
            var strings = new string[NumberStrings];

            var copy_data_struct = (CopyDataStruct)Marshal.PtrToStructure(message.LParam, typeof(CopyDataStruct));
            IntPtr data_ptr = copy_data_struct.lpData;

            for( int i = 0; i < NumberStrings; ++i )
            {
                int string_length = Marshal.ReadInt32(data_ptr);
                data_ptr += sizeof(int);

                strings[i] = Marshal.PtrToStringUni(data_ptr, string_length);
                data_ptr += string_length * sizeof(char);
            }
 
            return strings;
        }
    }
}
