using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace ParadataViewer
{
    partial class FilterPanelControl : UserControl
    {
        private Controller _controller;
        private List<FilterControl> _filterControls;

        public FilterPanelControl()
        {
            InitializeComponent();
        }

        internal List<FilterControl> Initialize(Controller controller)
        {
            _controller = controller;
            _filterControls = new List<FilterControl>();

            flowLayoutPanelFilters.SuspendLayout();
            flowLayoutPanelFilters.Controls.Clear();

            AddFilters();

            flowLayoutPanelFilters.ResumeLayout();

            labelFilters.Visible = true;
            flowLayoutPanelFilters.Visible = true;
            linkLabelResetFilters.Visible = true;

            return _filterControls;
        }

        internal void HideFilters()
        {
            labelFilters.Visible = false;
            flowLayoutPanelFilters.Visible = false;
            linkLabelResetFilters.Visible = false;

            flowLayoutPanelFilters.Controls.Clear();
            _filterControls.Clear();
        }

        private void AddFilters()
        {
            AddFilter("Date",new FilterControlDate(_controller));

            AddFilter("Case ID",new FilterControlKey(_controller));

            AddFilter("Operator ID",new InfoFilter(Filter.FilterType.Session,"operatorid_info","operatorid"));
            AddFilter("Username",new InstanceFilter(Filter.FilterType.Application,"device_info","username"));
            AddFilter("Device ID",new InstanceFilter(Filter.FilterType.Application,"device_info","deviceid"));

            AddFilter("Application Name",new InstanceFilter(Filter.FilterType.Application,"application_info","name"));
            AddFilter("Application Type",new InstanceFilter(Filter.FilterType.Application,"application_info","type"));
            AddFilter("Entry Mode",new InstanceFilterWithCodes(Filter.FilterType.Session,"session_info","mode",_controller.TableDefinitions));

            AddFilter("Primary Dictionary Name",new DictionaryNameFilter());

            AddFilter("Procedure Name",new ProcNameFilter());

            AddFilter("Operating System",new InstanceFilter(Filter.FilterType.Application,"device_info","os"));
            AddFilter("Device Manufacturer",new InstanceFilter(Filter.FilterType.Application,"device_info","device_manufacturer"));
            AddFilter("Device Model",new InstanceFilter(Filter.FilterType.Application,"device_info","device_model"));
        }

        private const int MinimumExpanderHeight = 30;
        private const int MarginSize = 15;
        private int FilterControlWidth
        {
            get
            {
                int width = this.Width - MarginSize;

                if( flowLayoutPanelFilters.VerticalScroll.Visible )
                    width -= SystemInformation.VerticalScrollBarWidth;

                return width;
            }
        }

        private void flowLayoutPanelFilters_SizeChanged(object sender,EventArgs e)
        {
            // resize all of the controls
            foreach( var panelControl in flowLayoutPanelFilters.Controls )
            {
                var control = (Control)panelControl;

                int widthDifference = FilterControlWidth - control.Width;

                // if the width hasn't changed, there is no need to change the widths
                if( widthDifference == 0 )
                    return;

                control.Width = FilterControlWidth;

                if( control is Expander )
                {
                    var expanderContent = ((Expander)control).Content;
                    expanderContent.Width += widthDifference;
                }
            }
        }

        private void AddFilter(string header,Filter filter)
        {
            AddFilter(header,new FilterControl(_controller,filter));
        }

        private void AddFilter(string header,FilterControl filterControl)
        {
            _filterControls.Add(filterControl);

            var expander = new Expander();
            var headerLabel = new Label();

            expander.Size = new Size(FilterControlWidth,MinimumExpanderHeight);
            expander.Location = new Point(MarginSize,MarginSize);
            expander.BorderStyle = BorderStyle.FixedSingle;
            expander.Header = headerLabel;
            expander.Content = filterControl;

            // the filter should be sized to nearly the width of the expander
            filterControl.Width = expander.Width - ( MarginSize / 2 );

            headerLabel.Text = header;
            headerLabel.AutoSize = false;
            headerLabel.Font = new Font(headerLabel.Font,FontStyle.Italic);
            headerLabel.TextAlign = ContentAlignment.MiddleLeft;
            headerLabel.Tag = expander;
            headerLabel.Click += FilterHeaderLabel_Click;

            // by default show the filter collapsed
            expander.Collapse();

            flowLayoutPanelFilters.Controls.Add(expander);
        }

        private async void FilterHeaderLabel_Click(object sender,EventArgs e)
        {
            if( e is MouseEventArgs )
            {
                var label = (Label)sender;
                var expander = (Expander)label.Tag;
                var filterControl = (FilterControl)expander.Content;
                var mouseEventArgs = (MouseEventArgs)e;

                await filterControl.ProcessClickAsync(expander,mouseEventArgs);

                if( mouseEventArgs.Button == MouseButtons.Left )
                    expander.Toggle();
            }
        }

        private void linkLabelResetFilters_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            // reset the checks
            foreach( var filterControl in _filterControls )
                filterControl.ResetFilter();

            // collapse the expanders
            foreach( var control in flowLayoutPanelFilters.Controls )
            {
                if( control is Expander )
                    ((Expander)control).Collapse();
            }

            _controller.RefreshFilters();
        }
    }
}
