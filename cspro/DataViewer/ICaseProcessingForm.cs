namespace DataViewer
{
    interface ICaseProcessingForm
    {
        // the current case being used by the form
        CSPro.Data.Case CurrentCase { get; }

        // called when the user clicks on a new case in the case listing; data_case will not be null
        void RefreshCase(CSPro.Data.Case data_case);

        // called after a data file refresh; data_case can be null
        void UpdateCaseFromRepository(CSPro.Data.Case data_case);

        // called when the settings have been changed
        void RefreshSettings();
    }
}
