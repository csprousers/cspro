#include "Stdafx.h"
#include "DataDictionary.h"
#include <zUtilO/FileExtensions.h>

using namespace System;
using namespace CSPro::Dictionary;

String^ DataDictionary::Extension::get()
{
     return clr_helpers::to_SystemString(FileExtensions::WithDot(FileExtensions::Dictionary));
}

DataDictionary::DataDictionary(CDataDict* pNativeDict, bool owns_dictionary)
    :   m_pNativeDict(pNativeDict),
        m_bOwnsDictionary(owns_dictionary)
{
}

DataDictionary::DataDictionary(CDataDict* pNativeDict)
    :   DataDictionary(pNativeDict, false)
{
}

DataDictionary::DataDictionary()
    :   DataDictionary(new CDataDict(), true)
{
}

DataDictionary::DataDictionary(String^ file_path)
    :   DataDictionary()
{
    try
    {
        m_pNativeDict->Open(clr_helpers::to_string(file_path), true);
    }

    catch( const CSProException& exception )
    {
        throw gcnew Exception(clr_helpers::to_SystemString(exception.what()));
    }
}

DataDictionary::!DataDictionary()
{
    if( m_bOwnsDictionary )
        delete m_pNativeDict;
}

void DataDictionary::Save(System::String^ file_path)
{
    try
    {
        m_pNativeDict->Save(clr_helpers::to_string(file_path));
    }

    catch( const CSProException& exception )
    {
        throw gcnew Exception(clr_helpers::to_SystemString(exception.what()));
    }
}

System::String^ DataDictionary::Name::get()
{
    return clr_helpers::to_SystemString(m_pNativeDict->GetName());
}

void DataDictionary::Name::set(System::String^ name)
{
    m_pNativeDict->SetName(clr_helpers::to_string(name));
}

System::String^ DataDictionary::Label::get()
{
    return gcnew System::String(m_pNativeDict->GetLabel());
}

void DataDictionary::Label::set(System::String^ label)
{
    m_pNativeDict->SetLabel((CString)label);
}

bool DataDictionary::AllowExport::get()
{
    return m_pNativeDict->GetAllowExport();
}

System::String^ DataDictionary::Note::get()
{
    return gcnew System::String(m_pNativeDict->GetNote());
}

void DataDictionary::Note::set(System::String^ note)
{
    m_pNativeDict->SetNote((CString)note);
}

int DataDictionary::RecordTypeStart::get()
{
    return m_pNativeDict->GetRecTypeStart();
}

void DataDictionary::RecordTypeStart::set(int n)
{
    m_pNativeDict->SetRecTypeStart(n);
}

int DataDictionary::RecordTypeLength::get()
{
    return m_pNativeDict->GetRecTypeLen();
}

void DataDictionary::RecordTypeLength::set(int n)
{
    m_pNativeDict->SetRecTypeLen(n);
}

bool DataDictionary::ZeroFill::get()
{
    return m_pNativeDict->IsZeroFill();
}

void DataDictionary::ZeroFill::set(bool b)
{
    m_pNativeDict->SetZeroFill(b);
}

array<DictionaryLevel^>^ DataDictionary::Levels::get()
{
    array<DictionaryLevel^>^ levels = gcnew array<DictionaryLevel^>(m_pNativeDict->GetNumLevels());

    for( size_t level_number = 0; level_number < m_pNativeDict->GetNumLevels(); ++level_number )
        levels[level_number] = gcnew DictionaryLevel(&m_pNativeDict->GetLevel(level_number));

    return levels;
}


array<Tuple<String^, String^>^>^ DataDictionary::Languages::get()
{
    const auto& dictionary = *m_pNativeDict;
    const auto& languages = dictionary.GetLanguages();

    auto clr_languages = gcnew array<Tuple<String^, String^>^>(languages.size());

    size_t index = 0;

    for( const auto& language : languages )
    {
        clr_languages[index++] = gcnew Tuple<String^, String^>(gcnew String(language.GetName().c_str()),
                                                               gcnew String(language.GetLabel().c_str()));
    }

    return clr_languages;
}


CDataDict* CSPro::Dictionary::DataDictionary::GetNativePointer()
{
    return m_pNativeDict;
}
