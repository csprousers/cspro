#pragma once

//---------------------------------------------------------------------------
//  File name: DicT.h
//
//  Description:
//          Header for engine-Dic class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              27 Nov 99   RHF     Basic conversion
//              03 Dec 99   vc      Customization
//              24 Feb 00   RHF+vc  Adding WriteCaseBehavior
//              27 Apr 00   RHF     Fix bug of uninitialized var
//              04 Jul 00   RHF     Adding map of record-codes for faster operation
//
//---------------------------------------------------------------------------
#include <zLogicO/Symbol.h>
#include <engine/Defines.h>
#include <engine/COMMONIN.H>

struct BinaryStorageFor80;
class CaseAccess;
class CDataDict;
class CEngineArea;
class CEngineDriver;

struct DICX;                                           // victor Jul 04, 00

//---------------------------------------------------------------------------
//
//  class CSymbolDict : public Symbol
//
//  Description:
//      Dictionaries
//
//  Construction/Destruction
//      CSymbolDict                 Constructor
//      ~CSymbolDict                Destructor
//      Init                        Initialize Dict' instance
//
//  Dict' contents
//      SetFirstSymSec              Set the symbol index of the first section
//      GetFirstSymSec              Return the symbol index of the first section
//      SetLastSymSec               Set the symbol index of the last section
//      GetLastSymSec               Return the symbol index of the last section
//
//  Descriptors for Level & Sect' identifiers
//      SetMaxLevel                 Set the maximum level allowed for the dictionary
//      GetMaxLevel                 Return the maximum level allowed for the dictionary
//      SetLevelsIdLen              Set the ids' total length (all levels)
//      GetLevelsIdLen              Return the ids' total length (all levels)
//      SetSecIdLoc                 Set the location of the section' identifier
//      GetSecIdLoc                 Return the location of the section' identifier
//      SetSecIdLen                 Set the length of the section' identifier
//      GetSecIdLen                 Return the length of the section' identifier
//
//  Dictionary status, stamps & file management
//      <none>
//
//  Common
//      Common_DoBuffer             ???
//      DictDoMainKey               ???
//      Common_Start                ???
//      Common_End                  ???
//      Common_DoSect               ???
//      Common_GetCommonSection     ???
//      Common_GetData              ???
//      Common_GetCommonFirstPos    ???
//      Common_GetCommonLastPos     ???
//      Common_GetCommonLen         ???
//
//  Dict keys (basic - primary key only)
//      ResetPrimaryKey             Reset the primary key values of both initial & current
//      SetPrimaryKey               Set the primary key value of the initial or current
//      GetPrimaryKey               Get the primary key value of the initial or current
//
//  Flow owner (the Flow owning this Dict)
//      SetFlow
//      GetFlow
//
//  Engine links
//      SetEngineDriver             Links to engine
//
//---------------------------------------------------------------------------

namespace DictionaryAccess
{
    const int RequiresIndex = 0x1;
    const int CannotHaveIndex = 0x2;
    const int IsWriteable = 0x4;
    const int HasDynamicFileManagement = 0x8;
    const int UsesSync = 0x10;
};


// DICT : dictionary
class CSymbolDict : public Symbol
{
        // --- Data members --------------------------------------------------------
    // --- executor brother-entry
private:
    DICX*   m_pDicX;                    // DICX* brother

    // --- dict' contents
public:
    int     SYMTfsec;                   // Symt entry with Dict' first section
    int     SYMTlsec;                   //                       last  section

    // --- descriptors for Level & Sect' identifiers
public:
    int     maxlevel;                   // Max level of the dictionary
    int     qloc[MaxNumberLevels];      // Q-id location (for Q-id's 1, 2, 3 & 4)
    int     qlen[MaxNumberLevels];      //      lengths
    int     qidlen;                     // Q-id total length (all levels)
    int     sloc;                       // S-id location
    int     slen;                       //      length

private:
    int     m_iNumDim;  // Max number of dimensions

public:
    int     m_iDictionaryAccessFlags;

    // --- common
public:
    COMMON_INFO CommonInfo;

    // --- dict keys (basic - primary key only)
private:
    CString m_PrimaryKeyInitial;        // initial contents
    CString m_PrimaryKeyCurrent;        // at-any-time contents

    // --- flow owner
private:
    FLOW*           m_pFlow;            // Flow owning this Dict

private:
    std::shared_ptr<const CDataDict> m_pDataDict;
    std::shared_ptr<CaseAccess> m_caseAccess;

public:
    const CDataDict* GetDataDict() const                         { return m_pDataDict.get(); }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_pDataDict; }
    void SetDataDict(std::shared_ptr<const CDataDict> dictionary);

    CaseAccess* GetCaseAccess() const                       { return m_caseAccess.get(); }
    std::shared_ptr<CaseAccess> GetSharedCaseAccess() const { return m_caseAccess; }

    void SetNumDim( int iNumDim ) { m_iNumDim = iNumDim; }
    int  GetNumDim() const        { return m_iNumDim; }

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;

    std::vector<std::shared_ptr<BinaryStorageFor80>> m_binaryStorageFor80;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction
public:
    CSymbolDict(std::wstring name, CEngineDriver* pEngineDriver);
    ~CSymbolDict();

    // --- executor brother-entry
public:
    DICX*       GetDicX()                       { return m_pDicX; }
    const DICX* GetDicX() const                 { return m_pDicX; }
    void        SetDicX(DICX* pDicX)            { m_pDicX = pDicX; }

    // --- dict' contents
public:
    void        SetFirstSymSec( int iSymSec )   { SYMTfsec = iSymSec; }
    int         GetFirstSymSec() const          { return SYMTfsec; }
    void        SetLastSymSec( int iSymSec )    { SYMTlsec = iSymSec; }
    int         GetLastSymSec() const           { return SYMTlsec; }

    // --- descriptors for Level & Sect' identifiers
public:
    void        SetMaxLevel(int iMaxLevel)  { maxlevel = iMaxLevel; }
    int         GetMaxLevel() const         { return maxlevel; }
    bool        IsMultiLevel() const        { return ( maxlevel > 1 ); }
    void        SetLevelsIdLen( int iIdLen) { qidlen = iIdLen; }
    int         GetLevelsIdLen() const      { return qidlen; }
    void        SetSecIdLoc(int iLoc)       { sloc = iLoc; }
    int         GetSecIdLoc() const         { return sloc; }
    void        SetSecIdLen(int iLen)       { slen = iLen; }
    int         GetSecIdLen() const         { return slen; }
    int         GetSecIdLastPos()           { return sloc + slen; } // victor Jul 10, 00

    // --- dictionary status, stamps & file management
public:
    int     GetDictionaryAccessFlags() const     { return m_iDictionaryAccessFlags; }
    void    SetDictionaryAccessFlags(int iFlags) { m_iDictionaryAccessFlags = iFlags; }

    bool    GetNeedsIndex() const               { return ( m_iDictionaryAccessFlags & DictionaryAccess::RequiresIndex ) != 0; }
    void    SetNeedsIndex()                     { m_iDictionaryAccessFlags |= DictionaryAccess::RequiresIndex; }
    bool    GetCannotHaveIndex() const          { return ( m_iDictionaryAccessFlags & DictionaryAccess::CannotHaveIndex ) != 0; }
    void    SetCannotHaveIndex()                { m_iDictionaryAccessFlags |= DictionaryAccess::CannotHaveIndex; }
    bool    GetWriteable() const                { return ( m_iDictionaryAccessFlags & DictionaryAccess::IsWriteable ) != 0; }
    void    SetWriteable()                      { m_iDictionaryAccessFlags |= DictionaryAccess::IsWriteable; }
    bool    GetHasDynamicFileManagement() const { return ( m_iDictionaryAccessFlags & DictionaryAccess::HasDynamicFileManagement ) != 0; }
    void    SetHasDynamicFileManagement()       { m_iDictionaryAccessFlags |= DictionaryAccess::HasDynamicFileManagement; }
    bool    GetUsesSync() const                 { return ( m_iDictionaryAccessFlags & DictionaryAccess::UsesSync ) != 0; }
    void    SetUsesSync()                       { m_iDictionaryAccessFlags |= DictionaryAccess::UsesSync; }

    // --- common
public:
    int         Common_DoBuffer(TCHAR* pBuffer, int iFromLevel, int iUntilLevel);
    int         DictDoMainKey(TCHAR* pBuffer, const TCHAR* pSourceBuffer, int iFromLevel, int iUntilLevel);
    void        Common_Start();
    void        Common_End();
    void        Common_DoSect( int bClear );
    int         Common_GetCommonSection( int iLevel );
    void        Common_GetData( int iFromLevel, int iUntilLevel );
    int         Common_GetCommonFirstPos( int iLevel ); // 1-based
    int         Common_GetCommonLastPos( int iLevel );  // 1-based
    int         Common_GetCommonLen( int iLevel );

    // --- dict keys (basic - primary key only)
public:
    //  - iType is 0 for initial key (current key otherwise)
    //  - pls call ResetPrimaryKey to empty both keys prior to any use
    void        ResetPrimaryKey();
    void        SetPrimaryKey( bool bCurrent = false );
    void        GetPrimaryKey( TCHAR* pszKey, bool bCurrent = false );

    // --- flow owner (the Flow owning this Dict)
public:
    void        SetFlow( FLOW* pFlow )    { m_pFlow = pFlow; }
    FLOW*       GetFlow()                 { return m_pFlow; }

    // Symbol overrides
    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    const Logic::SymbolTable& GetSymbolTable() const;

private:
    int m_containerIndex = 0; // the container table index

public:
    int GetContainerIndex() const  			    { return m_containerIndex; }
    void SetContainerIndex(int container_index) { m_containerIndex = container_index; }
};

#include <engine/VarT.h>
