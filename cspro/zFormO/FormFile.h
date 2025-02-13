#pragma once

//***************************************************************************
//      File name: FormFile.h
//
//      Description:
//               the base class defs for IMSA's data entry applications (EntryDev/Run)
//
//      History:
//              Date
//              (yy-mm-dd)      Author  Comment
//              ---------------------------------------------------------------------
//              99-01-15        smg             this file is a revamp of the IMSA ApClass.h file, reflecting the
//                                              new file structure (.apw->.frm); removal of CDEGroup and CDELevel
//
//
//***************************************************************************

#include <zFormO/zFormO.h>
#include <zFormO/Box.h>
#include <zFormO/Block.h>
#include <zFormO/Field.h>
#include <zFormO/FieldColors.h>
#include <zFormO/Form.h>
#include <zFormO/Level.h>
#include <zFormO/Text.h>
#include <zDictO/DDClass.h>

class CAppLoader;
class CDEFormFile;
class CDERoster;
class DragOptions;


//      default command strings

// smg:can we have a more generalized file to contain some of the oft-used strings
// (such as name, label, version) and err msgs?  i'm recreating the defs herein cause
// i sorta don't want to link to impsdict.h if that's the *only* thing i'm using it for

#define FRM_CMD_PRIMARY                 _T("Primary")       // used by CDEFormFile
#define FRM_CMD_SECONDARY               _T("Secondary")
#define FRM_CMD_FONT                    _T("Font")
#define FRM_FLD_FONT                    _T("FieldEntryFont")
#define FRM_DEF_TXTFONT                 _T("DefaultTextFont")
#define FRM_CMD_PATH                    _T("Type")
#define FRM_CMD_FORANGE                 _T("ForceOutOfRange")

#define FRM_FIELDCOLOR_BASE             _T("FieldColor")
#define FRM_FIELDCOLOR_NOTVISITED       _T("NotVisited")
#define FRM_FIELDCOLORS_VISITED         _T("Visited")
#define FRM_FIELDCOLORS_CURRENT         _T("Current")
#define FRM_FIELDCOLORS_SKIPPED         _T("Skipped")

#define FRM_CMD_NAME                    _T("Name")
#define FRM_CMD_LABEL                   _T("Label")
#define FRM_CMD_POSITION                _T("Position")              // string needed by CDEFormBase
#define FRM_CMD_FORMNUM                 _T("Form")                  // used by CDEItemBase
#define FRM_CMD_RECORD                  _T("Record")
#define FRM_CMD_QSTXT_HGT               _T("QuestionTextHeight")

#define FRM_CMD_DICTORDER               _T("DictionaryOrder")
#define FRM_CMD_FILE                    _T("File")

#define FRM_CMD_ITEM                    _T("Item")                  // strings used by CDEField
#define FRM_CMD_SUBITEM                 _T("SubItem")
#define FRM_CMD_INPUT                   _T("InputType")
#define FRM_CMD_LENGTH                  _T("Length")

#define FRM_CMD_ENTERKEY                _T("UseEnterKey")
#define FRM_CMD_VERIFY                  _T("Verify")
#define FRM_CMD_FORCEORANGE             _T("ForceOutofRange")
#define FRM_CMD_VALIDATION_METHOD       _T("ValidationMethod")
#define FRM_CMD_PROTECTED               _T("Protected")
#define FRM_CMD_SEQUENTIAL              _T("Sequential")
#define FRM_CMD_MIRROR                  _T("Mirror")
#define FRM_CMD_PERSISTENT              _T("Persistent")
#define FRM_CMD_UPPERCASE               _T("UpperCase")
#define FRM_CMD_USEUNICODETEXTBOX       _T("UseUnicodeTextBox")
#define FRM_CMD_ALLOWMULTILINE          _T("AllowMultiLine")
#define FRM_CMD_FIELD_LABEL_TYPE        _T("FieldLabelType")
#define FRM_CMD_HIDE_IN_CASETREE        _T("HideInCaseTree")
#define FRM_CMD_ALWAYS_VISUAL_VALUE     _T("AlwaysVisualValue")

#define FRM_CMD_KEYBOARD_ID             _T("Keyboard") // 20120817

#define FRM_CMD_WHOLEFORM               _T("WholeForm")

#define FRM_CMD_SKIPTO                  _T("SkipTo")

#define FRM_CMD_TEXT                    _T("Text")                      // strings needed by CDEText
#define FRM_CMD_COLOR                   _T("Color")
#define FRM_CAPTUREPOS                  _T("CapturePos") // 20120405
#define FRM_CMD_HORZ_ALIGN              _T("HorizontalAlignment")  // csc 12/20/00
#define FRM_CMD_VERT_ALIGN              _T("VerticalAlignment")    // csc 12/20/00
#define FRM_CMD_ALIGN_LEFT              _T("Left")
#define FRM_CMD_ALIGN_CENTER            _T("Center")
#define FRM_CMD_ALIGN_RIGHT             _T("Right")
#define FRM_CMD_ALIGN_TOP               _T("Top")
#define FRM_CMD_ALIGN_MIDDLE            _T("Middle")
#define FRM_CMD_ALIGN_BOTTOM            _T("Bottom")

#define FRM_CMD_LOGFONT                 _T("Font")          //Glenn's Spec

#define FRM_CMD_SIZE                    _T("Size")          // used by CDEForm
#define FRM_CMD_LEVEL                   _T("Level")
#define FRM_CMD_REPEAT                  _T("Repeat")
#define FRM_CMD_SINGLE                  _T("Single")
#define FRM_CMD_MULTIPLE                _T("Multiple")

#define FRM_CMD_BOX                     _T("Box")           // used by CDEBox, i.e., Box=#,#,#,#,Style

#define FRM_CMD_ETCHEDBOX               _T("Etched")        // box style types
#define FRM_CMD_RAISEDBOX               _T("Raised")
#define FRM_CMD_THINBOX                 _T("Thin")
#define FRM_CMD_THICKBOX                _T("Thick")

#define FRM_CMD_UNKNOWN                 _T("Unknown")

#define FRM_CMD_KEYED                   _T("Keyed")         // for use w/enum eInputType
#define FRM_CMD_PERSISTENT              _T("Persistent")    // finish up eEntryType
#define FRM_CMD_AUTOINCREMENT           _T("AutoIncrement")

#define FRM_CMD_ROW                     _T("Row")           // for use w/enum eRCType and CDEFreeCell
#define FRM_CMD_COL                     _T("Col")           //      "

#define FRM_CMD_ITEMORIENT              _T("ItemOrientation")
#define FRM_CMD_ENTRYORIENT             _T("EntryOrientation")
#define FRM_CMD_OCCLABEL                _T("Occurrence")

#define ROSTER_ORIENT_HORZ              _T("Horizontal")
#define ROSTER_ORIENT_VERT              _T("Vertical")

// for blocks
#define FRM_CMD_DISPLAY_TOGETHER        _T("DisplayTogether")

//////////////////////////////////////////////////////////
//      default section headings

#define HEAD_FORM_FILE          _T("[FormFile]")
#define HEAD_LEVEL              _T("[Level]")
#define HEAD_GROUP              _T("[Group]")
#define HEAD_ENDGROUP           _T("[EndGroup]")
#define HEAD_FORM               _T("[Form]")
#define HEAD_ENDFORM            _T("[EndForm]")             // nd, as a [page] can contain variable # of [RFT]s
#define HEAD_TEXT               _T("[Text]")
#define HEAD_FIELD              _T("[Field]")
#define HEAD_ROSTER             _T("[Roster]")
#define HEAD_ENDROSTER          _T("[EndRoster]")   // nd, as a [Roster] as can contain variable # of [Field]s
#define HEAD_BLOCK              _T("[Block]")

#define sEmptyName   _T("EmptyName")
#define sEmptyLabel  _T("EmptyLabel")


//
// the following structure is useful for dict lookups; use elsewhere? &&& gsf
//
struct DICT_LOOKUP_INFO
{
    CString csName;
    const DictLevel* pLevel;
    const CDictRecord* pRecord;
    const CDictItem* pItem;
    const DictValueSet* pVSet;
    int iLevel;
    int iRecord;
    int iItem;
    int iVSet;
    int iOcc;
};

bool LookupSymbol(CDEFormFile* pFormFile, const CString& csDict, DICT_LOOKUP_INFO& structLookup);


constexpr int SEP_SIZE = 1; //used by zFormf and CSEntry for the tick seperator size

extern bool bShowFormSelDlg;


/***************************************************************************
*
*                       CDEFormFile : public CDEFormBase
*
***************************************************************************/

class CLASS_DECL_ZFORMO CDEFormFile : public CDEFormBase
{
    DECLARE_DYNAMIC (CDEFormFile)

public:
    // construction/destruction
    CDEFormFile();
    CDEFormFile(const CDEFormFile& ff);  // copy constructor
    CDEFormFile(const CString& sFormPathName, const CString& sDictPathName);
    ~CDEFormFile();

    void BaseConstructor ();

    void RenumberAllForms();

    bool IsPathOn () const   { return m_bPathOn; }
    void IsPathOn (bool b)  { m_bPathOn = b; }
    void IsPathOn (const CString& sVal);

    void SetRTLRostersFlag(bool bFlag)  { m_bRTLRosters = bFlag; }
    bool GetRTLRostersFlag() const      { return m_bRTLRosters; }

    bool Reconcile(CString& csErr, bool bSilent, bool bAutoFix);
    bool ReconcileLevels(CString& csErr, bool bSilent, bool bAutoFix);

    //Order reconcile .SAVY 07/25
    bool OReconcile(CString& csErr, bool bSilent, bool bAutoFix);
    bool CheckNAddMissingItems();

    bool IsDictOrder() const      { return m_bDictOrder;}
    void SetDictOrder(bool bFlag) { m_bDictOrder = bFlag;}

    bool FCheckNAddLevels();
    bool FindNMatchFLevel(const CDataDict& dictionary, int iDictLevel);

    void UpdatePointers ();     // func for ISSA; initializes the m_pParent ptrs in CDEItemBase
    void RefreshAssociatedFieldText();
    void UpdateDictItemPointers(CDEGroup* parent);
    bool SetDictItem(CDEField* pField);

    void CreateFormFile(const CDataDict* pDD, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                        const DragOptions& drag_options, int iDropSpacing = 500, bool bBuildRecords = false);

    CDEForm* CreateForm(int level, CDEGroup* pGroup=NULL);
    void CreateGroup(CDEGroup* pGroup, const DictLevel& dict_level, int iFormNum);

    void CreateGroup(CDEGroup* pGroup, const CDictRecord* pDictRec, int iFormNum, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                     int iDropSpacing, const DragOptions& drag_options, bool bIdRec = false, CDEGroup* pPrevGroup = NULL);


    void CreateRoster(CDERoster* pRoster, const CDictRecord* pDR, int iFormNum, CPoint dropPt,
                      const DragOptions& drag_options, bool bAddItemsFromRecord = true);

    void CreateRoster(CDERoster* pRoster, const CDictItem* pDI, int iFormNum, CPoint dropPt, const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                      const DragOptions& drag_options, int iOcc, bool bDropSubitems, const CDataDict* pDD);

    void    CreateRosterField(CDEField* pField, CDERoster* pRoster);

    ///FUNCTIONS USED FOR ORDER FILES
    void CreateOrderFile(CDataDict& dictionary, bool bBuildRecords = false);
    void CreateOrderFile(const CDataDict& dictionary, bool bBuildRecords = false);

    void CreateGroupForOrder (CDEGroup* pGroup,
                              const CDictRecord* pDictRec,
                              int iFormNum,
                              bool bIdRec = false,         // = false
                              CDEGroup* pPrevGroup = NULL); /* =NULL */
    //Find the group which has this record type name
    CDEGroup* FindGroup(CDEGroup* pRootGroup, const CString& sRecName);
    //find the group which has a field corresponding to this  item name
    CDEGroup* FindItemGroup(CDEGroup* pRootGroup, const CString& sItemName,bool bUseTypeName =false);
    bool EnsureAllItemsPresent(CDELevel* pLevel , const CDictRecord* pRecord);
    void RemoveGroupItemsFromForm(CDEGroup* pGroup , bool bRemove =true);
    CDEGroup* OCreateGroupField(CDEGroup* pGroup, const CDictItem* pDI, int iFormNum);

    //      to implement the drag opts dialog settings, i.e., allowing subitems to be dropped
    //      instead of the item itself, i've had to write some dictionary-related support funcs

    int  GetNumDictSubitems(const CDictRecord* pDR, const CDictItem* pDI) const;
    int  GetDictItemParentIndex(const CDictRecord* pDR, const CDictItem* pDI);
    bool DoAnySubitemsOverlap(const CDictRecord* pDR, const CDictItem* pDI);
    bool AreAnySubitemsBeingKeyed(const CDictRecord* pDR, const CDictItem* pDI);
    bool TestSubitemDrop(const CDictRecord* pDR, const CDictItem* pDI, const DragOptions& drag_options, bool bCheck4KeyedSubs=true);

    // archiving methods

    bool Open(const CString& csFileName, bool bSilent=false);

    bool Build(CSpecFile& frmFile, std::shared_ptr<ProgressDlg> = nullptr);

    bool BuildWrapUp ();

    void Save(CSpecFile& frmFile) const override;
    bool Save(const CString& csFileName) const;

    void SetVersion(const CString& cs)  { m_csVersion = cs; }
    const CString& GetVersion() const   { return m_csVersion; }

    // operators
    void operator=(const CDEFormFile& ff);
    bool operator==(const CDEFormFile& ff) const;
    bool operator!=(const CDEFormFile& ff) const { return !operator==(ff); }

    // methods for dicts
    const CString& GetDictionaryFilename() const                   { return m_dictionaryFilename; }
    void SetDictionaryFilename(const CString& dictionary_filename) { m_dictionaryFilename = dictionary_filename; }

    const CString& GetDictionaryName() const               { return m_dictionaryName; }
    void SetDictionaryName(const CString& dictionary_name) { m_dictionaryName = dictionary_name; }

    const CDataDict* GetDictionary() const                          { return m_dictionary.get(); }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const    { return m_dictionary; }
    void SetDictionary(std::shared_ptr<const CDataDict> dictionary) { m_dictionary = dictionary; }

    // methods for forms
    int  GetNumForms        () const                        { return (int)m_aForm.size(); }
    void AddForm            (CDEForm* pForm)                { m_aForm.emplace_back(pForm); }
    void RemoveForm         (int iIndex, bool bRenumber=true);
    void RemoveFormAt       (int iIndex);
    void RemoveAllForms ();


    CDEForm*        GetForm                 (int i) const;

    // methods for Levels
    int             GetNumLevels() const        { return (int)m_aLevel.size(); }
    CDELevel*       GetLevel(int i) const       { return m_aLevel [i];}
    CDELevel*       GetLastLevel() const        { return m_aLevel.back(); }
    void            AddLevel(CDELevel* pLevel)  { m_aLevel.emplace_back(pLevel); }

    void            InsertLevelAt(CDELevel* pLevel, int ndx) { m_aLevel.insert(m_aLevel.begin() + ndx, pLevel); }
    void            InsertFormAt(CDEForm*  pForm, int ndx)   { m_aForm.insert(m_aForm.begin() + ndx, pForm); }

    void            RemoveAllLevels(); // don't know how, but can't find def of this in IMPSDeAp


    // unique names
    bool IsNameUnique(const CString& name) const { return ( m_uniqueNames.find(name) == m_uniqueNames.cend() ); }
    void RemoveUniqueName(const CString& name)   { m_uniqueNames.erase(name); }
    void RemoveAllUniqueNames()                  { m_uniqueNames.clear(); }

    void AddUniqueName(const CString& name);

    void BuildUniqueNL();
    void AddUniqueNames(const CDEGroup* pGroup);
    void RemoveUniqueNames(const CDEGroup* pGroup);

    CString CreateUniqueName(CString name, bool add_name = true);


    std::vector<CDEField*> GetAllFields() const;


    // searching funcs

    // these funcs use the dictionary's name

    bool FindItem(const CString& sDictName) const;
    bool FindItem(const CString& sDictName, CDEForm** pLocatedForm);

    // whereas these three funcs use the unique name to the FF

    bool FindItem(const CString& sFFName, int iFormIndex, int* iItemIndex) const;
    bool FindItem(const CString& sFFName, CDEForm** pForm, CDEItemBase** pItem);

    bool FindItem(CDEForm* pForm, CPoint cPT, CDEItemBase** pItem);

    CDERoster* FindRoster(const CString& sRosterName);

    bool FindField(const CString& sFldName, CDEForm** pForm, CDEItemBase** pItem);

    // and this func just looks for a form based on it's name!

    int  FindForm (const CString& sName, bool bByGroupName = false ); // RHF Nov 28, 2002 Add bByGroupName

    void UpdateGroupFormIndices (int iStartingFormNo);
    void UpdateFormFieldIndices (int iStartingFormNo);      // used by RemoveForm()

    //      these funcs help to renumber the forms and their items

    void RenumberFormsAndItems ();      // see comment w/the func for its purpose

    //      more funcs

    bool RemoveItem(const CString& sItemName);

    //Application Runtime Functions

    bool LoadRTDicts                (CAppLoader* pLoader); // Load the CDataDict objects @ runtime

#ifdef GENERATE_BINARY
    bool SaveRTDicts(const std::wstring& archive_name); // writes dictionary to disk
#endif

    const PortableFont& GetFieldFont() const { return m_fieldFont; }
    void SetFieldFont(PortableFont font)     { m_fieldFont = std::move(font); }

    const PortableFont& GetDefaultTextFont() const { return m_defaultTextFont; }
    void SetDefaultTextFont(PortableFont font)     { m_defaultTextFont = std::move(font); }

    //Savy &&&
    std::vector<CString> GetOrder() const;

    enum class UpdateFlagsNFontsAction { SetFontToDefaultTextFont, SetUseDefaultFontFlagAndFieldFont };
    void UpdateFlagsNFonts(UpdateFlagsNFontsAction action = UpdateFlagsNFontsAction::SetUseDefaultFontFlagAndFieldFont);

    //Runtime Functions by Savy
    CDEField* GetField(int iSym);

    FieldColors GetEvaluatedFieldColors() const   { return m_fieldColors.GetEvaluatedFieldColors(*this); }
    const FieldColors& GetFieldColors() const     { return m_fieldColors; }
    void SetFieldColors(FieldColors field_colors) { m_fieldColors = std::move(field_colors); }

    bool ReconcileName(const CDataDict& dictionary);
    void ChangeDName(const CDataDict& dictionary);
    void ChangeGName(const CString& sOldName, const CString& sNewName);

    void ClearUsed();
    void SetUsed(const CDictItem& dict_item, bool used = true);
    bool IsUsed(const CDictItem& dict_item) const;

    void CheckLevels();
    void CheckFormFile();

    bool ReconcileSkipTo(CString& sMsg);
    bool CheckValidSkip(CDEGroup* pGroup, CDEField* pSource, CString& sMsg);

    bool ReconcileFieldAttributes(CString& sMsg);
    bool CheckFieldAttributes(CDEField* pSource, CString& sMsg);

    bool Compare(CDEFormFile* pFormFile);

    // RHF INIC Nov 13, 2002
    void SetFileName(const CString& sFormPathName) { m_csFormPathName = sFormPathName; }
    const CString& GetFileName() const             { return m_csFormPathName; }

    // RHF END Nov 13, 2002
    void RenumberFormsNItems4BCH() ;
    void SetMaxFieldPointer ();

    void serialize(Serializer& ar);

private:
    void CopyFF(const CDEFormFile& ff);


private:
    bool m_bPathOn;     // path on means survey, off means census app
    bool m_bRTLRosters; // Set Rosters Right to Left for Arabic.

    CString m_csVersion; // IMSA 1.0, etc.

    CString m_dictionaryFilename;
    CString m_dictionaryName;
    std::shared_ptr<const CDataDict> m_dictionary;

    std::vector<CDEForm*> m_aForm;   // all forms that comprise the form file
    std::vector<CDELevel*> m_aLevel; // all levels w/in a form
    std::set<CString> m_uniqueNames; // list of all the unique names in the form file

    PortableFont m_fieldFont;
    PortableFont m_defaultTextFont;

    FieldColors m_fieldColors;

    bool m_bDictOrder; //used in ord file

    CString m_csFormPathName;

    std::set<const CDictItem*> m_usedDictItems;
};
