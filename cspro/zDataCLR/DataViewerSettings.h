#pragma once

#include <zCaseO/CaseToHtmlConverter.h>
#include <zDataO/DataRepositoryDefines.h>


namespace CSPro
{
    namespace Data
    {
        [System::SerializableAttribute()]
        public ref class DataViewerSettings sealed
        {
        private:
            DataViewerSettings();

        public:
            static DataViewerSettings^ Load();

            ~DataViewerSettings()
            {
                this->!DataViewerSettings();
            }

            !DataViewerSettings();

            void Reset()
            {
                m_caseIterationCaseStatus = CaseIterationCaseStatus::NotDeletedOnly;
                m_keyPrefix = nullptr;
            }

            property System::Windows::Forms::FormWindowState WindowState
            {
                System::Windows::Forms::FormWindowState get()           { return m_windowState; }
                void set(System::Windows::Forms::FormWindowState value) { m_windowState = value; }
            }

            property System::Drawing::Size WindowSize
            {
                System::Drawing::Size get()           { return m_windowSize; }
                void set(System::Drawing::Size value) { m_windowSize = value; }
            }

            CaseIterationMethod GetCaseIterationMethod()         { return m_caseIterationMethod; }
            CaseIterationOrder GetCaseIterationOrder()           { return m_caseIterationOrder; }
            CaseIterationCaseStatus GetCaseIterationCaseStatus() { return m_caseIterationCaseStatus; }

            property System::String^ KeyPrefix
            {
                System::String^ get()           { return m_keyPrefix; }
                void set(System::String^ value) { m_keyPrefix = value; }
            }

            property bool ShowCaseLabels
            {
                bool get()           { return m_showCaseLabels; }
                void set(bool value) { m_showCaseLabels = value; }
            }

            property bool ShowKeyPrefixPanel
            {
                bool get()           { return m_showKeyPrefixPanel; }
                void set(bool value) { m_showKeyPrefixPanel = value; }
            }

            property System::String^ LanguageName
            {
                System::String^ get()           { return m_languageName; }
                void set(System::String^ value) { m_languageName = value; }
            }

            void UpdateMenuChecks(System::Collections::Generic::List<System::Windows::Forms::ToolStripMenuItem^>^ menu_items);

            property System::Collections::Generic::List<System::String^>^ RecentFiles
            {
                System::Collections::Generic::List<System::String^>^ get();
            }

            void AddToRecentFilesList(System::String^ filename);

            // for the Exporter
            property System::Collections::Generic::List<System::String^>^ ExportFormatSelections
            {
                System::Collections::Generic::List<System::String^>^ get();
            }

            property bool ExportOneFilePerRecord
            {
                bool get()           { return m_exportOneFilePerRecord; }
                void set(bool value) { m_exportOneFilePerRecord = value; }
            }

        internal:
            void ApplyCaseToHtmlConverterSettings(CaseToHtmlConverter& case_to_html_converter);
            void ChangeSetting(System::Windows::Forms::ToolStripMenuItem^ menu_item, bool called_via_accelerator);

        private:
            System::Windows::Forms::FormWindowState m_windowState;
            System::Drawing::Size m_windowSize;
            System::Collections::Generic::List<System::String^>^ m_recentFilesList;

            CaseIterationMethod m_caseIterationMethod;
            CaseIterationOrder m_caseIterationOrder;
            CaseIterationCaseStatus m_caseIterationCaseStatus;
            System::String^ m_keyPrefix;
            bool m_showCaseLabels;
            bool m_showKeyPrefixPanel;

            CaseToHtmlConverter::Statuses m_statuses;
            CaseToHtmlConverter::CaseConstructionErrors m_caseConstructionErrors;
            CaseToHtmlConverter::NameDisplay m_nameDisplay;
            CaseToHtmlConverter::RecordOrientation m_recordOrientation;
            CaseToHtmlConverter::OccurrenceDisplay m_occurrenceDisplay;
            CaseToHtmlConverter::ItemTypeDisplay m_itemTypeDisplay;
            CaseToHtmlConverter::BlankValues m_blankValues;
            CaseItemPrinter::Format m_caseItemPrinterFormat;
            System::String^ m_languageName;

            System::Collections::Generic::List<System::String^>^ m_exportFormatSelections;
            bool m_exportOneFilePerRecord;
        };
    }
}
