using System;
using System.IO;
using System.Net;
using System.Web;

namespace WinFormsShared
{
    class LocalFileServer
    {
        private string _rootDirectory;
        private string _baseUrl;

        private LocalFileServer(string root_directory)
        {
            _rootDirectory = root_directory;

            // find an available port (code based on DropboxAuthorizer.cs)
            const int MinPort = 50500;
            const int MaxPort = 50530;

            for( int port = MinPort; port <= MaxPort; ++port )
            {
                try
                {
                    var http_listener = new HttpListener();

                    _baseUrl = $"http://localhost:{port}/";
                    http_listener.Prefixes.Add(_baseUrl);

                    // ignore errors when the server is closed before a stream is fully written
                    http_listener.IgnoreWriteExceptions = true;

                    http_listener.Start();

                    // start listening for requests and return
                    http_listener.BeginGetContext(new AsyncCallback(HttpListenerCallback), http_listener);

                    return;
                }
                catch { }
            }

            throw new Exception("Could not initialize the Local File Server.");
        }

        public static LocalFileServer CreateFromDirectory(string root_directory)
        {
            return new LocalFileServer(root_directory);
        }

        public static LocalFileServer CreateFromApplicationDirectory(string relative_path = "")
        {
            return new LocalFileServer(Path.Combine(Path.GetDirectoryName(
                System.Reflection.Assembly.GetExecutingAssembly().Location), relative_path));
        }

        public Uri CreateUri(string relative_path)
        {
            return new Uri(_baseUrl + relative_path);
        }

        private void HttpListenerCallback(IAsyncResult result)
        {
            var http_listener = (HttpListener)result.AsyncState;

            HttpListenerContext context = http_listener.EndGetContext(result);

            // listen for the next request
            http_listener.BeginGetContext(new AsyncCallback(HttpListenerCallback), http_listener);

            // process the current request
            HttpListenerRequest request = context.Request;
            HttpListenerResponse response = context.Response;

            string filename = _rootDirectory;

            foreach( string segment in request.RawUrl.Split(new char[] { '/' }, StringSplitOptions.RemoveEmptyEntries) )
                filename = Path.Combine(filename, segment);

            try
            {
                var fi = new FileInfo(filename);

                if( fi.Exists )
                {
                    response.ContentLength64 = fi.Length;
                    response.ContentType = MimeMapping.GetMimeMapping(filename);
                    response.StatusCode = (int)HttpStatusCode.OK;

                    using( var file = File.OpenRead(filename) )
                        file.CopyTo(response.OutputStream);

                    response.OutputStream.Close();

                    return;
                }
            }
            catch { }

            // if here, there was an error, so return no content
            response.ContentLength64 = 0;
            response.StatusCode = (int)HttpStatusCode.NotFound;
            response.OutputStream.Close();
        }
    }
}
