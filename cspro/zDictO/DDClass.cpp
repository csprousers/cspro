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
        m_allowDataManagerModifications(false),
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
        m_allowDataManagerModifications(rhs.m_allowDataManagerModifications),
        m_allowExport(rhs.m_allowExport),
        m_cachedPasswordMinutes(rhs.m_cachedPasswordMinutes),
        m_readOptimization(rhs.m_readOptimization),
        m_filePath(rhs.m_filePath),
        m_serializedFileModifiedTime(rhs.m_serializedFileModifiedTime),
        m_iSymbol(-1),
        m_pChangedObject(nullptr),
        m_enableBinaryItems(rhs.m_enableBinaryItems),
        m_syncableName(rhs.m_syncableName),
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
    SetAllowDataManagerModifications(dictionary.GetAllowDataManagerModifications());
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
    m_allowDataManagerModifications = dict.m_allowDataManagerModifications;
    m_allowExport = dict.m_allowExport;
    m_cachedPasswordMinutes = dict.m_cachedPasswordMinutes;
    m_readOptimization = dict.m_readOptimization;
    m_enableBinaryItems = dict.m_enableBinaryItems;
    m_syncableName = dict.m_syncableName;
    m_languages = dict.m_languages;
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
                ASSERT(dict_record->GetName().front() == '_');
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

void CDataDict::AddToNameList(const std::string& name, int iLevel /*=NONE*/,
                                                       int iRec /*=NONE*/,
                                                       int iItem /*=NONE*/,
                                                       int iVSet /*=NONE*/)
{
    if( !name.empty() )
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

    for( const std::string& alias : dict_element.GetAliases() )
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
bool CDataDict::LookupName(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record/* = nullptr*/,
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

template CLASS_DECL_ZDICTO bool CDataDict::LookupName<void>(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<DictLevel>(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<CDictRecord>(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<CDictItem>(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<DictValueSet>(const std::string& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;

template<typename T/* = void*/>
bool CDataDict::LookupName(const std::string& name, DictLevel** dict_level, CDictRecord** dict_record/* = nullptr*/,
                           CDictItem** dict_item/* = nullptr*/, DictValueSet** dict_value_set/* = nullptr*/)
{
    return const_cast<const CDataDict*>(this)->LookupName<T>(name, const_cast<const DictLevel**>(dict_level),
                                                                   const_cast<const CDictRecord**>(dict_record),
                                                                   const_cast<const CDictItem**>(dict_item),
                                                                   const_cast<const DictValueSet**>(dict_value_set));
}

template CLASS_DECL_ZDICTO bool CDataDict::LookupName<void>(const std::string& name, DictLevel** dict_level, CDictRecord** dict_record, CDictItem** dict_item, DictValueSet** dict_value_set);
template CLASS_DECL_ZDICTO bool CDataDict::LookupName<DictValueSet>(const std::string& name, DictLevel** dict_level, CDictRecord** dict_record, CDictItem** dict_item, DictValueSet** dict_value_set);


template<typename T>
const T* CDataDict::LookupName(const std::string& name) const
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

template CLASS_DECL_ZDICTO const DictLevel* CDataDict::LookupName<DictLevel>(const std::string& name) const;
template CLASS_DECL_ZDICTO const CDictRecord* CDataDict::LookupName<CDictRecord>(const std::string& name) const;
template CLASS_DECL_ZDICTO const CDictItem* CDataDict::LookupName<CDictItem>(const std::string& name) const;
template CLASS_DECL_ZDICTO const DictValueSet* CDataDict::LookupName<DictValueSet>(const std::string& name) const;


bool CDataDict::LookupName(const std::string& name, int* iLevel, int* iRecord, int* iItem, int* iVSet) const
{
    *iLevel = *iRecord = *iItem = *iVSet = NONE;

    const auto& name_map_pair_lookup = m_mapNames.find(name);

    if( name_map_pair_lookup == m_mapNames.cend() )
        return false;

    const CDictName& dict_name = name_map_pair_lookup->second;

    *iLevel = dict_name.m_iLevel;
    *iRecord = dict_name.m_iRec;
    *iItem = dict_name.m_iItem;
    *iVSet = dict_name.m_iVSet;

    return true;
}


const DictNamedBase* CDataDict::LookupName(const std::string& name) const
{
    const DictLevel* dict_level;
    const CDictRecord* dict_record;
    const CDictItem* dict_item;
    const DictValueSet* dict_value_set;

    if( LookupName(name, &dict_level, &dict_record, &dict_item, &dict_value_set) )
    {
        return ( dict_level != nullptr )  ? static_cast<const DictNamedBase*>(dict_level) :
               ( dict_record != nullptr ) ? static_cast<const DictNamedBase*>(dict_record) :
               ( dict_item != nullptr )   ? static_cast<const DictNamedBase*>(dict_item) :
                                            static_cast<const DictNamedBase*>(dict_value_set);
    }

    return nullptr;
}


bool CDataDict::IsNameUnique(const std::string& name, int iLevel /*=NONE*/, int iRec /*=NONE*/, int iItem /*=NONE*/, int iVSet /*=NONE*/) const
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
        if( SO::EqualsNoCase(dict_relation.GetName(), name) )
            return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetUniqueName
//
/////////////////////////////////////////////////////////////////////////////

std::string  CDataDict::GetUniqueName(const std::string & base_name,
                                      int iLevelNum /*=NONE*/,
                                      int iRecordNum /*=NONE*/,
                                      int iItemNum /*=NONE*/,
                                      int iVSetNum /*=NONE*/,
                                      const std::set<std::string>* const additional_names_in_use/* = nullptr*/) const
{
    // names should come here already as valid CSPro names
    ASSERT(CIMSAString::MakeName(base_name) == base_name);

    return CIMSAString::CreateUnreservedName(base_name,
            [&](const std::string& name_candidate)
            {
                if( additional_names_in_use != nullptr &&
                    additional_names_in_use->find(name_candidate) != additional_names_in_use->cend() )
                {
                    return false;
                }

                return IsNameUnique(name_candidate, iLevelNum, iRecordNum, iItemNum, iVSetNum);
            });
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict::Find
//
/////////////////////////////////////////////////////////////////////////////

bool CDataDict::Find(bool bNext, bool bCaseSensitive, const std::string& find_text,
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
            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || GetName().find(find_text) != std::string::npos ) {
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
                if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pLevel->GetName().find(find_text) != std::string::npos) {
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
                        if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != -1 || pItem->GetName().find(find_text) != std::string::npos) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || dict_value_set.GetName().find(find_text) != std::string::npos) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE) {
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
                    if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pRec->GetName().find(find_text) != std::string::npos) {
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
                        if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pItem->GetName().find(find_text) != std::string::npos) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || dict_value_set.GetName().find(find_text) != std::string::npos) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE) {
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
                            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || dict_value_set.GetName().find(find_text) != std::string::npos) {
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
                        if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pItem->GetName().find(find_text) != std::string::npos) {
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
                    if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pRec->GetName().find(find_text) != std::string::npos) {
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
                        if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE) {
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
                        if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || dict_value_set.GetName().find(find_text) != std::string::npos) {
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
                    if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != -1 || pItem->GetName().find(find_text) != std::string::npos) {
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
                if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || pLevel->GetName().find(find_text) != std::string::npos) {
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
            if (csLabel.Find(UTF8_TODO::GetCString(find_text)) != NONE || GetName().find(find_text) != std::string::npos) {
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

        std::string GetMd5() const
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
            const size_t current_size = m_structureData.size();
            m_structureData.resize(current_size + data_length);
            memcpy(m_structureData.data() + current_size, data, data_length);
        }

        void Process(const std::string& text)
        {
            Process(text.c_str(), text.length());
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


std::string CDataDict::GetStructureMd5() const
{
    DictionaryIteratorForStructureMd5 iterator;
    iterator.Iterate(const_cast<CDataDict&>(*this));
    return iterator.GetMd5();
}


uint32_t CDataDict::GetIdStructureHashForKeyIndex(const bool hash_name, const bool hash_start) const
{
    // calculate a hash that describes the IDs
    uint32_t id_structure_hash = 0;

    const CDictRecord& dict_id_record = *m_dictLevels.front().GetIdItemsRec();

    for( int i = 0; i < dict_id_record.GetNumItems(); ++i )
    {
        const CDictItem& dict_id_item = *dict_id_record.GetItem(i);

        ASSERT(!DictionaryRules::CanHaveSubitems(dict_id_record, dict_id_item) &&
               !DictionaryRules::CanItemHaveMultipleOccurrences(dict_id_record) &&
               !DictionaryRules::CanHaveDecimals(dict_id_record, dict_id_item.GetContentType()));

        if( hash_name )
            Hash::Combine(id_structure_hash, dict_id_item.GetName());

        if( hash_start )
            Hash::Combine(id_structure_hash, dict_id_item.GetStart());

        Hash::Combine(id_structure_hash, dict_id_item.GetLen());
        Hash::Combine(id_structure_hash, dict_id_item.GetContentType());
        Hash::Combine(id_structure_hash, dict_id_item.GetZeroFill());
    }

    return id_structure_hash;
}


const std::string& CDataDict::GetSyncableName(const bool get_evaluated_name/* = true*/) const
{
    if( get_evaluated_name && m_syncableName.empty() )
        return GetName();

    return m_syncableName;
}


void CDataDict::SetSyncableName(std::string syncable_name)
{
    if( syncable_name == GetName() || SO::IsWhitespace(syncable_name) )
    {
        m_syncableName.clear();
    }

    else
    {
        m_syncableName = std::move(syncable_name);
    }
}


std::string CDataDict::MakeQualifiedName(const std::string& name) const
{
    return FormatText("%s.%s", GetName().c_str(), name.c_str());
}


/////////////////////////////////////////////////////////////////////////////
//
//                         CDataDict::GetFileModifiedTime
//
/////////////////////////////////////////////////////////////////////////////

int64_t CDataDict::GetFileModifiedTime() const
{
    return m_serializedFileModifiedTime.has_value() ? *m_serializedFileModifiedTime :
                                                      PortableFunctions::FileModifiedTime(m_filePath);
}


const CDictRecord* CDataDict::FindRecord(const std::string_view record_name_sv) const
{
    for( const DictLevel& dict_level : m_dictLevels )
    {
        for( int r = -1; r < dict_level.GetNumRecords(); ++r )
        {
            const CDictRecord* const dict_record = ( r == -1 ) ? dict_level.GetIdItemsRec() :
                                                                 dict_level.GetRecord(r);

            if( SO::EqualsNoCase(record_name_sv, dict_record->GetName()) )
                return dict_record;
        }
    }

    return nullptr;
}


const CDictItem* CDataDict::FindItem(const std::string_view item_name_sv) const
{
    for( const DictLevel& dict_level : m_dictLevels )
    {
        for( int iRecord = -1; iRecord < dict_level.GetNumRecords(); ++iRecord )
        {
            const CDictRecord* const dict_record = ( iRecord == -1 ) ? dict_level.GetIdItemsRec() :
                                                                       dict_level.GetRecord(iRecord);

            const CDictItem* const dict_item = dict_record->FindItem(item_name_sv);

            if( dict_item != nullptr )
                return dict_item;
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
                dict_record->SetName(FormatText("_IDS%d", static_cast<int>(level_number)));

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

std::optional<size_t> CDataDict::IsLanguageDefined(const std::string_view language_name_sv) const
{
    const auto& lookup = std::find_if(m_languages.cbegin(), m_languages.cend(),
                                      [&](const Language& language) { return SO::EqualsNoCase(language.GetName(), language_name_sv); });

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


std::vector<std::string> CDataDict::GetUniqueNames(const char* const prefix, int iNumDigits, int iNumNames) const
{
    std::vector<std::string> unique_names;

    for( int i = 0; i < iNumNames; ++i )
    {
        std::string name = FormatText("%s%0*d", prefix, iNumDigits, i + 1);
        name = CIMSAString::MakeName(name);
        unique_names.emplace_back(GetUniqueName(name));
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
                                    const std::vector<std::string>* const value_set_names_added_on_paste/* = nullptr*/)
{
    // - if action_or_updated_dict_value_set is not null, then all value sets linked to it are updated with the values in action_or_updated_dict_value_set
    // - if the action is UpdateValuesFromLinks, all value sets are updated based on the values in the value set with the most values
    // - if the action is OnPaste, the value sets are updated similarly to UpdateValuesFromLinks except that source value set (with the most values)
    //      will not be one of the value sets named in the value_set_names_added_on_paste vector

    bool using_action = std::holds_alternative<SyncLinkedValueSetsAction>(action_or_updated_dict_value_set);
    ASSERT(using_action || ( std::get<DictValueSet*>(action_or_updated_dict_value_set) != nullptr &&
                             std::get<DictValueSet*>(action_or_updated_dict_value_set)->IsLinkedValueSet() ));

    // first pass: figure out the linkages
    std::map<std::string, std::vector<DictValueSet*>> linked_value_sets;

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


std::unique_ptr<CDataDict> CDataDict::InstantiateAndOpen(const InterfaceString& file_path, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    auto dictionary = std::make_unique<CDataDict>();
    dictionary->Open(file_path, silent, std::move(message_logger));
    return dictionary;
}


void CDataDict::Open(const InterfaceString& file_path, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(file_path, std::move(message_logger), [&]() { return ConvertPre80SpecFile(file_path); });
    return Open(*json_reader, silent);
}


void CDataDict::Open(JsonSpecFile::Reader& json_reader, const bool silent)
{
    try
    {
        json_reader.CheckVersion();
        json_reader.CheckFileType(JK::dictionary);

        CreateFromJsonWorker(json_reader);

        m_filePath = json_reader.GetFilePath();
    }

    catch( const CSProException& exception )
    {
        json_reader.GetMessageLogger().RethrowException(json_reader.GetFilePath(), exception);
    }

    // report any warnings
    json_reader.GetMessageLogger().DisplayWarnings(silent);
}


void CDataDict::OpenFromText(const std::string_view text_sv)
{
    if( !text_sv.empty() && text_sv.front() == JsonSpecFile::Pre80SpecFileStartCharacter )
    {
        // save the pre-JSON file to a temporary file and open it
        TemporaryFile temporary_file;
        FileIO::WriteText(temporary_file.GetPath(), text_sv, true);
        ConvertPre80SpecFile(temporary_file.GetPath());
        Open(temporary_file.GetPath(), true);
    }

    else
    {
        const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader("", text_sv);
        return Open(*json_reader, true);
    }
}


void CDataDict::Save(std::string file_path, bool continue_using_file_path/* = true*/) const
{
    const std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(file_path, JK::dictionary);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    if( continue_using_file_path )
        const_cast<CDataDict*>(this)->m_filePath = std::move(file_path);
}


std::string CDataDict::GetJson(const bool spec_file_format/* = true*/) const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( spec_file_format )
        JsonSpecFile::WriteHeading(*json_writer, JK::dictionary);

    WriteJson(*json_writer, false);

    json_writer->EndObject();

    return json_writer->ReleaseString();
}


template<typename T/* = CDataDict*/>
T CDataDict::CreateFromJson(const JsonNode& json_node)
{
    T dictionary;

    if constexpr(std::is_same_v<T, std::unique_ptr<CDataDict>>)
    {
        dictionary = std::make_unique<CDataDict>();
        dictionary->CreateFromJsonWorker(json_node);
    }

    else
    {
        dictionary.CreateFromJsonWorker(json_node);
    }

    return dictionary;
}

template CLASS_DECL_ZDICTO CDataDict CDataDict::CreateFromJson(const JsonNode& json_node);
template CLASS_DECL_ZDICTO std::unique_ptr<CDataDict> CDataDict::CreateFromJson(const JsonNode& json_node);


void CDataDict::CreateFromJsonWorker(const JsonNode& json_node)
{
    // some values are assumed to come in with the default values
    ASSERT(m_languages.size() == 1 && m_languages[0] == Language());
    ASSERT(!m_allowDataManagerModifications && !m_allowExport && m_cachedPasswordMinutes == 0);

    DictionarySerializerHelper dict_serializer_helper(*this);
    const auto dict_serializer_helper_holder = json_node.GetSerializerHelper().Register(&dict_serializer_helper);

    // only parse DictBase after the languages have been parsed
    DictNamedBase::ParseJsonInput(json_node, false);

    if( json_node.Contains(JK::sync) )
        SetSyncableName(json_node.Get(JK::sync).GetOrConstruct<std::string>(JK::name));

    if( json_node.Contains(JK::languages) )
    {
        m_languages = json_node.GetArray(JK::languages).GetVector<Language>();

        // add a default language if no languages were defind
        if( m_languages.empty() )
        {
            json_node.LogWarning("A default language was added to '%s'", GetName().c_str());
            m_languages.emplace_back();
        }
    }

    DictBase::ParseJsonInput(json_node);

    if( json_node.Contains(JK::security) )
    {
        DeserializeSecurityOptions(json_node.Get(JK::security).Get<std::string_view>(JK::settings), GetName(),
                                   m_allowDataManagerModifications, m_allowExport, m_cachedPasswordMinutes);
    }

    m_readOptimization = json_node.GetOrDefault(JK::readOptimization, DictionaryDefaults::ReadOptimization);

    const JsonNode record_type_node = json_node.Get(JK::recordType);
    m_uRecTypeStart = record_type_node.Get<unsigned>(JK::start);
    m_uRecTypeLen = record_type_node.Get<unsigned>(JK::length);

    const JsonNode defaults_node = json_node.GetOrEmpty(JK::defaults);
    m_bDecChar = defaults_node.GetOrDefault(JK::decimalMark, DictionaryDefaults::DecChar);
    m_bZeroFill = defaults_node.GetOrDefault(JK::zeroFill, DictionaryDefaults::ZeroFill);

    m_bPosRelative = json_node.Get<bool>(JK::relativePositions);


    // levels
    m_dictLevels = json_node.GetArrayOrEmpty(JK::levels).GetVector<DictLevel>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning("A level was not added to '%s' due to errors: %s", GetName().c_str(), exception.what());
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
            json_node.LogWarning("A relation was not added to '%s' due to errors: %s", GetName().c_str(), exception.what());
        });

    m_enableBinaryItems = ( dict_serializer_helper.GetUsesBinaryItems() ||
                            json_node.GetOrDefault("enableBinaryItems", false) );

    // finalize the dictionary
    BuildNameList();
    UpdatePointers();
    SyncLinkedValueSets();
}


void CDataDict::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
{
    DictionarySerializerHelper dict_serializer_helper(*this);
    const auto dict_serializer_helper_holder = json_writer.GetSerializerHelper().Register(&dict_serializer_helper);

    if( write_to_new_json_object )
        json_writer.BeginObject();

    // DictBase will be written once the languages have been written
    DictNamedBase::WriteJson(json_writer, false);

    if( !m_syncableName.empty() )
    {
        json_writer.BeginObject(JK::sync)
                   .Write(JK::name, m_syncableName)
                   .EndObject();
    }

    ASSERT(!m_languages.empty());

    if( json_writer.Verbose() || m_languages.size() > 1 || m_languages.front() != Language() )
        json_writer.Write(JK::languages, m_languages);

    DictBase::WriteJson(json_writer);

    json_writer.BeginObject(JK::security)
               .Write(JK::allowDataManagerModifications, m_allowDataManagerModifications)
               .Write(JK::allowExport, m_allowExport)
               .Write(JK::cachedPasswordMinutes, m_cachedPasswordMinutes)
               .Write(JK::settings, SerializeSecurityOptions(GetName(), m_allowDataManagerModifications, m_allowExport, m_cachedPasswordMinutes))
               .EndObject();

    json_writer.Write(JK::readOptimization, m_readOptimization);

    json_writer.WriteIfNot("enableBinaryItems", m_enableBinaryItems, false);

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
    const auto dict_serializer_helper_holder = ar.GetSerializerHelper().Register(std::make_unique<DictionarySerializerHelper>(*this));

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

    ar & m_oldName;
    ar & m_iSymbol;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
        ar & m_syncableName;

    ar & m_languages;

    ar & m_allowDataManagerModifications
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
    if( std::wstring(GetCommandLine()).find(L"/extract") != std::wstring::npos )
    {
        const std::string file_path = PortableFunctions::CreateFilePath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop),
                                                                        GetName(), FileExtensions::Dictionary);
        Save(file_path);
    }
#endif
}


std::string CDataDict::SerializeSecurityOptions(const std::string& dictionary_name, const bool allow_data_manager_modifications,
                                                const bool allow_export, const int cached_password_minutes)
{
    const std::string security_options_text = FormatText("%s\tv1\t%d\t%d\t%d",
                                                         dictionary_name.c_str(),
                                                         allow_data_manager_modifications ? 1 : 0,
                                                         allow_export ? 1 : 0,
                                                         cached_password_minutes);

    Encryptor encryptor(Encryptor::Type::RijndaelHex, dictionary_name);
    return encryptor.Encrypt(security_options_text);
}


void CDataDict::DeserializeSecurityOptions(const std::string_view encrypted_security_options_sv, const std::string& dictionary_name,
                                           bool& allow_data_manager_modifications, bool& allow_export, int& cached_password_minutes)
{
    Encryptor encryptor(Encryptor::Type::RijndaelHex, dictionary_name);
    const std::string security_options_text = encryptor.Decrypt(encrypted_security_options_sv);
    const std::vector<std::string> security_options = SO::SplitString(security_options_text, "\t");

    // only process text that was correctly decrypted
    if( security_options.size() == 5 &&
        security_options[0] == dictionary_name &&
        security_options[1] == "v1" )
    {
        allow_data_manager_modifications = ( CIMSAString::Val(security_options[2]) == 1 );
        allow_export = ( CIMSAString::Val(security_options[3]) == 1 );
        cached_password_minutes = static_cast<int>(CIMSAString::Val(security_options[4]));
    }
}
