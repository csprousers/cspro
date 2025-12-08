using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace UpdateSEE
{
    partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private void buttonGo_Click(object sender, EventArgs e)
        {
            try
            {
                string exe_directory = Path.GetDirectoryName(Process.GetCurrentProcess().MainModule.FileName);
                string sqlite_dll_directory = new DirectoryInfo(Path.Combine(exe_directory, @"..\..\..\..\..\cspro\external\SQLite")).FullName;

                // sqlite3.h
                var header_lines = new List<string>()
                {
                    "#pragma once",
                    "#include <zSql/zSql.h>",
                    "#define SQLITE_HAS_CODEC"
                };

                header_lines.AddRange(File.ReadAllLines(Path.Combine(textBoxDirectory.Text, "sqlite3.h")));

                File.WriteAllLines(Path.Combine(sqlite_dll_directory, "sqlite3.h"), header_lines.ToArray(), Encoding.UTF8);


                // sqlite3.c
                var source_lines = new List<string>()
                {
                    "#include <zSql/zSql.h>"
                };

                source_lines.AddRange(File.ReadAllLines(Path.Combine(textBoxDirectory.Text, "sqlite3-" + textBoxEncryptionVariant.Text)));

                File.WriteAllLines(Path.Combine(sqlite_dll_directory, "sqlite3.c"), source_lines.ToArray(), Encoding.UTF8);


                MessageBox.Show("Success");
                Close();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }
    }
}
