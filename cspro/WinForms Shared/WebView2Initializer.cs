using System;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace WinFormsShared
{
    class WebView2Initializer
    {
        public static async Task Initialize(WebView2 web_view)
        {
            string user_data_directory = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "CSPro", "webview", "WinForms");

            var environment = await CoreWebView2Environment.CreateAsync(null, user_data_directory, null);

            await web_view.EnsureCoreWebView2Async(environment);
        }

        public static void ApplyDefaultSettings(WebView2 web_view, bool allow_scripts_and_web_messages)
        {
            web_view.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
            web_view.CoreWebView2.Settings.AreDefaultScriptDialogsEnabled = false;
            web_view.CoreWebView2.Settings.AreHostObjectsAllowed = false;
            web_view.CoreWebView2.Settings.IsScriptEnabled = allow_scripts_and_web_messages;
            web_view.CoreWebView2.Settings.IsStatusBarEnabled = false;
            web_view.CoreWebView2.Settings.IsWebMessageEnabled = allow_scripts_and_web_messages;
        }
    }
}
