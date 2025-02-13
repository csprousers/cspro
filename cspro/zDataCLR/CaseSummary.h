#pragma once


namespace CSPro
{
    namespace Data
    {
        public enum class PartialSaveMode
        {
            None,
            Add,
            Modify,
            Verify
        };

        public value struct CaseSummary
        {
            System::String^ Key;
            double PositionInRepository;
            System::String^ CaseLabel;
            bool Deleted;
            bool Verified;
            PartialSaveMode PartialSaveMode;

            property System::String^ KeyForSingleLineDisplay { System::String^ get(); }
            property System::String^ CaseLabelForSingleLineDisplay { System::String^ get(); }
        };
    }
}
