#include "Stdafx.h"
#include "DictionaryItem.h"


CSPro::Dictionary::DictionaryItem::DictionaryItem(CDictItem* pNativeItem)
    :   m_pNativeItem(pNativeItem)
{
    ASSERT(pNativeItem != nullptr);
}

System::String^ CSPro::Dictionary::DictionaryItem::Name::get()
{
    return gcnew System::String(m_pNativeItem->GetName());
}

void CSPro::Dictionary::DictionaryItem::Name::set(System::String^ name)
{
    m_pNativeItem->SetName((CString)name);
}

System::String^ CSPro::Dictionary::DictionaryItem::Label::get()
{
    return gcnew System::String(m_pNativeItem->GetLabel());
}

void CSPro::Dictionary::DictionaryItem::Label::set(System::String^ label)
{
    m_pNativeItem->SetLabel((CString)label);
}

System::String^ CSPro::Dictionary::DictionaryItem::Note::get()
{
    return gcnew System::String(m_pNativeItem->GetNote());
}

void CSPro::Dictionary::DictionaryItem::Note::set(System::String^ note)
{
    m_pNativeItem->SetNote((CString)note);
}

int CSPro::Dictionary::DictionaryItem::Start::get()
{
    return m_pNativeItem->GetStart();
}

void CSPro::Dictionary::DictionaryItem::Start::set(int s)
{
    m_pNativeItem->SetStart(s);
}

int CSPro::Dictionary::DictionaryItem::Length::get()
{
    return m_pNativeItem->GetLen();
}

void CSPro::Dictionary::DictionaryItem::Length::set(int len)
{
    m_pNativeItem->SetLen(len);
}

int CSPro::Dictionary::DictionaryItem::Occurrences::get()
{
    return m_pNativeItem->GetOccurs();
}

void CSPro::Dictionary::DictionaryItem::Occurrences::set(int occ)
{
    m_pNativeItem->SetOccurs(occ);
}

CSPro::Dictionary::ItemType CSPro::Dictionary::DictionaryItem::ItemType::get()
{
    return m_pNativeItem->GetItemType() == ::ItemType::Item ? CSPro::Dictionary::ItemType::Item :
                                                              CSPro::Dictionary::ItemType::Subitem;
}

void CSPro::Dictionary::DictionaryItem::ItemType::set(CSPro::Dictionary::ItemType t)
{
    m_pNativeItem->SetItemType(t == CSPro::Dictionary::ItemType::Item ? ::ItemType::Item :
                                                                        ::ItemType::Subitem);
}

CSPro::Dictionary::ContentType CSPro::Dictionary::DictionaryItem::ContentType::get()
{
    return (CSPro::Dictionary::ContentType)m_pNativeItem->GetContentType();
}

void CSPro::Dictionary::DictionaryItem::ContentType::set(CSPro::Dictionary::ContentType t)
{
    m_pNativeItem->SetContentType((::ContentType)t);
}

bool CSPro::Dictionary::DictionaryItem::DecimalChar::get()
{
    return m_pNativeItem->GetDecChar();
}

void CSPro::Dictionary::DictionaryItem::DecimalChar::set(bool b)
{
    m_pNativeItem->SetDecChar(b);
}

int CSPro::Dictionary::DictionaryItem::DecimalPlaces::get()
{
    return m_pNativeItem->GetDecimal();
}

void CSPro::Dictionary::DictionaryItem::DecimalPlaces::set(int p)
{
    m_pNativeItem->SetDecimal(p);
}

bool CSPro::Dictionary::DictionaryItem::ZeroFill::get()
{
    return m_pNativeItem->GetZeroFill();
}

void CSPro::Dictionary::DictionaryItem::ZeroFill::set(bool b)
{
    m_pNativeItem->SetZeroFill(b);
}

array<CSPro::Dictionary::ValueSet^>^ CSPro::Dictionary::DictionaryItem::ValueSets::get()
{
    array<ValueSet^>^ value_sets = gcnew array<ValueSet^>(m_pNativeItem->GetNumValueSets());

    int v = 0;
    for( const auto& dict_value_set : m_pNativeItem->GetValueSets() )
        value_sets[v++] = gcnew ValueSet(dict_value_set);

    return value_sets;
}
