using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.WinForms;

namespace DataViewer
{
    class WebView2Helper
    {
        public static async Task Initialize(WebView2 web_view)
        {
            await WinFormsShared.WebView2Initializer.Initialize(web_view);

            // set some additional settings
            WinFormsShared.WebView2Initializer.ApplyDefaultSettings(web_view, false);

            // for passing accelerator keys to the main form for processing
            web_view.KeyDown += WebView_KeyDown;
        }

        private static void WebView_KeyDown(object sender, KeyEventArgs e)
        {
            if( Control.ModifierKeys != Keys.None )
            {
                if( MainForm.HandleAcceleratorKey(e.KeyCode | Control.ModifierKeys) )
                    e.Handled = true;
            }
        }
    }
}
