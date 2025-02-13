//***************************************************************************
//  File name: DDClass.cpp
//
//  Description:
//       Data Dictionary classes definitions and implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              02 Aug 00   bmd     Created for CSPro 2.1
//              03 Nov 00   RHF     Modify function DoIssaRanges
//
//***************************************************************************

#include "StdAfx.h"
#include "DDClass.h"
#include "DictionaryValidator.h"
#include <zToolsO/Encryption.h>
#include <zToolsO/Hash.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/ProcessSummary.h>
#include <zUtilO/TemporaryFile.h>


namespace
{
    inline void DeserializeSecurityOptions(const CString& encrypted_security_options, const CString& dictionary_name,
                                           bool& allow_data_viewer_modifications, bool& allow_export, int& cached_password_minutes)
    {
        CString security_options = WS2CS(Encryptor(Encryptor::Type::RijndaelHex, dictionary_name).Decrypt(encrypted_security_options));
        int pos = 0;

        for( int i = 0; i < 5; ++i )
        {
            CString argument = security_options.Tokenize(_T("\t"), pos);

            // only process text that was correctly decrypted
            if( i == 0 )
            {
                if( argument.Compare(dictionary_name) != 0 )
                    return;
            }

            else if( i == 1 )
            {
                if( argument.Compare(_T("v1")) != 0 )
                    return;
            }

            else if( i == 2 )
            {
                allow_data_viewer_modifications = ( CIMSAString::Val(argument) == 1 );
            }

            else if( i == 3 )
            {
                allow_export = ( CIMSAString::Val(argument) == 1 );
            }

            else if( i == 4 )
            {
                cached_password_minutes = (int)CIMSAString::Val(argument);
            }
        }
    }
}

// this is out of the namespace because it is used by the pre-8.0 spec file converter
inline std::wstring SerializeSecurityOptions(const TCHAR* dictionary_name, bool allow_data_viewer_modifications,
                                             bool allow_export, int cached_password_minutes)
{
    CString security_options = FormatText(_T("%s\tv1\t%d\t%d\t%d"), dictionary_name,
        allow_data_viewer_modifications ? 1 : 0, allow_export ? 1 : 0, cached_password_minutes);

    return Encryptor(Encryptor::Type::RijndaelHex, dictionary_name).Encrypt(security_options);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CDataDict::CDataDict
//
/////////////////////////////////////////////////////////////////////////////

CDataDict::CDataDict()
    :   m_uRecTypeStart(DictionaryDefaults::RecTypeStart),
        m_uRecTypeLen(DictionaryDefaults::RecTypeLen),
        m_bPosRelative(DictionaryDefaults::RelativePositions),
        m_bZeroFill(DictionaryDefaults::ZeroFill),
        m_bDecChar(DictionaryDefaults::DecChar),
        m_allowDataViewerModifications(false),
        m_allowExport(false),
        m_cachedPasswordMinutes(0),
        m_readOptimization(DictionaryDefaults::ReadOptimization),
        m_iSymbol(-1),
        m_pChangedObject(nullptr),
        m_enableBinaryItems(false)
{
    // add a default language
    m_languages.emplace_back();
}


CDataDict::CDataDict(const CDataDict& rhs)
    :   DictNamedBase(rhs),
        m_uRecTypeStart(rhs.m_uRecTypeStart),
        m_uRecTypeLen(rhs.m_uRecTypeLen),
        m_bPosRelative(rhs.m_bPosRelative),
        m_bZeroFill(rhs.m_bZeroFill),
        m_bDecChar(rhs.m_bDecChar),
        m_mapNames(rhs.m_mapNames),
        m_allowDataViewerModifications(rhs.m_allowDataViewerModifications),
        m_allowExport(rhs.m_allowExport),
        m_cachedPasswordMinutes(rhs.m_cachedPasswordMinutes),
        m_readOptimization(rhs.m_readOptimization),
        m_filename(rhs.m_filename),
        m_serializedFileModifiedTime(rhs.m_serializedFileModifiedTime),
        m_iSymbol(-1),
        m_pChangedObject(nullptr),
        m_enableBinaryItems(rhs.m_enableBinaryItems),
        m_languages(rhs.m_languages),
        m_dictLevels(rhs.m_dictLevels),
        m_dictRelations(rhs.m_dictRelations)
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CDataDict::GetNumRecords
//
/////////////////////////////////////////////////////////////////////////////

size_t CDataDict::GetNumRecords() const
{
    size_t num_records = 0;

    for( const DictLevel& dict_level : m_dictLevels )
        num_records += dict_level.GetNumRecords();

    return num_records;
}


void CDataDict::CopyDictionarySettings(const CDataDict& dictionary)
{
    SetZeroFill(dictionary.IsZeroFill());
    SetDecChar(dictionary.IsDecChar());
    SetAllowDataViewerModifications(dictionary.GetAllowDataViewerModifications());
    SetAllowExport(dictionary.GetAllowExport());
    SetCachedPasswordMinutes(dictionary.GetCachedPasswordMinutes());
}


std::unique_ptr<ProcessSummary> CDataDict::CreateProcessSummary() const
{
    auto process_summary = std::make_unique<ProcessSummary>();

    process_summary->SetAttributesType(ProcessSummary::AttributesType::Records);
    process_summary->SetNumberLevels(GetNumLevels());

    return process_summary;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::Check
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN_DESKTOP
bool CDataDict::IsValid(CString& csError)
{
    DictionaryValidator ddRule;
    ddRule.SetCurrDict(this);

    if( !ddRule.IsValid(this,true,true) )
    {
        csError = ddRule.GetErrorReport();
        return false;
    }

    return true;
}
#endif


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::operator=
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::operator=(CDataDict& dict)
{
    DictNamedBase::operator=(dict);

    m_uRecTypeStart = dict.m_uRecTypeStart;
    m_uRecTypeLen   = dict.m_uRecTypeLen;
    m_bPosRelative  = dict.m_bPosRelative;
    m_bZeroFill     = dict.m_bZeroFill;
    m_bDecChar      = dict.m_bDecChar;
    m_allowDataViewerModifications = dict.m_allowDataViewerModifications;
    m_allowExport = dict.m_allowExport;
    m_cachedPasswordMinutes = dict.m_cachedPasswordMinutes;
    m_readOptimization = dict.m_readOptimization;
    m_enableBinaryItems = dict.m_enableBinaryItems;
    m_dictLevels = dict.m_dictLevels;
    m_dictRelations = dict.m_dictRelations;

    m_mapNames = dict.m_mapNames;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::BuildNameList
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::BuildNameList()
{
    m_mapNames.clear();

    AddToNameList(*this);

    for( size_t level_number = 0; level_number < m_dictLevels.size(); ++level_number )
    {
        const DictLevel& dict_level = m_dictLevels[level_number];
        AddToNameList(dict_level, level_number);

        auto add_record_and_below = [&](int r)
        {
            const CDictRecord* dict_record = dict_level.GetRecord(r);

            if( r != COMMON )
            {
                AddToNameList(*dict_record, level_number, r);
            }

            else
            {
                // don't add the _IDS name
                ASSERT(dict_record->GetName()[0] == '_');
            }

            for( int i = 0; i < dict_record->GetNumItems(); ++i )
            {
                const CDictItem* dict_item = dict_record->GetItem(i);
                AddToNameList(*dict_item, level_number, r, i);

                int v = 0;
                for( const auto& dict_value_set : dict_item->GetValueSets() )
                {
                    AddToNameList(dict_value_set, level_number, r, i, v);
                    v++;
                }
            }
        };

        add_record_and_below(COMMON);

        for( int r = 0 ; r < dict_level.GetNumRecords(); ++r )
            add_record_and_below(r);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::UpdateNameList
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::UpdateNameList(const DictNamedBase& dict_element,
                               int iLevel /*=NONE*/,
                               int iRec   /*=NONE*/,
                               int iItem  /*=NONE*/,
                               int iVSet  /*=NONE*/)
{
    RemoveFromNameList(iLevel, iRec, iItem, iVSet);
    AddToNameList(dict_element, iLevel, iRec, iItem, iVSet);
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::UpdateNameList
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::UpdateNameList(int iLevel, int iRec)
{
    // remove all the names for this record
    for( auto name_map_pair = m_mapNames.cbegin(); name_map_pair != m_mapNames.cend(); )
    {
        const CDictName& name = name_map_pair->second;

        if( name.m_iLevel == iLevel && name.m_iRec == iRec )
            name_map_pair = m_mapNames.erase(name_map_pair);

        else
            name_map_pair++;
    }

    // now add them back in based on current dd
    const CDictRecord* dict_record = GetLevel(iLevel).GetRecord(iRec);

    if( !dict_record->IsIdRecord() )
        AddToNameList(*dict_record, iLevel, iRec);

    for( int i = 0 ; i < dict_record->GetNumItems(); ++i )
    {
        const CDictItem* dict_item = dict_record->GetItem(i);
        AddToNameList(*dict_item, iLevel, iRec, i);

        int v = 0;
        for( const auto& dict_value_set : dict_item->GetValueSets() )
        {
            AddToNameList(dict_value_set, iLevel, iRec, i, v);
            ++v;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::AddToNameList
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::AddToNameList(const CString& name, int iLevel /*=NONE*/,
                                                   int iRec /*=NONE*/,
                                                   int iItem /*=NONE*/,
                                                   int iVSet /*=NONE*/)
{
    if( !name.IsEmpty() )
    {
        ASSERT(SO::IsUpper(name) && CIMSAString::IsName(name));
        ASSERT(m_mapNames.find(name) == m_mapNames.cend());

        m_mapNames.emplace(name, CDictName(iLevel, iRec, iItem, iVSet));
    }
}

void CDataDict::AddToNameList(const DictNamedBase& dict_element, int iLevel /*=NONE*/,
                                                                 int iRec /*=NONE*/,
                                                                 int iItem /*=NONE*/,
                                                                 int iVSet /*=NONE*/)
{
    AddToNameList(dict_element.GetName(), iLevel, iRec, iItem, iVSet);

    for( const CString& alias : dict_element.GetAliases() )
        AddToNameList(alias, iLevel, iRec, iItem, iVSet);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::RemoveFromNameList
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::RemoveFromNameList(int iLevel /*=NONE*/, int iRec /*=NONE*/, int iItem /*=NONE*/, int iVSet /*=NONE*/)
{
    for( auto name_map_pair = m_mapNames.cbegin(); name_map_pair != m_mapNames.cend(); )
    {
        const CDictName& name = name_map_pair->second;

        if( name.m_iLevel == iLevel && name.m_iRec == iRec && name.m_iItem == iItem && name.m_iVSet == iVSet )
            name_map_pair = m_mapNames.erase(name_map_pair);

        else
            ++name_map_pair;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::LookupName
//
/////////////////////////////////////////////////////////////////////////////
template<typename T/* = void*/>
bool CDataDict::LookupName(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record/* = nullptr*/,
                           const CDictItem** dict_item/* = nullptr*/, const DictValueSet** dict_value_set/* = nullptr*/) const
{
    ASSERT(SO::IsUpper(name));

    const DictLevel* this_dict_level = nullptr;
    const CDictRecord* this_dict_record = nullptr;
    const CDictItem* this_dict_item = nullptr;
    const DictValueSet* this_dict_value_set = nullptr;

    const auto& name_map_pair_lookup = m_mapNames.find(name);
    bool found = false;

    if( name_map_pair_lookup != m_mapNames.cend() && name_map_pair_lookup->second.m_iLevel != NONE )
    {
        const CDictName& dict_name = name_map_pair_lookup->second;

        if constexpr(std::is_same_v<T, DictLevel>)
        {
            found = ( dict_name.m_iRec == NONE );
        }

        else if constexpr(std::is_same_v<T, CDictRecord>)
        {
            found = ( dict_name.m_iRec != NONE && dict_name.m_iItem == NONE );
        }

        else if constexpr(std::is_same_v<T, CDictItem>)
        {
            found = ( dict_name.m_iItem != NONE && dict_name.m_iVSet == NONE );
        }

        else if constexpr(std::is_same_v<T, DictValueSet>)
        {
            found = ( dict_name.m_iVSet != NONE );
        }

        else
        {
            found = true;
        }


        if( found )
        {
            this_dict_level = &GetLevel(dict_name.m_iLevel);

            if( dict_name.m_iRec != NONE )
            {
                this_dict_record = this_dict_level->GetRecord(dict_name.m_iRec);

                if( dict_name.m_iItem != NONE )
                {
                    this_dict_item = this_dict_record->GetItem(dict_name.m_iItem);

                    if( dict_name.m_iVSet != NONE )
                        this_dict_value_set = &this_dict_item->GetValueSet(dict_name.m_iVSet);
                }
            }
        }
    }

    if( dict_level != nullptr )     *dict_level     = this_dict_level;
    if( dict_record != nullptr )    *dict_record    = this_dict_record;
    if( dict_item != nullptr )      *dict_item      = this_dict_item;
    if( dict_value_set != nullptr ) *dict_value_set = this_dict_value_set;

    return found;
}

template CLASS_DECL_ZDICTO bool CDataDict::LookupName<void>(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<DictLevel>(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<CDictRecord>(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<CDictItem>(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<DictValueSet>(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;


template<typename T>
const T* CDataDict::LookupName(const CString& name) const
{
    ASSERT(SO::IsUpper(name));

    const auto& name_map_pair_lookup = m_mapNames.find(name);
    bool found = ( name_map_pair_lookup != m_mapNames.cend() );

    if( found )
    {
        const CDictName& dict_name = name_map_pair_lookup->second;

        if constexpr(std::is_same_v<T, DictLevel>)
        {
            if( dict_name.m_iLevel != NONE && dict_name.m_iRec == NONE )
                return &GetLevel(dict_name.m_iLevel);
        }

        else if constexpr(std::is_same_v<T, CDictRecord>)
        {
            if( dict_name.m_iRec != NONE && dict_name.m_iItem == NONE )
                return GetLevel(dict_name.m_iLevel).GetRecord(dict_name.m_iRec);
        }

        else if constexpr(std::is_same_v<T, CDictItem>)
        {
            if( dict_name.m_iItem != NONE && dict_name.m_iVSet == NONE )
                return GetLevel(dict_name.m_iLevel).GetRecord(dict_name.m_iRec)->GetItem(dict_name.m_iItem);
        }

        else
        {
            if( dict_name.m_iVSet != NONE )
                return &GetLevel(dict_name.m_iLevel).GetRecord(dict_name.m_iRec)->GetItem(dict_name.m_iItem)->GetValueSet(dict_name.m_iVSet);
        }
    }

    return nullptr;
}

template CLASS_DECL_ZDICTO const DictLevel* CDataDict::LookupName<DictLevel>(const CString& name) const;
template CLASS_DECL_ZDICTO const CDictRecord* CDataDict::LookupName<CDictRecord>(const CString& name) const;
template CLASS_DECL_ZDICTO const CDictItem* CDataDict::LookupName<CDictItem>(const CString& name) const;
template CLASS_DECL_ZDICTO const DictValueSet* CDataDict::LookupName<DictValueSet>(const CString& name) const;


bool CDataDict::LookupName(const CString& csName, int* iLevel, int* iRecord, int* iItem, int* iVSet) const
{
    *iLevel = *iRecord = *iItem = *iVSet = NONE;

    const auto& name_map_pair_lookup = m_mapNames.find(csName);

    if( name_map_pair_lookup == m_mapNames.cend() )
        return false;

    const CDictName& name = name_map_pair_lookup->second;

    *iLevel = name.m_iLevel;
    *iRecord = name.m_iRec;
    *iItem = name.m_iItem;
    *iVSet = name.m_iVSet;

    return true;
}

bool CDataDict::LookupName(const std::wstring& name, int* iLevel, int* iRecord, int* iItem, int* iVSet) const
{
    return LookupName(WS2CS(name), iLevel, iRecord, iItem, iVSet);
}


const DictNamedBase* CDataDict::LookupName(const CString& name) const
{
    const DictLevel* dict_level;
    const CDictRecord* dict_record;
    const CDictItem* dict_item;
    const DictValueSet* dict_value_set;

    if( LookupName(name, &dict_level, &dict_record, &dict_item, &dict_value_set) )
    {
        return ( dict_level != nullptr )  ? (const DictNamedBase*)dict_level :
               ( dict_record != nullptr ) ? (const DictNamedBase*)dict_record :
               ( dict_item != nullptr )   ? (const DictNamedBase*)dict_item :
                                            (const DictNamedBase*)dict_value_set;
    }

    return nullptr;
}

bool CDataDict::IsNameUnique(const CString& name, int iLevel /*=NONE*/, int iRec /*=NONE*/, int iItem /*=NONE*/, int iVSet /*=NONE*/) const
{
    ASSERT(SO::IsUpper(name));

    int iL, iR, iI, iVS;

    // is the name in use for a different dictionary entity?
    if( LookupName(name, &iL, &iR, &iI, &iVS) )
    {
        if( iL != iLevel || iR != iRec || iI != iItem || iVS != iVSet )
            return false;
    }

    // also check against relation names
    for( const auto& dict_relation : m_dictRelations )
    {
        if( dict_relation.GetName().CompareNoCase(name) == 0 )
            return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetUniqueName
//
/////////////////////////////////////////////////////////////////////////////

CString CDataDict::GetUniqueName(const CString& base_name,
                                 int iLevelNum /*=NONE*/,
                                 int iRecordNum /*=NONE*/,
                                 int iItemNum /*=NONE*/,
                                 int iVSetNum /*=NONE*/,
                                 const std::set<CString>* additional_names_in_use/* = nullptr*/) const
{
    // names should come here already as valid CSPro names
    ASSERT(CIMSAString::MakeName(base_name) == base_name);

    return WS2CS(CIMSAString::CreateUnreservedName(base_name,
            [&](const std::wstring& name_candidate)
            {
                if( additional_names_in_use != nullptr && additional_names_in_use->find(WS2CS(name_candidate)) != additional_names_in_use->cend() )
                    return false;

                return IsNameUnique(WS2CS(name_candidate), iLevelNum, iRecordNum, iItemNum, iVSetNum);
            }));
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::Find
//
/////////////////////////////////////////////////////////////////////////////

bool CDataDict::Find(bool bNext, bool bCaseSensitive, const CString& csFindText,
                     int& iLevel, int& iRec, int& iItem, int& iVSet, int& iValue)
{
    // Intermediate variables
    int iL, iR, iI, iVS;
    CString csLabel;
    DictLevel* pLevel = NULL;
    CDictRecord* pRec = NULL;
    CDictItem* pItem = NULL;

    // prep so that we can skip to the correct starting position
    bool bCheckLevel = true;
    bool bCheckRec   = true;
    bool bCheckItem  = true;
    bool bCheckVSet  = true;
    if (bNext) {
        bCheckLevel = (iRec == NONE);
        bCheckRec   = (iItem == NONE);
        bCheckItem  = (iVSet == NONE);
        bCheckVSet  = (iValue == NONE);
    }
    else {
        bCheckLevel = !(iRec == NONE);
        bCheckRec   = !(iItem == NONE);
        bCheckItem  = !(iVSet == NONE);
        bCheckVSet  = !(iValue == NONE);
    }

    if (bNext) {
        // Check dictionary label and name
        if (iLevel == NONE) {
            csLabel = GetLabel();
            if (!bCaseSensitive) {
                csLabel.MakeUpper();
            }
            if (csLabel.Find(csFindText) != NONE || GetName().Find(csFindText) != NONE) {
                if (iLevel != NONE || iRec != NONE || iItem != NONE || iVSet != NONE || iValue != NONE) {
                    iLevel = NONE;
                    iRec = NONE;
                    iItem = NONE;
                    iVSet = NONE;
                    iValue = NONE;
                }
                return true;
            }
        }

        // Check level labels and names
        for (iL = std::max(iLevel,0) ; iL < (int)GetNumLevels() ; iL++) {
            pLevel = &GetLevel(iL);
            if (bCheckLevel) {
                csLabel = pLevel->GetLabel();
                if (!bCaseSensitive) {
                    csLabel.MakeUpper();
                }
                if (csLabel.Find(csFindText) != NONE || pLevel->GetName().Find(csFindText) != NONE) {
                    if (iLevel != iL || iRec != NONE || iItem != NONE || iVSet != NONE || iValue != NONE) {
                        iLevel = iL;
                        iRec = NONE;
                        iItem = NONE;
                        iVSet = NONE;
                        iValue = NONE;
                        return true;
                    }
                }
            }
            bCheckLevel = true;

            // Check id item labels and names
            if (iRec < 0) {
                pRec = pLevel->GetIdItemsRec();
                for (iI = std::max(iItem,0) ; iI < pRec->GetNumItems() ; iI++)     {
                    pItem = pRec->GetItem(iI);
                    if (bCheckItem) {
                        csLabel = pItem->GetLabel();
                        if (!bCaseSensitive) {
                            csLabel.MakeUpper();
                        }
                        if (csLabel.Find(csFindText) != -1 || pItem->GetName().Find(csFindText) != -1) {
                            if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != NONE || iValue != NONE) {
                                iLevel = iL;
                                iRec = COMMON;
                                iItem = iI;
                                iVSet = NONE;
                                iValue = NONE;
                                return true;
                            }
                        }
                    }
                    bCheckItem = true;

                    // Check id item value set labels and names
                    for (iVS = std::max(iVSet,0) ; iVS < (int)pItem->GetNumValueSets() ; iVS++) {
                        const DictValueSet& dict_value_set = pItem->GetValueSet(iVS);
                        if (bCheckVSet) {
                            csLabel = dict_value_set.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE || dict_value_set.GetName().Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != iVS || iValue != NONE) {
                                    iLevel = iL;
                                    iRec=COMMON;
                                    iItem=iI;
                                    iVSet = iVS;
                                    iValue = NONE;
                                    return true;
                                }
                            }
                        }
                        bCheckVSet = true;

                        // Check id item value set value labels
                        for (int iV = std::max(iValue,0) ; iV < (int)dict_value_set.GetNumValues() ; iV++) {
                            const auto& dict_value = dict_value_set.GetValue(iV);
                            csLabel = dict_value.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != iVS || iValue != iV) {
                                    iLevel = iL;
                                    iRec = COMMON;
                                    iItem = iI;
                                    iVSet = iVS;
                                    iValue = iV;
                                    return true;
                                }
                            }
                            iValue = 0;
                        }
                        iVSet = 0;
                    }
                    iItem = 0;
                }
                iRec = 0;
            }

            // check the record labels and names
            for (iR = std::max(iRec,0) ; iR < pLevel->GetNumRecords() ; iR++) {
                pRec = pLevel->GetRecord(iR);
                if (bCheckRec) {
                    csLabel = pRec->GetLabel();
                    if (!bCaseSensitive) {
                        csLabel.MakeUpper();
                    }
                    if (csLabel.Find(csFindText) != NONE || pRec->GetName().Find(csFindText) != NONE) {
                        if (iLevel != iL || iRec != iR || iItem != NONE || iVSet != NONE || iValue != NONE) {
                            iLevel = iL;
                            iRec = iR;
                            iItem = NONE;
                            iVSet = NONE;
                            iValue = NONE;
                            return true;
                        }
                    }
                }
                bCheckRec = true;

                // Check the record item labels and names
                for (iI = std::max(iItem,0) ; iI < pRec->GetNumItems(); iI++){
                    pItem = pRec->GetItem(iI);
                    if (bCheckItem) {
                        csLabel = pItem->GetLabel();
                        if (!bCaseSensitive) {
                            csLabel.MakeUpper();
                        }
                        if (csLabel.Find(csFindText) != NONE || pItem->GetName().Find(csFindText) != NONE) {
                            if (iLevel != iL || iRec != iR || iItem != iI || iVSet != NONE || iValue != NONE) {
                                iLevel = iL;
                                iRec = iR;
                                iItem = iI;
                                iVSet = NONE;
                                iValue = NONE;
                                return true;
                            }
                        }
                    }
                    bCheckItem = true;

                    // Check the record item value set labels and names
                    for (iVS = std::max(iVSet,0) ; iVS < (int)pItem->GetNumValueSets() ; iVS++) {
                        const DictValueSet& dict_value_set = pItem->GetValueSet(iVS);
                        if (bCheckVSet) {
                            csLabel = dict_value_set.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE || dict_value_set.GetName().Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != iR || iItem != iI || iVSet != iVS || iValue != NONE) {
                                    iLevel = iL;
                                    iRec = iR;
                                    iItem = iI;
                                    iVSet = iVS;
                                    iValue = NONE;
                                    return true;
                                }
                            }
                        }
                        bCheckVSet = true;

                        // check record item value set value labels
                       for (int iV = std::max(iValue,0) ; iV < (int)dict_value_set.GetNumValues() ; iV++) {
                            const auto& dict_value = dict_value_set.GetValue(iV);
                            csLabel = dict_value.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != iR || iItem != iI || iVSet != iVS || iValue != iV) {
                                    iLevel = iL;
                                    iRec = iR;
                                    iItem = iI;
                                    iVSet = iVS;
                                    iValue = iV;
                                    return true;
                                }
                            }
                        }
                        iValue = 0;
                    }
                    iVSet = 0;
                }
                iItem = 0;
            }
            iRec = 0;
        }
    }
    else {      // Find Prev
        // Check level labels and names
        int iLastLevel = (int)GetNumLevels() - 1;
        for (iL = std::min(iLevel,iLastLevel) ; iL >= 0 ; iL--) {
            // check the record labels and names
            pLevel = &GetLevel(iL);
            int iLastRec = pLevel->GetNumRecords() - 1;
            for (iR = std::min(iRec,iLastRec) ; iR >= 0 ; iR--) {
                // Check the record item labels and names
                pRec = pLevel->GetRecord(iR);
                int iLastItem = pRec->GetNumItems() - 1;
                for (iI = std::min(iItem,iLastItem) ; iI >= 0 ; iI--){
                    // Check the record item value set labels and names
                    pItem = pRec->GetItem(iI);
                    int iLastVSet = (int)pItem->GetNumValueSets() - 1;
                    for (iVS = std::min(iVSet,iLastVSet) ; iVS >= 0 ; iVS--) {
                        // check record item value set value labels
                        const DictValueSet& dict_value_set = pItem->GetValueSet(iVS);
                        int iLastValue = (int)dict_value_set.GetNumValues() - 1;
                        for (int iV = std::min(iValue,iLastValue) ; iV >= 0 ; iV--) {
                            const auto& dict_value = dict_value_set.GetValue(iV);
                            csLabel = dict_value.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != iR || iItem != iI || iVSet != iVS || iValue != iV) {
                                    iLevel = iL;
                                    iRec = iR;
                                    iItem = iI;
                                    iVSet = iVS;
                                    iValue = iV;
                                    return true;
                                }
                            }
                        }
                        if (bCheckVSet) {
                            csLabel = dict_value_set.GetLabel();
                            if (!bCaseSensitive) {
                                csLabel.MakeUpper();
                            }
                            if (csLabel.Find(csFindText) != NONE || dict_value_set.GetName().Find(csFindText) != NONE) {
                                if (iLevel != iL || iRec != iR || iItem != iI || iVSet != iVS || iValue != NONE) {
                                    iLevel = iL;
                                    iRec = iR;
                                    iItem = iI;
                                    iVSet = iVS;
                                    iValue = NONE;
                                    return true;
                                }
                            }
                        }
                        bCheckVSet = true;
                        iValue = INT_MAX;
                    }
                    if (bCheckItem) {
                        csLabel = pItem->GetLabel();
                        if (!bCaseSensitive) {
                            csLabel.MakeUpper();
                        }
                        if (csLabel.Find(csFindText) != NONE || pItem->GetName().Find(csFindText) != NONE) {
                            if (iLevel != iL || iRec != iR || iItem != iI || iVSet != NONE || iValue != NONE) {
                                iLevel = iL;
                                iRec = iR;
                                iItem = iI;
                                iVSet = NONE;
                                iValue = NONE;
                                return true;
                            }
                        }
                    }
                    bCheckItem = true;
                    iVSet = INT_MAX;
                }
                if (bCheckRec) {
                    csLabel = pRec->GetLabel();
                    if (!bCaseSensitive) {
                        csLabel.MakeUpper();
                    }
                    if (csLabel.Find(csFindText) != NONE || pRec->GetName().Find(csFindText) != NONE) {
                        if (iLevel != iL || iRec != iR || iItem != NONE || iVSet != NONE || iValue != NONE) {
                            iLevel = iL;
                            iRec = iR;
                            iItem = NONE;
                            iVSet = NONE;
                            iValue = NONE;
                            return true;
                        }
                    }
                }
                bCheckRec = true;
                iItem = INT_MAX;
            }

            pRec = pLevel->GetIdItemsRec();
            int iLastItem = pRec->GetNumItems() - 1;
            for (iI = std::min(iItem,iLastItem) ; iI >= 0 ; iI--) {
                // Check id item value set labels and names
                pItem = pRec->GetItem(iI);
                int iLastVSet = (int)pItem->GetNumValueSets() - 1;
                for (iVS = std::min(iVSet,iLastVSet) ; iVS >= 0 ; iVS--) {
                    // Check id item value set value labels
                    const DictValueSet& dict_value_set = pItem->GetValueSet(iVS);
                    int iLastValue = (int)dict_value_set.GetNumValues() - 1;
                    for (int iV = std::min(iValue,iLastValue) ; iV >= 0 ; iV--) {
                        const auto& dict_value = dict_value_set.GetValue(iV);
                        csLabel = dict_value.GetLabel();
                        if (!bCaseSensitive) {
                            csLabel.MakeUpper();
                        }
                        if (csLabel.Find(csFindText) != NONE) {
                            if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != iVS || iValue != iV) {
                                iLevel = iL;
                                iRec = COMMON;
                                iItem = iI;
                                iVSet = iVS;
                                iValue = iV;
                                return true;
                            }
                        }
                    }
                    if (bCheckVSet) {
                        csLabel = dict_value_set.GetLabel();
                        if (!bCaseSensitive) {
                            csLabel.MakeUpper();
                        }
                        if (csLabel.Find(csFindText) != NONE || dict_value_set.GetName().Find(csFindText) != NONE) {
                            if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != iVS || iValue != NONE) {
                                iLevel = iL;
                                iRec=COMMON;
                                iItem=iI;
                                iVSet = iVS;
                                iValue = NONE;
                                return true;
                            }
                        }
                    }
                    bCheckVSet = true;
                    iValue = INT_MAX;
                }
                if (bCheckItem) {
                    pItem = pRec->GetItem(iI);
                    csLabel = pItem->GetLabel();
                    if (!bCaseSensitive) {
                        csLabel.MakeUpper();
                    }
                    if (csLabel.Find(csFindText) != -1 || pItem->GetName().Find(csFindText) != -1) {
                        if (iLevel != iL || iRec != COMMON || iItem != iI || iVSet != NONE || iValue != NONE) {
                            iLevel = iL;
                            iRec = COMMON;
                            iItem = iI;
                            iVSet = NONE;
                            iValue = NONE;
                            return true;
                        }
                    }
                }
                bCheckItem = true;
                iVSet = INT_MAX;
            }
            iItem = INT_MAX;
            // Check level label and name
            if (bCheckLevel) {
                csLabel = pLevel->GetLabel();
                if (!bCaseSensitive) {
                    csLabel.MakeUpper();
                }
                if (csLabel.Find(csFindText) != NONE || pLevel->GetName().Find(csFindText) != NONE) {
                    if (iLevel != iL || iRec != NONE || iItem != NONE || iVSet != NONE || iValue != NONE) {
                        iLevel = iL;
                        iRec = NONE;
                        iItem = NONE;
                        iVSet = NONE;
                        iValue = NONE;
                        return true;
                    }
                }
            }
            bCheckLevel = true;
            iRec = INT_MAX;
        }
        // Check dictionary label and name
        if (iLevel == NONE) {
            csLabel = GetLabel();
            if (!bCaseSensitive) {
                csLabel.MakeUpper();
            }
            if (csLabel.Find(csFindText) != NONE || GetName().Find(csFindText) != NONE) {
                if (iLevel != NONE || iRec != NONE || iItem != NONE || iVSet != NONE || iValue != NONE) {
                    iLevel = NONE;
                    iRec = NONE;
                    iItem = NONE;
                    iVSet = NONE;
                    iValue = NONE;
                }
                return true;
            }
        }
    }

    return false;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         CDataDict::GetStructureMd5
//
/////////////////////////////////////////////////////////////////////////////

namespace
{
    class DictionaryIteratorForStructureMd5 : public DictionaryIterator::Iterator
    {
    public:
        DictionaryIteratorForStructureMd5()
            :   m_structureData(0)
        {
        }

        std::wstring GetMd5() const
        {
            return PortableFunctions::BinaryMd5(m_structureData);
        }

    protected:
        void ProcessDictionary(CDataDict& dictionary) override
        {
            Process(dictionary.GetName());
            Process(dictionary.GetRecTypeStart());
            Process(dictionary.GetRecTypeLen());
        }

        void ProcessLevel(DictLevel& dict_level) override
        {
            Process(dict_level.GetName());
        }

        void ProcessRecord(CDictRecord& dict_record) override
        {
            Process(dict_record.GetName());
            Process(dict_record.GetRecTypeVal());
            Process(dict_record.GetRequired());
            Process(dict_record.GetMaxRecs());
        }

        void ProcessItem(CDictItem& dict_item) override
        {
            Process(dict_item.GetName());
            Process(dict_item.GetStart());
            Process(dict_item.GetLen());
            Process(dict_item.GetContentType());
            Process(dict_item.GetItemType());
            Process(dict_item.GetOccurs());
            Process(dict_item.GetDecimal());
            Process(dict_item.GetDecChar());
            Process(dict_item.GetZeroFill());
        }

    private:
        void Process(const void* data, size_t data_length)
        {
            size_t current_size = m_structureData.size();
            m_structureData.resize(current_size + data_length);
            memcpy(m_structureData.data() + current_size, data, data_length);
        }

        void Process(const CString& text)
        {
            Process(text.GetString(), text.GetLength() * sizeof(TCHAR));
        }

        template<typename T>
        void Process(const T& value)
        {
            Process(&value, sizeof(T));
        }

    private:
        std::vector<std::byte> m_structureData;
    };
}

std::wstring CDataDict::GetStructureMd5() const
{
    DictionaryIteratorForStructureMd5 iterator;
    iterator.Iterate(const_cast<CDataDict&>(*this));
    return iterator.GetMd5();
}


size_t CDataDict::GetIdStructureHashForKeyIndex(bool hash_name, bool hash_start) const
{
    // calculate a hash that describes the IDs
    size_t id_structure_hash = m_dictLevels.size();

    for( const DictLevel& dict_level : m_dictLevels )
    {
        const CDictRecord& dict_id_record = *dict_level.GetIdItemsRec();

        for( int i = 0; i < dict_id_record.GetNumItems(); ++i )
        {
            const CDictItem& dict_id_item = *dict_id_record.GetItem(i);

            ASSERT(!DictionaryRules::CanHaveSubitems(dict_id_record, dict_id_item) &&
                   !DictionaryRules::CanItemHaveMultipleOccurrences(dict_id_record) &&
                   !DictionaryRules::CanHaveDecimals(dict_id_record, dict_id_item.GetContentType()));

            if( hash_name )
                Hash::Combine(id_structure_hash, wstring_view(dict_id_item.GetName()));

            if( hash_start )
                Hash::Combine(id_structure_hash, dict_id_item.GetStart());

            Hash::Combine(id_structure_hash, dict_id_item.GetLen());
            Hash::Combine(id_structure_hash, dict_id_item.GetContentType());
            Hash::Combine(id_structure_hash, dict_id_item.GetZeroFill());
        }
    }

    return id_structure_hash;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         CDataDict::GetFileModifiedTime
//
/////////////////////////////////////////////////////////////////////////////

int64_t CDataDict::GetFileModifiedTime() const
{
    if( m_serializedFileModifiedTime.has_value() )
        return *m_serializedFileModifiedTime;

    else
        return PortableFunctions::FileModifiedTime(m_filename);
}


const CDictRecord* CDataDict::FindRecord(wstring_view record_name) const
{
    for( const DictLevel& dict_level : m_dictLevels )
    {
        for( int r = -1; r < dict_level.GetNumRecords(); ++r )
        {
            const CDictRecord* record = ( r == -1 ) ? dict_level.GetIdItemsRec() : dict_level.GetRecord(r);

            if( SO::EqualsNoCase(record_name, record->GetName()) )
                return record;
        }
    }

    return nullptr;
}


const CDictItem* CDataDict::FindItem(wstring_view item_name) const
{
    for( const DictLevel& dict_level : m_dictLevels )
    {
        for( int iRecord = -1; iRecord < dict_level.GetNumRecords(); ++iRecord )
        {
            const CDictRecord* pRec = ( iRecord == -1 ) ? dict_level.GetIdItemsRec() : dict_level.GetRecord(iRecord);
            const CDictItem* pItem = pRec->FindItem(item_name);

            if( pItem != nullptr )
                return pItem;
        }
    }

    return nullptr;
}



/////////////////////////////////////////////////////////////////////////////
//
//                             CDictName::CDictName
//
/////////////////////////////////////////////////////////////////////////////

CDictName::CDictName(int iLevel, int iRec, int iItem, int iVSet) :
        m_iLevel(iLevel), m_iRec(iRec), m_iItem(iItem), m_iVSet(iVSet) {
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictName::operator=
//
/////////////////////////////////////////////////////////////////////////////

void CDictName::operator= (const CDictName& n) {
    m_iLevel = n.m_iLevel;
    m_iRec = n.m_iRec;
    m_iItem = n.m_iItem;
    m_iVSet = n.m_iVSet;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDataDict::UpdatePointers
//
/////////////////////////////////////////////////////////////////////////////

void CDataDict::UpdatePointers()
{
    for( size_t level_number = 0; level_number < m_dictLevels.size(); ++level_number )
    {
        DictLevel& dict_level = m_dictLevels[level_number];
        ASSERT(level_number == dict_level.GetLevelNumber());

        for( int r = COMMON; r < dict_level.GetNumRecords(); ++r )
        {
            CDictRecord* dict_record = dict_level.GetRecord(r);
            dict_record->SetDataDict(this);
            dict_record->SetLevel(&dict_level);
            dict_record->SetSonNumber(r);

            if( r == COMMON )
            {
                dict_record->SetName(FormatText(_T("_IDS%d"), (int)level_number));

                static_assert(COMMON == -2);
                ++r;
            }

            CDictItem* parent_dict_item = nullptr;

            for( int i = 0; i < dict_record->GetNumItems(); ++i )
            {
                CDictItem* dict_item = dict_record->GetItem(i);
                dict_item->SetSonNumber(i);
                dict_item->SetLevel(&dict_level);
                dict_item->SetRecord(dict_record);

                if( dict_item->GetItemType() == ItemType::Item )
                {
                    parent_dict_item = dict_item;
                    dict_item->SetParentItem(nullptr);
                }

                else
                {
                    ASSERT(parent_dict_item != nullptr);
                    dict_item->SetParentItem(parent_dict_item);
                }
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// CDataDict -- languages
//
/////////////////////////////////////////////////////////////////////////////

std::optional<size_t> CDataDict::IsLanguageDefined(wstring_view language_name) const
{
    const auto& lookup = std::find_if(m_languages.cbegin(), m_languages.cend(),
                                      [&](const auto& language) { return SO::EqualsNoCase(language.GetName(), language_name); });

    if( lookup != m_languages.cend() )
        return std::distance(m_languages.cbegin(), lookup);

    return std::nullopt;
}


void CDataDict::AddLanguage(Language language)
{
    if( !IsLanguageDefined(language.GetName()).has_value() )
        m_languages.emplace_back(std::move(language));
}


void CDataDict::ModifyLanguage(size_t language_index, Language language)
{
    ASSERT(language_index < m_languages.size());
    m_languages[language_index] = std::move(language);
}


void CDataDict::DeleteLanguage(size_t language_index)
{
    ASSERT(m_languages.size() > 1 && language_index < m_languages.size());

    DictionaryIterator::ForeachLabelSet(*this,
        [language_index](LabelSet& label_set)
        {
            label_set.DeleteLabel(language_index);
        });

    m_languages.erase(m_languages.begin() + language_index);
}


void CDataDict::SetCurrentLanguage(size_t language_index) const
{
    ASSERT(language_index < m_languages.size());

    DictionaryIterator::ForeachLabelSet(*this,
        [language_index](const LabelSet& label_set)
        {
            label_set.SetCurrentLanguage(language_index);
        });
}


std::vector<const CDictItem*> CDataDict::GetIdItems(std::vector<int>* pAcumItemsByLevelIdx/* = nullptr*/, std::vector<const CDictRecord*>* paIdRecs/* = nullptr*/) const
{
    std::vector<const CDictItem*> id_items;

    for( size_t level_number = 0; level_number < m_dictLevels.size(); ++level_number )
    {
        const DictLevel& dict_level = m_dictLevels[level_number];
        const CDictRecord* pIdRecord = dict_level.GetIdItemsRec();

        //FABN Jan 2005
        if( paIdRecs != nullptr )
            paIdRecs->emplace_back(pIdRecord);

        int iNumIdItems = pIdRecord->GetNumItems();

        for(int iIdItemIdx=0; iIdItemIdx<iNumIdItems; iIdItemIdx++){
            const CDictItem* pIdItem = pIdRecord->GetItem(iIdItemIdx);
            id_items.emplace_back(pIdItem);
        }

        if( pAcumItemsByLevelIdx != nullptr )
            pAcumItemsByLevelIdx->emplace_back(level_number==0 ? iNumIdItems : pAcumItemsByLevelIdx->at(level_number - 1) + iNumIdItems );
    }

    return id_items;
}


std::vector<std::vector<const CDictItem*>> CDataDict::GetIdItemsByLevel() const
{
    std::vector<const CDictItem*> id_items = GetIdItems();

    std::vector<std::vector<const CDictItem*>> id_items_by_level;

    for( size_t i = 0; i < id_items.size(); ++i )
    {
        size_t level_number = id_items[i]->GetLevel()->GetLevelNumber();

        if( level_number >= id_items_by_level.size() )
            id_items_by_level.emplace_back();

        id_items_by_level[level_number].emplace_back(id_items[i]);
    }

    return id_items_by_level;
}

unsigned CDataDict::GetKeyLength() const
{
    // ENGINECR_TODO(GetKeyLength) if this function gets called a lot, save the value in EngineDictionary or CaseMetadata
    const CDictRecord* id_items_rec = GetLevel(0).GetIdItemsRec();
    unsigned key_length = 0;

    for( int i = 0; i < id_items_rec->GetNumItems(); ++i )
        key_length += id_items_rec->GetItem(i)->GetLen();

    return key_length;
}


std::vector<CString> CDataDict::GetUniqueNames(const TCHAR* prefix, int iNumDigits, int iNumNames) const
{
    std::vector<CString> unique_names;

    for( int i = 0; i < iNumNames; ++i )
    {
        CString csName = FormatText(_T("%s%0*d"), prefix, iNumDigits, i + 1);
        csName = CIMSAString::MakeName(csName);
        csName = GetUniqueName(csName);
        unique_names.emplace_back(std::move(csName));
    }

    return unique_names;
}


int CDataDict::GetParentItemNum(int iLevel, int iRec, int iItem) const
{
    int iRetVal = iItem;
    const CDictRecord* pRec= GetLevel(iLevel).GetRecord(iRec);
    ASSERT(pRec);
    const CDictItem* pItem = pRec->GetItem(iItem);
    if (pItem->GetItemType() == ItemType::Subitem) {
        bool bDone = false;
        while (!bDone) {
            if (--iItem < 0) {
                iRetVal = NONE;
                bDone = true;
            }
            else {
                pItem = pRec->GetItem(iItem);
                iRetVal = iItem;
                bDone = (pItem->GetItemType() == ItemType::Item);
            }
        }
    }
    return iRetVal;
}


const CDictItem* CDataDict::GetParentItem(int iLevel, int iRec, int iItem) const
{
    ASSERT(GetParentItemNum(iLevel, iRec, iItem) != NONE);
    return GetLevel(iLevel).GetRecord(iRec)->GetItem(GetParentItemNum(iLevel, iRec, iItem));
}



// --------------------------------------------------------------------------
// levels
// --------------------------------------------------------------------------

namespace
{
    inline void ResetLevelNumbers(std::vector<DictLevel>& dict_levels, size_t level_number)
    {
        for( ; level_number < dict_levels.size(); ++level_number )
            dict_levels[level_number].SetLevelNumber(level_number);
    }
}

void CDataDict::AddLevel(DictLevel dict_level)
{
    dict_level.SetLevelNumber(m_dictLevels.size());

    m_dictLevels.emplace_back(std::move(dict_level));
}

void CDataDict::InsertLevel(size_t index, DictLevel dict_level)
{
    ASSERT(index <= m_dictLevels.size());
    m_dictLevels.insert(m_dictLevels.begin() + index, std::move(dict_level));

    ResetLevelNumbers(m_dictLevels, index);
}

void CDataDict::RemoveLevel(size_t index)
{
    ASSERT(index < m_dictLevels.size());
    m_dictLevels.erase(m_dictLevels.begin() + index);

    ResetLevelNumbers(m_dictLevels, index);
}



// --------------------------------------------------------------------------
// relations
// --------------------------------------------------------------------------

void CDataDict::AddRelation(DictRelation dict_relation)
{
    m_dictRelations.emplace_back(std::move(dict_relation));
}

void CDataDict::RemoveRelation(size_t index)
{
    ASSERT(index < m_dictRelations.size());
    m_dictRelations.erase(m_dictRelations.begin() + index);
}

void CDataDict::SetRelations(std::vector<DictRelation> dict_relations)
{
    m_dictRelations = std::move(dict_relations);
}



// --------------------------------------------------------------------------
// linked value set management
// --------------------------------------------------------------------------

size_t CDataDict::CountValueSetLinks(const DictValueSet& dict_value_set) const
{
    size_t links = 0;

    if( dict_value_set.IsLinkedValueSet() )
    {
        DictionaryIterator::Foreach<DictValueSet>(*this,
            [&](const DictValueSet& this_dict_value_set)
        {
            if( this_dict_value_set.IsLinkedValueSet() && this_dict_value_set.GetLinkedValueSetCode() == dict_value_set.GetLinkedValueSetCode() )
            {
                ++links;
            }
        });
    }

    return links;
}


void CDataDict::SyncLinkedValueSets(std::variant<SyncLinkedValueSetsAction, DictValueSet*> action_or_updated_dict_value_set/* = SyncLinkedValueSetsAction::UpdateValuesFromLinks*/,
                                    const std::vector<CString>* value_set_names_added_on_paste/* = nullptr*/)
{
    // - if action_or_updated_dict_value_set is not null, then all value sets linked to it are updated with the values in action_or_updated_dict_value_set
    // - if the action is UpdateValuesFromLinks, all value sets are updated based on the values in the value set with the most values
    // - if the action is OnPaste, the value sets are updated similarly to UpdateValuesFromLinks except that source value set (with the most values)
    //      will not be one of the value sets named in the value_set_names_added_on_paste vector

    bool using_action = std::holds_alternative<SyncLinkedValueSetsAction>(action_or_updated_dict_value_set);
    ASSERT(using_action || ( std::get<DictValueSet*>(action_or_updated_dict_value_set) != nullptr &&
                             std::get<DictValueSet*>(action_or_updated_dict_value_set)->IsLinkedValueSet() ));

    // first pass: figure out the linkages
    std::map<std::wstring, std::vector<DictValueSet*>> linked_value_sets;

    DictionaryIterator::Foreach<DictValueSet>(*this,
        [&](DictValueSet& dict_value_set)
        {
            if( dict_value_set.IsLinkedValueSet() &&
                ( using_action || dict_value_set.GetLinkedValueSetCode() == std::get<DictValueSet*>(action_or_updated_dict_value_set)->GetLinkedValueSetCode() ) )
            {
                linked_value_sets[dict_value_set.GetLinkedValueSetCode()].emplace_back(&dict_value_set);
            }
        });


    // second pass: sync the values for valid linkages and remove the linkages if they no longer exist
    bool on_paste = ( using_action && std::get<SyncLinkedValueSetsAction>(action_or_updated_dict_value_set) == SyncLinkedValueSetsAction::OnPaste );
    ASSERT(on_paste == ( value_set_names_added_on_paste != nullptr ));

    for( auto& [serialized_link, value_sets] : linked_value_sets )
    {
        // unlink the value set if is no longer connected to any other value sets
        if( value_sets.size() == 1 )
        {
            value_sets.front()->UnlinkValueSet();
            continue;
        }

        // otherwise determine the value set that contains the values to copy; generally this
        // will be the value set with the most values, but it could also be the value set
        // that is currently being edited
        DictValueSet* primary_dict_value_set;

        if( !using_action )
        {
            ASSERT(std::find(value_sets.cbegin(), value_sets.cend(), std::get<DictValueSet*>(action_or_updated_dict_value_set)) != value_sets.cend());
            primary_dict_value_set = std::get<DictValueSet*>(action_or_updated_dict_value_set);
        }

        else
        {
            primary_dict_value_set = nullptr;

            // on paste, the updated values in a value set (not coming from the paste) should be prioritzed
            for( bool first_pass = true; ; first_pass = false )
            {
                for( const auto& value_set : value_sets )
                {
                    // on the first pass, ignore value sets from the paste
                    if( on_paste && first_pass && std::find(value_set_names_added_on_paste->cbegin(), value_set_names_added_on_paste->cend(),
                                                            value_set->GetName()) != value_set_names_added_on_paste->cend() )
                    {
                        continue;
                    }

                    if( primary_dict_value_set == nullptr || value_set->GetNumValues() > primary_dict_value_set->GetNumValues() )
                        primary_dict_value_set = value_set;
                }

                if( primary_dict_value_set != nullptr )
                    break;
            }

            ASSERT(primary_dict_value_set != nullptr);
        }

        // set the value set links and copy the values from the primary linked value set
        for( auto& value_set : value_sets )
        {
            if( value_set != primary_dict_value_set )
            {
                value_set->LinkValueSet(*primary_dict_value_set);
                value_set->SetValues(primary_dict_value_set->GetValues());
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//  returns # occurrences this item can have (usually 1)

int GetSuperItemNum(const CDictRecord* pRec, const CDictItem* pItem, int iItem);

UINT GetDictOccs(const CDictRecord* pRec, const CDictItem* pItem, int iItem)
{
    UINT uOccs;

    uOccs = pItem->GetOccurs();
    if (uOccs > 1) {
        return uOccs;
    }

    if (pItem->GetItemType() == ItemType::Subitem) {
        int iSuper = GetSuperItemNum(pRec, pItem, iItem);
        const CDictItem* pSuper = pRec->GetItem(iSuper);
        uOccs = pSuper->GetOccurs();
        if (uOccs > 1) {
            return uOccs;
        }
    }

    return 1;
}

int GetSuperItemNum(const CDictRecord* pRec, const CDictItem* pItem, int iItem)
{
    int iRetVal = iItem;
    if (pItem->GetItemType() == ItemType::Subitem) {
        BOOL bDone = FALSE;
        while (!bDone) {
            if (--iItem < 0) {
                iRetVal = NONE;
                bDone = TRUE;
            }
            else  {
                pItem = pRec->GetItem(iItem);
                iRetVal = iItem;
                bDone = (pItem->GetItemType() == ItemType::Item);
            }
        }
    }
    return iRetVal;
}


/////////////////////////////////////////////////
// positions pItem to the occurring item
// normally this is the same as the incoming item
// however, if the incoming is a subitem of an item which occurs,
//   pItem will be changed to point to the super item

const CDictItem* GetDictOccItem(const CDictRecord* pRecord, const CDictItem* pItem, int /*iRec*/, int iItem)
{
    UINT uOccs;

    uOccs = pItem->GetOccurs();
    if (uOccs > 1) {
        return pItem;
    }

    if (pItem->GetItemType() == ItemType::Subitem) {
        int iSuper = GetSuperItemNum(pRecord, pItem, iItem);
        pItem = pRecord->GetItem(iSuper);
        uOccs = pItem->GetOccurs();
        ASSERT(uOccs > 1);
    }
    else {
        ASSERT(FALSE);
    }
    return pItem;
}


std::unique_ptr<CDataDict> CDataDict::InstantiateAndOpen(const NullTerminatedString filename, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    auto dictionary = std::make_unique<CDataDict>();
    dictionary->Open(filename, silent, std::move(message_logger));
    return dictionary;
}


void CDataDict::Open(const NullTerminatedString filename, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    auto json_reader = JsonSpecFile::CreateReader(filename, std::move(message_logger), [&]() { return ConvertPre80SpecFile(filename); });
    return Open(*json_reader, silent);
}


void CDataDict::Open(JsonSpecFile::Reader& json_reader, const bool silent)
{
    try
    {
        json_reader.CheckVersion();
        json_reader.CheckFileType(JK::dictionary);

        CreateFromJsonWorker(json_reader);

        m_filename = WS2CS(json_reader.GetFilename());
    }

    catch( const CSProException& exception )
    {
        json_reader.GetMessageLogger().RethrowException(json_reader.GetFilename().c_str(), exception);
    }

    // report any warnings
    json_reader.GetMessageLogger().DisplayWarnings(silent);
}


void CDataDict::OpenFromText(wstring_view text)
{
    if( !text.empty() && text.front() == JsonSpecFile::Pre80SpecFileStartCharacter )
    {
        // save the pre-JSON file to a temporary file and open it
        TemporaryFile temporary_file;
        FileIO::WriteText(temporary_file.GetPath(), text, true);
        ConvertPre80SpecFile(temporary_file.GetPath());
        Open(temporary_file.GetPath(), true);
    }

    else
    {
        auto json_reader = JsonSpecFile::CreateReader(_T(""), text);
        return Open(*json_reader, true);
    }
}


void CDataDict::Save(NullTerminatedString filename, bool continue_using_filename/* = true*/) const
{
    auto json_writer = JsonSpecFile::CreateWriter(filename, JK::dictionary);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    if( continue_using_filename )
        const_cast<CDataDict*>(this)->m_filename = CString(filename.c_str());
}


std::wstring CDataDict::GetJson(const bool spec_file_format/* = true*/) const
{
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( spec_file_format )
        JsonSpecFile::WriteHeading(*json_writer, JK::dictionary);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    return json_writer->GetString();
}


CDataDict CDataDict::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    CDataDict dictionary;
    dictionary.CreateFromJsonWorker(json_node);
    return dictionary;
}


void CDataDict::CreateFromJsonWorker(const JsonNode<wchar_t>& json_node)
{
    // some values are assumed to come in with the default values
    ASSERT(m_languages.size() == 1 && m_languages[0] == Language());
    ASSERT(!m_allowDataViewerModifications && !m_allowExport && m_cachedPasswordMinutes == 0);

    DictionarySerializerHelper dict_serializer_helper(*this);
    auto dict_serializer_helper_holder = json_node.GetSerializerHelper().Register(&dict_serializer_helper);

    // only parse DictBase after the languages have been parsed
    DictNamedBase::ParseJsonInput(json_node, false);

    if( json_node.Contains(JK::languages) )
    {
        m_languages = json_node.GetArray(JK::languages).GetVector<Language>();

        // add a default language if no languages were defind
        if( m_languages.empty() )
        {
            json_node.LogWarning(_T("A default language was added to '%s'"), GetName().GetString());
            m_languages.emplace_back();
        }
    }

    DictBase::ParseJsonInput(json_node);

    if( json_node.Contains(JK::security) )
        DeserializeSecurityOptions(json_node.Get(JK::security).Get<CString>(JK::settings), GetName(), m_allowDataViewerModifications, m_allowExport, m_cachedPasswordMinutes);

    m_readOptimization = json_node.GetOrDefault(JK::readOptimization, DictionaryDefaults::ReadOptimization);

    const auto& record_type_node = json_node.Get(JK::recordType);
    m_uRecTypeStart = record_type_node.Get<unsigned>(JK::start);
    m_uRecTypeLen = record_type_node.Get<unsigned>(JK::length);

    const auto& defaults_node = json_node.GetOrEmpty(JK::defaults);
    m_bDecChar = defaults_node.GetOrDefault(JK::decimalMark, DictionaryDefaults::DecChar);
    m_bZeroFill = defaults_node.GetOrDefault(JK::zeroFill, DictionaryDefaults::ZeroFill);

    m_bPosRelative = json_node.Get<bool>(JK::relativePositions);


    // levels
    m_dictLevels = json_node.GetArrayOrEmpty(JK::levels).GetVector<DictLevel>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A level was not added to '%s' due to errors: %s"), GetName().GetString(), exception.GetErrorMessage().c_str());
        });

    ResetLevelNumbers(m_dictLevels, 1);

    // if there are multiple levels, the start positions may need to be adjusted when using relative positioning, because
    // the start positions for a level won't reflect the spacing needed for the IDs on subsequent levels (which are only
    // processed after the start positions for lower levels have already been set)
    if( m_bPosRelative && m_dictLevels.size() > 1 )
        DictionaryValidator::AdjustStartPositions(*this);


    // relations
    m_dictRelations = json_node.GetArrayOrEmpty(JK::relations).GetVector<DictRelation>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A relation was not added to '%s' due to errors: %s"), GetName().GetString(), exception.GetErrorMessage().c_str());
        });

    m_enableBinaryItems = ( dict_serializer_helper.GetUsesBinaryItems() ||
                            json_node.GetOrDefault(_T("enableBinaryItems"), false) );

    // finalize the dictionary
    BuildNameList();
    UpdatePointers();
    SyncLinkedValueSets();
}


void CDataDict::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
{
    DictionarySerializerHelper dict_serializer_helper(*this);
    auto dict_serializer_helper_holder = json_writer.GetSerializerHelper().Register(&dict_serializer_helper);

    if( write_to_new_json_object )
        json_writer.BeginObject();

    // only write DictBase once the languages have been written
    DictNamedBase::WriteJson(json_writer, false);

    ASSERT(!m_languages.empty());

    if( json_writer.Verbose() || m_languages.size() > 1 || m_languages.front() != Language() )
        json_writer.Write(JK::languages, m_languages);

    DictBase::WriteJson(json_writer);

    json_writer.BeginObject(JK::security)
               .Write(JK::allowDataViewerModifications, m_allowDataViewerModifications)
               .Write(JK::allowExport, m_allowExport)
               .Write(JK::cachedPasswordMinutes, m_cachedPasswordMinutes)
               .Write(JK::settings, SerializeSecurityOptions(GetName(), m_allowDataViewerModifications, m_allowExport, m_cachedPasswordMinutes))
               .EndObject();

    json_writer.Write(JK::readOptimization, m_readOptimization);

    json_writer.WriteIfNot(_T("enableBinaryItems"), m_enableBinaryItems, false);

    json_writer.BeginObject(JK::recordType)
               .Write(JK::start, m_uRecTypeStart)
               .Write(JK::length, m_uRecTypeLen)
               .EndObject();

    json_writer.BeginObject(JK::defaults)
               .Write(JK::decimalMark, m_bDecChar)
               .Write(JK::zeroFill, m_bZeroFill)
               .EndObject();

    json_writer.Write(JK::relativePositions, m_bPosRelative);

    json_writer.Write(JK::levels, m_dictLevels);

    if( json_writer.Verbose() || !m_dictRelations.empty() )
        json_writer.Write(JK::relations, m_dictRelations);

    if( write_to_new_json_object )
        json_writer.EndObject();
}


void CDataDict::serialize(Serializer& ar)
{
    auto dict_serializer_helper_holder = ar.GetSerializerHelper().Register(std::make_shared<DictionarySerializerHelper>(*this));

    m_serializedFileModifiedTime = static_cast<int64_t>(ar.GetArchiveModifiedDate());

    DictNamedBase::serialize(ar);

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
        SetNote(ar.Read<CString>());

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_csError

    ar & m_uRecTypeStart;
    ar & m_uRecTypeLen;
    ar & m_bPosRelative;
    ar & m_bZeroFill;
    ar & m_bDecChar;

    ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumLevels

    ar & m_csOldName;
    ar & m_iSymbol;

    ar & m_languages;

    ar & m_allowDataViewerModifications
       & m_allowExport
       & m_cachedPasswordMinutes;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_2) )
        ar & m_readOptimization;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar & m_enableBinaryItems;

    ar & m_dictLevels;

    ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumRelations
    ar & m_dictRelations;

    if( ar.IsLoading() )
    {
        if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_002_1) )
            ResetLevelNumbers(m_dictLevels, 0);

        BuildNameList();
        UpdatePointers();
        SyncLinkedValueSets();
    }

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    // allow a way for developers to recover people's dictionaries from .pen files
    if( std::wstring(GetCommandLine()).find(_T("/extract")) != std::wstring::npos )
    {
        const std::wstring filename = PortableFunctions::PathAppendToPath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop),
                                                                          GetName() + FileExtensions::WithDot::Dictionary);
        Save(filename);
    }
#endif
}
