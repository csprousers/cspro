using CSPro.Util;

namespace DataViewer
{
    public class SyncServerParameters
    {
        public SyncServerType Type { get; private set; }
        public string Url { get; set; }

        public SyncServerParameters(SyncServerType type, string url)
        {
            Type = type;
            Url = url;
        }

        public SyncServerParameters(string serverUrl)
        {
            if (serverUrl == "Dropbox")
            {
                Type = SyncServerType.Dropbox;
                Url = null;
            }
            else
            {
                var scheme = GetUrlScheme(serverUrl);
                if (IsValidFtpScheme(scheme))
                {
                    Type = SyncServerType.FTP;
                    Url = serverUrl;
                }
                else if (IsValidHttpScheme(scheme))
                {
                    Type = SyncServerType.CSWeb;
                    Url = serverUrl;
                }
                else if (IsValidFileScheme(scheme))
                {
                    Type = SyncServerType.LocalFiles;
                    Url = RemoveUrlScheme(serverUrl); // just path, no file:///
                }
                else
                {
                    Type = SyncServerType.CSWeb;
                    Url = null;
                }
            }
        }

        public void ChangeType(SyncServerType new_type)
        {
            Type = new_type;

            if (Type == SyncServerType.LocalFiles)
            {
                // If switching from url to local path, clear the url
                if (GetUrlScheme(Url) != "")
                    Url = "";
            }
            else if (Type == SyncServerType.CSWeb)
            {
                if (GetUrlScheme(Url) == "")
                {
                    // If switching from local path to url, clear the url
                    Url = "http://";
                }
                else if (!IsValidHttpScheme(GetUrlScheme(Url)))
                {
                    // Switch from ftp to http
                    Url = ReplaceUrlScheme(Url, "http");
                }
            }
            else if (Type == SyncServerType.FTP)
            {
                if (GetUrlScheme(Url) == "")
                {
                    // If switching from local path to url, clear the url
                    Url = "ftp://";
                }
                else if (!IsValidFtpScheme(GetUrlScheme(Url)))
                {
                    // Switch from http to ftp
                    Url = ReplaceUrlScheme(Url, "ftp");
                }
            }
        }

        private string GetUrlScheme(string url)
        {
            if (url == null)
                return "";

            int sepPos = url.IndexOf("://");
            if (sepPos < 0)
            {
                return "";
            }
            else
            {
                if( url.IndexOf("file:///") == 0 )
                    ++sepPos;

                return url.Substring(0, sepPos);
            }
        }
        private string ReplaceUrlScheme(string url, string newScheme)
        {
            return newScheme + "://" + ( newScheme == "file" ? "/" : "" ) + RemoveUrlScheme(url);
        }

        private string RemoveUrlScheme(string url)
        {
            if (url == null)
                return null;
            int sepPos = url.IndexOf("://");
            if (sepPos < 0)
            {
                return url;
            }
            else
            {
                return url.Substring(sepPos + 3);
            }
        }

        private bool IsValidHttpScheme(string scheme)
        {
            switch (scheme)
            {
                case "http":
                case "https":
                    return true;
                default:
                    return false;
            }
        }
        private bool IsValidFtpScheme(string scheme)
        {
            switch (scheme)
            {
                case "ftp":
                case "ftps":
                case "ftpes":
                    return true;
                default:
                    return false;
            }
        }

        public bool TypeRequiresUrl
        {
            get { return Type != SyncServerType.Dropbox && Type != SyncServerType.LocalDropbox; }
        }

        public bool IsValidServerUrl()
        {
            if (!TypeRequiresUrl)
                return true;

            if (Url == null)
                return false;

            if (Type == SyncServerType.LocalFiles)
            {
                return Url.Length > 0;
            }
            else
            {
                var scheme = GetUrlScheme(Url);
                if (scheme.Length == 0)
                    return false;

                // Check for a path after the scheme (and the "://")
                if (Url.Length <= scheme.Length + 3)
                    return false;

                return true;
            }
        }

        private bool IsValidFileScheme(string scheme)
        {
            return scheme == "file";
        }
    }
}
