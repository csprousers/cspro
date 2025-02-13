#include "Stdafx.h"
#include "SaveArrayFile.h"
#include <zMessageO/SystemMessageIssuer.h>
#include <zEngineO/Array.h>
#include <zEngineO/SaveArrayFile.h>


namespace
{
    class SaveArrayFileClrErrorReporter : public SystemMessageIssuer
    {
    public:
        bool HasErrors() const { return !m_errors.empty(); }

        const std::wstring& GetErrors() const { return m_errors; }

    protected:
        void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text) override
        {
            SO::AppendWithSeparator(m_errors, message_text, _T("\r\n\r\n"));
        }

    private:
        std::wstring m_errors;
    };
}


CSPro::Engine::SaveArrayFile::SaveArrayFile()
    :   m_logicArrays(new std::vector<LogicArray*>)
{
}


CSPro::Engine::SaveArrayFile::!SaveArrayFile()
{
    for( LogicArray* logic_array : *m_logicArrays )
        delete logic_array;

    delete m_logicArrays;
}


void CSPro::Engine::SaveArrayFile::LoadOrSave(System::String^ filename, bool loading)
{
    ::SaveArrayFile save_array_file;
    auto save_array_file_clr_error_reporter = std::make_shared<SaveArrayFileClrErrorReporter>();

    if( loading )
    {
        save_array_file.ReadArrays(ToWS(filename), *m_logicArrays, save_array_file_clr_error_reporter, true);
    }

    else
    {
        save_array_file.WriteArrays(ToWS(filename), *m_logicArrays, save_array_file_clr_error_reporter, 0, false);
    }

    if( save_array_file_clr_error_reporter->HasErrors() )
        throw gcnew System::Exception(gcnew System::String(save_array_file_clr_error_reporter->GetErrors().c_str()));
}


namespace
{
    class ValueCopier : public SaveArrayViewerHelpers::ValueCopier
    {
    public:
        ValueCopier(CSPro::Engine::SaveArrayValues^ save_array_values)
            :   m_saveArrayValues(save_array_values)
        {
        }
        
        std::tuple<std::wstring, size_t, size_t> GetValues(size_t index) const override
        {
            return
            {
                ToWS(m_saveArrayValues->Values[index]),
                static_cast<size_t>(m_saveArrayValues->Gets[index]),
                static_cast<size_t>(m_saveArrayValues->Puts[index])
            };
        }

        void SetValues(size_t index, const std::wstring& value, size_t gets, size_t puts) override
        {
            m_saveArrayValues->Values[index] = gcnew System::String(value.c_str());
            m_saveArrayValues->Gets[index] = gets;
            m_saveArrayValues->Puts[index] = puts;
        }

    private:
        gcroot<CSPro::Engine::SaveArrayValues^> m_saveArrayValues;
    };
}


System::Collections::Generic::List<CSPro::Engine::SaveArrayValues^>^ CSPro::Engine::SaveArrayFile::GetSaveArrayValues()
{
    auto list_save_array_values = gcnew System::Collections::Generic::List<CSPro::Engine::SaveArrayValues^>();

    for( const LogicArray& logic_array : VI_V(*m_logicArrays) )
    {
        auto save_array_values = gcnew CSPro::Engine::SaveArrayValues();

        save_array_values->Name = gcnew System::String(logic_array.GetName().c_str());

        size_t number_cells = 1;

        save_array_values->Dimensions = gcnew System::Collections::Generic::List<int>;

        for( size_t dimension : logic_array.GetDimensions() )
        {
            save_array_values->Dimensions->Add(dimension);
            number_cells *= dimension;
        }

        save_array_values->Numeric = logic_array.IsNumeric();
        save_array_values->PaddingStringLength = logic_array.GetPaddingStringLength();        

        const SaveArray* save_array = logic_array.GetSaveArray();
        save_array_values->Runs = save_array->GetNumberRuns();
        save_array_values->Cases = save_array->GetNumberCases();

        // convert the values
        save_array_values->Values = gcnew array<System::String^>(number_cells);
        save_array_values->Gets = gcnew array<int>(number_cells);
        save_array_values->Puts = gcnew array<int>(number_cells);

        ValueCopier value_copier(save_array_values);
        SaveArrayViewerHelpers::CopyValues(const_cast<LogicArray&>(logic_array), value_copier, true);

        list_save_array_values->Add(save_array_values);
    }

    return list_save_array_values;
}


void CSPro::Engine::SaveArrayFile::SetSaveArrayValues(System::Collections::Generic::List<CSPro::Engine::SaveArrayValues^>^ list_save_array_values)
{
    ASSERT(m_logicArrays->empty());

    for each( auto save_array_values in list_save_array_values )
    {
        std::vector<size_t> dimensions;

        for each( int dimension in save_array_values->Dimensions )
        {
            // only add specified dimensions
            if( dimension == 1 )
                break;

            dimensions.emplace_back(dimension);
        }

        // create a new logic array
        LogicArray* logic_array = SaveArrayViewerHelpers::CreateLogicArray(ToWS(save_array_values->Name), std::move(dimensions));

        logic_array->SetNumeric(save_array_values->Numeric);
        logic_array->SetPaddingStringLength(save_array_values->PaddingStringLength);

        SaveArray* save_array = logic_array->GetSaveArray();
        save_array->SetNumberRuns(save_array_values->Runs);
        save_array->SetNumberCases(save_array_values->Cases);

        // convert the values
        ValueCopier value_copier(save_array_values);
        SaveArrayViewerHelpers::CopyValues(*logic_array, value_copier, false);

        m_logicArrays->emplace_back(logic_array);
    }
}
