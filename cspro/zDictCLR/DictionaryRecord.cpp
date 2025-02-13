#include "Stdafx.h"
#include "DictionaryRecord.h"

using namespace System;
using namespace CSPro::Dictionary;


DictionaryRecord::DictionaryRecord(CDictRecord* pNativeRecord) :
    m_pNativeRecord(pNativeRecord)
{
    ASSERT(pNativeRecord);
}

String^ DictionaryRecord::Name::get()
{
    return gcnew String(m_pNativeRecord->GetName());
}

void DictionaryRecord::Name::set(String^ name)
{
    m_pNativeRecord->SetName((CString)name);
}

String^ DictionaryRecord::Label::get()
{
    return gcnew String(m_pNativeRecord->GetLabel());
}

void DictionaryRecord::Label::set(String^ label)
{
    m_pNativeRecord->SetLabel((CString)label);
}

String^ DictionaryRecord::Note::get()
{
    return gcnew String(m_pNativeRecord->GetNote());
}

void DictionaryRecord::Note::set(String^ note)
{
    m_pNativeRecord->SetNote((CString)note);
}

String^ DictionaryRecord::RecordType::get()
{
    return gcnew String(m_pNativeRecord->GetRecTypeVal());
}

void DictionaryRecord::RecordType::set(String^ rt)
{
    m_pNativeRecord->SetRecTypeVal((CString)rt);
}

array<DictionaryItem^>^ DictionaryRecord::Items::get()
{
    array<DictionaryItem^>^ records = gcnew array<DictionaryItem^>(m_pNativeRecord->GetNumItems());
    for (int i = 0; i < m_pNativeRecord->GetNumItems(); ++i)
        records[i] = gcnew DictionaryItem(m_pNativeRecord->GetItem(i));
    return records;
}

DictionaryItem^ DictionaryRecord::AddItem()
{
    CDictItem item;
    m_pNativeRecord->AddItem(&item);
    return gcnew DictionaryItem(m_pNativeRecord->GetItem(m_pNativeRecord->GetNumItems() - 1));
}

bool DictionaryRecord::Required::get()
{
    return m_pNativeRecord->GetRequired();
}

void DictionaryRecord::Required::set(bool required)
{
    m_pNativeRecord->SetRequired(required);
}

int DictionaryRecord::MaxRecs::get()
{
    return m_pNativeRecord->GetMaxRecs();
}

void DictionaryRecord::MaxRecs::set(int maxRecs)
{
    m_pNativeRecord->SetMaxRecs(maxRecs);
}

int DictionaryRecord::Length::get()
{
    return m_pNativeRecord->GetRecLen();
}

void DictionaryRecord::Length::set(int len)
{
    m_pNativeRecord->SetRecLen(len);
}
