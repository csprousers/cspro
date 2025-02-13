using System.Windows.Forms;

namespace ParadataViewer
{
    partial class Controller
    {
        internal void SaveSplitterState(Form form,Panel panel,Splitter splitter)
        {
            string key = $"{form.GetType().Name}-{panel.Name}";
            bool adjustingWidth = ( splitter.Dock == DockStyle.Left );

            Settings.SplitterStates[key] = adjustingWidth ? panel.Width : panel.Height;
        }

        internal void RestoreSplitterState(Form form,Panel panel,Splitter splitter)
        {
            string key = $"{form.GetType().Name}-{panel.Name}";
            bool adjustingWidth = ( splitter.Dock == DockStyle.Left );
            int size;

            if( Settings.SplitterStates.TryGetValue(key,out size) )
            {
                if( adjustingWidth )
                {
                    if( size < panel.Parent.Width )
                        panel.Width = size;
                }

                else
                {
                    if( size < panel.Parent.Height )
                        panel.Height = size;
                }
            }
        }


        internal void SaveCheckBoxState(CheckBox checkBox)
        {
            string key = $"{checkBox.Parent.Name}-{checkBox.Name}";
            Settings.CheckBoxStates[key] = checkBox.Checked;
        }

        internal void RestoreCheckBoxState(CheckBox checkBox)
        {
            string key = $"{checkBox.Parent.Name}-{checkBox.Name}";
            bool isChecked;

            if( Settings.CheckBoxStates.TryGetValue(key,out isChecked) )
                checkBox.Checked = isChecked;
        }
    }
}
