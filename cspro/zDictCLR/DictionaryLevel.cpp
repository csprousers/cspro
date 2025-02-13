#include "Stdafx.h"
#include "DictionaryLevel.h"

using namespace System;
using namespace CSPro::Dictionary;

DictionaryLevel::DictionaryLevel(DictLevel* pNativeLevel) :
    m_pNativeLevel(pNativeLevel)
{
    ASSERT(pNativeLevel);
}

DictLevel* DictionaryLevel::GetNativePointer()
{
    return m_pNativeLevel;
}

String^ DictionaryLevel::Name::get()
{
    return gcnew String(m_pNativeLevel->GetName());
}

void DictionaryLevel::Name::set(System::String^ name)
{
    m_pNativeLevel->SetName((CString)name);
}

String^ DictionaryLevel::Label::get()
{
    return gcnew String(m_pNativeLevel->GetLabel());
}

void DictionaryLevel::Label::set(System::String^ label)
{
    m_pNativeLevel->SetLabel((CString)label);
}

String^ DictionaryLevel::Note::get()
{
    return gcnew String(m_pNativeLevel->GetNote());
}

void DictionaryLevel::Note::set(System::String^ note)
{
    m_pNativeLevel->SetNote((CString)note);
}

DictionaryRecord^ DictionaryLevel::IdItems::get()
{
    return gcnew DictionaryRecord(m_pNativeLevel->GetIdItemsRec());
}

array<DictionaryRecord^>^ DictionaryLevel::Records::get()
{
    array<DictionaryRecord^>^ records = gcnew array<DictionaryRecord^>(m_pNativeLevel->GetNumRecords());
    for (int i = 0; i < m_pNativeLevel->GetNumRecords(); ++i)
        records[i] = gcnew DictionaryRecord(m_pNativeLevel->GetRecord(i));
    return records;
}

DictionaryRecord^ CSPro::Dictionary::DictionaryLevel::AddRecord()
{
    CDictRecord record;
    m_pNativeLevel->AddRecord(&record);
    return gcnew DictionaryRecord(m_pNativeLevel->GetRecord(m_pNativeLevel->GetNumRecords() - 1));
}
