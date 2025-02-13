using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization.Json;
using System.Runtime.Serialization;
using System.Net;
using System.Security.Cryptography;
using System.Net.Http;

namespace DropboxAuth
{
    /// <summary>
    /// Authentication with Dropbox REST API using Oauth
    /// </summary>
    /// Uses oauth code flow as described here https://www.dropbox.com/developers/documentation/http/documentation#oauth2-token
    /// Displays Dropbox auth page in default browser.
    /// Once user completes authorization Dropbox redirects to a local uri
    /// for which this program has a listener to retreive the auth code.
    /// The program makes a request to Dropbox to retreive the token
    /// from that code.
    class DropboxAuthorizer
    {

        public async Task<string> DoAuthorizeAsync(string locale)
        {
            // Listener for redirect - can fail if no available ports
            using (HttpListener http_listener = CreateListenerOnFreePort())
            {
                if (http_listener == null)
                    return null;

                var redirect_uri = http_listener.Prefixes.First();

                var auth_code = await ShowAuthInBrowserAsync(locale, http_listener, redirect_uri);
                if (auth_code == null)
                    return null;

                var token_result = await GetTokenAsync(auth_code, redirect_uri);
                if (!string.IsNullOrEmpty(token_result))
                {
                    await RespondToRefreshAsync(http_listener, success_html);
                }
                else
                {
                    await RespondToRefreshAsync(http_listener, error_html);
                }
                return token_result;
            }
        }

        const string error_html = "<html><head><title>CSPro Dropbox</title></head><body><div><h2>Failed to connect to Dropbox.</h2></div></br>Return to CSPro and try again.</body></html>";
        const string success_html = "<html><head><title>CSPro Dropbox</title></head><body><div><h1>You have successfully connected your Dropbox to CSPro!</h1></div><br/><div>Please return to CSPro to continue.</div></body></html>";

        private async Task<string> ShowAuthInBrowserAsync(string locale, HttpListener http_listener, string redirect_uri)
        {
            string state_sent = randomDataBase64url(32);

            const string authorization_endpoint = "https://www.dropbox.com/1/oauth2/authorize";
            string auth_request_uri = $"{authorization_endpoint}?response_type=code&token_access_type=offline&redirect_uri={redirect_uri}&client_id={CSPro.Sync.DropboxKeys.client_id}&state={state_sent}";
            if (locale != null)
                auth_request_uri += $"&locale={locale}";

            // Launch auth request in default browser
            System.Diagnostics.Process.Start(auth_request_uri);

            // If the user completes auth in browser it will redirect to the redirect_uri which
            // will come to our listener.

            // Wait for the redirect to hit the listener
            var context = await http_listener.GetContextAsync();

            var error_code = context.Request.QueryString.Get("error");
            if (error_code != null)
            {
                var error_description = context.Request.QueryString.Get("error_description");
                Console.Error.WriteLine($"Error {error_code}. {error_description}");
                await SendResponseToBrowserAsync(context.Response, error_html);
                return null;
            }

            var code = context.Request.QueryString.Get("code");
            var state_received = context.Request.QueryString.Get("state");

            if (code == null || state_received == null)
            {
                Console.Error.WriteLine("Missing code and/or state in redirect response");
                await SendResponseToBrowserAsync(context.Response, error_html);
                return null;
            }

            // Check that state matches what was sent (prevents XSS)
            if (state_sent != state_received)
            {
                Console.Error.WriteLine("State in redirect does not match");
                await SendResponseToBrowserAsync(context.Response, error_html);
                return null;
            }

            // This will cause a refresh which will result in a second call to listener that we will
            // respond to after getting the token
            await SendResponseToBrowserAsync(context.Response, $"<html><head><meta http-equiv='refresh' content='0;url={redirect_uri}connected'></head><body>Connecting your Dropbox to CSPro...</body></html>");

            return code;
        }

        private async Task SendResponseToBrowserAsync(HttpListenerResponse response, string message)
        {
            string message_html = $"<html><head><title>CSPro Dropbox</title></head><body>{message}</body></html>";
            var buffer = Encoding.UTF8.GetBytes(message_html);
            response.ContentLength64 = buffer.Length;
            using (var output_stream = response.OutputStream) {
                await output_stream.WriteAsync(buffer, 0, buffer.Length);
            }
            response.Close();
        }

        private async Task RespondToRefreshAsync(HttpListener http_listener, string message)
        {
            bool waitingForConnected = true;
            while (waitingForConnected)
            {
                var context = await http_listener.GetContextAsync();
                if (context.Request.Url.LocalPath == "/connected")
                {
                    await SendResponseToBrowserAsync(context.Response, message);
                    waitingForConnected = false;
                } else
                {
                    context.Response.StatusCode = 404;
                }
                context.Response.Close();
            }
        }

        [DataContract]
        private class TokenResponse
        {
            [DataMember]
            public string access_token;
            [DataMember]
            public string refresh_token;
            [DataMember]
            public string token_type;
            [DataMember]
            public string account_id;
            [DataMember]
            public string uid;
        }

        private async Task<string> GetTokenAsync(string auth_code, string redirect_uri)
        {
            using (HttpClient http_client = new HttpClient())
            {
                const string token_endpoint = "https://api.dropboxapi.com/oauth2/token";
                string request_body = $"code={auth_code}&grant_type=authorization_code&client_id={CSPro.Sync.DropboxKeys.client_id}&client_secret={CSPro.Sync.DropboxKeys.client_secret}";
                var post_data_dict = new Dictionary<string, string>()
                {
                    { "code", auth_code },
                    { "grant_type", "authorization_code" },
                    { "redirect_uri", redirect_uri },
                    { "client_id", CSPro.Sync.DropboxKeys.client_id },
                    { "client_secret", CSPro.Sync.DropboxKeys.client_secret }
                };

                try
                {
                    var content = new FormUrlEncodedContent(post_data_dict);
                    var content_str = content.ToString();
                    var response = await http_client.PostAsync(token_endpoint, new FormUrlEncodedContent(post_data_dict));

                    if (response.StatusCode != HttpStatusCode.OK)
                    {
                        Console.Error.WriteLine($"Error retrieving token {await response.Content.ReadAsStringAsync()}");
                        return null;
                    }

                    var responseValue = await response.Content.ReadAsStringAsync();

                    DataContractJsonSerializer serializer =
                        new DataContractJsonSerializer(typeof(TokenResponse));

                    var stream = new System.IO.MemoryStream(Encoding.UTF8.GetBytes(responseValue));
                    var tokenResponse =  (TokenResponse)serializer.ReadObject(stream);

                    if (tokenResponse?.refresh_token != null)
                        return responseValue;

                }
                catch (HttpRequestException e)
                {
                    Console.Error.WriteLine($"Error retrieving token {e.Message}");
                }
            }
            return null;
        }

        private HttpListener CreateListenerOnFreePort()
        {
            // All ports in this range need to be registered in the Dropbox app console
            // Unfortunately Dropbox does not allow the redirect url to have a wildcard port
            const int min_port = 50500; // Arbitrary port in IANA recommended range of 49215 to 65535
            const int max_port = 50530;

            for (int port = min_port; port < max_port; port++)
            {
                HttpListener listener = new HttpListener();
                listener.Prefixes.Add($"http://localhost:{port}/");
                try
                {
                    listener.Start();
                    return listener;
                }
                catch
                {
                }
            }

            Console.Error.WriteLine($"Failed to find an open port in the range {min_port} to {max_port}");
            return null;
        }

        /// <summary>
        /// Returns URI-safe data with a given input length.
        /// </summary>
        /// <param name="length">Input length (nb. output will be longer)</param>
        /// <returns></returns>
        private static string randomDataBase64url(uint length)
        {
            RNGCryptoServiceProvider rng = new RNGCryptoServiceProvider();
            byte[] bytes = new byte[length];
            rng.GetBytes(bytes);
            return base64urlencodeNoPadding(bytes);
        }

        /// <summary>
        /// Base64url no-padding encodes the given input buffer.
        /// </summary>
        /// <param name="buffer"></param>
        /// <returns></returns>
        private static string base64urlencodeNoPadding(byte[] buffer)
        {
            string base64 = Convert.ToBase64String(buffer);

            // Converts base64 to base64url.
            base64 = base64.Replace("+", "-");
            base64 = base64.Replace("/", "_");
            // Strips padding.
            base64 = base64.Replace("=", "");

            return base64;
        }
    }
}
