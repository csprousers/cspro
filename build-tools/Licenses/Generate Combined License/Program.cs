using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Generate_Combined_License
{
    class Program
    {
        private class License
        {
            public string Name;
            public string ProjectUrl;
            public string LicenseFilename;
            public string LicenseUrl;
            public string LicenseText;
        }

        private static string GetStringFromLine(string text)
        {
            const string NullMarker = "{}";
            return text.Equals(NullMarker) ? null : text.Trim();
        }

        static void Main(string[] args)
        {
            try
            {
                string executable_directory = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
                string base_directory = Path.GetFullPath(Path.Combine(executable_directory, "../../../"));

                // load the licenses
                var lines = File.ReadAllLines(Path.Combine(base_directory, "_Details.txt"));
                var licenses = new List<License>();

                for( int i = 0; i < lines.Length; ++i )
                {
                    if( !string.IsNullOrWhiteSpace(lines[i]) )
                    {
                        var license = new License();

                        license.Name = GetStringFromLine(lines[i++]);
                        license.ProjectUrl = GetStringFromLine(lines[i++]);
                        license.LicenseFilename = GetStringFromLine(lines[i++]);
                        license.LicenseUrl = GetStringFromLine(lines[i]);

                        license.LicenseText = File.ReadAllText(Path.Combine(base_directory, "Licenses", license.Name + ".txt")).TrimEnd();

                        licenses.Add(license);
                    }
                }

                licenses = licenses.OrderBy(x => x.Name).ToList();


                // add each license name
                var license_list_html = new StringBuilder();

                license_list_html.Append("<ul>");

                foreach( var license in licenses )
                    license_list_html.Append($"<li><a href=\"#{GetAnchorName(license.Name)}\">{license.Name}</a></li>");

                license_list_html.Append("</ul>");


                // add each license
                var license_html = new StringBuilder();

                foreach( var license in licenses )
                {
                    license_html.Append($"<h2 id=\"{GetAnchorName(license.Name)}\">{license.Name}</h2>");

                    license_html.Append("\n<p>");

                    if( license.ProjectUrl != null )
                        license_html.Append($"Project website: <a href=\"{license.ProjectUrl}\">{license.ProjectUrl}</a><br />");

                    if( license.LicenseFilename != null || license.LicenseUrl != null )
                    {
                        license_html.Append("License ");

                        if( license.LicenseFilename != null )
                        {
                            license_html.Append($"found in the file {license.LicenseFilename}");

                            if( license.LicenseUrl != null )
                                license_html.Append(" and ");
                        }

                        if( license.LicenseUrl != null )
                            license_html.Append($"available at <a href=\"{license.LicenseUrl}\">{license.LicenseUrl}</a>");
                    }

                    license_html.Append("</p>\n");

                    license_html.Append($"<p class=\"license\">{GetHtmlFromText(license.LicenseText)}</p>\n\n");
                }


                // read the licenses template and the CSPro license
                string license_template_html = File.ReadAllText(Path.Combine(base_directory, "_LicensesTemplate.html"));
                string cspro_license = File.ReadAllText(Path.Combine(base_directory, "Licenses", "CSPro.txt"));

                string final_html = license_template_html
                    .Replace("{{ list-list }}", license_list_html.ToString())
                    .Replace("{{ cspro-license }}", $"<p class=\"license\">{GetHtmlFromText(cspro_license)}</p>\n\n")
                    .Replace("{{ licenses }}", license_html.ToString());

                File.WriteAllText(Path.Combine(base_directory, "Licenses.html"), final_html, Encoding.UTF8);

                Console.WriteLine("Success!");
            }

            catch( Exception exception )
            {
                Console.WriteLine(exception.Message);
            }
        }

        private static string GetAnchorName(string name)
        {
            // only use lower-case letters
            return new string(name.ToLower().ToCharArray().Where(x => Char.IsLetter(x)).ToArray());
        }

        private static string GetHtmlFromText(string text)
        {
            return text
                .Replace("&", "&amp;")
                .Replace("<", "&lt;")
                .Replace(">", "&gt;")
                .Replace("\r\n", "\n")
                .Replace("\n", "<br />")
                .Replace("\t", "    ")
                .Replace("< ", "<&nbsp;")
                .Replace("> ", ">&nbsp;")
                .Replace("  ", " &nbsp;")
                .Replace("<br />", "<br />\n");
        }
    }
}
