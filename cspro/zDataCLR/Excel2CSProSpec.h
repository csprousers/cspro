#pragma once


namespace CSPro::Data::Excel2CSPro
{
    public enum class CaseManagement
    {
        CreateNewFile = 0,
        ModifyAddCases = 1,
        ModifyAddDeleteCases = 2
    };


    public ref class ItemMapping sealed
    {
    public:
        property System::String^ ItemName;
        property System::Nullable<int> Occurrence;
        property int ColumnIndex;
    };

    public ref class RecordMapping sealed
    {
    public:
        RecordMapping()
        {
            ItemMappings = gcnew System::Collections::Generic::List<ItemMapping^>;
        }

        property System::String^ RecordName;
        property System::String^ WorksheetName;
        property int WorksheetIndex;

        property System::Collections::Generic::List<ItemMapping^>^ ItemMappings;
    };


    public ref class Spec sealed
    {
    public:
        Spec();

        property System::String^ ExcelFilename;
        property System::String^ DictionaryFilename;
        property CSPro::Util::ConnectionString^ OutputConnectionString;

        property int StartingRow;
        property CaseManagement CaseManagement;
        property bool RunOnlyIfNewer;

        property System::Collections::Generic::List<RecordMapping^>^ Mappings;

        // Load and Save throw exceptions
        void Load(System::String^ filename);
        void Save(System::String^ filename);
    };
}
