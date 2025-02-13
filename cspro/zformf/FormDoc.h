#pragma once

#include <zformf/zFormF.h>
#include <zformf/CapiEditorViewModel.h>
#include <zUtilO/UndoStack.h>
#include <zFormO/FormFile.h>
#include <zDesignerF/FormFileBasedDoc.h>

class Application;
class CapiQuestionManager;
class CFormID;
class CFormTreeCtrl;
class CFormScrollView;

#define FD_TEXT_FORMAT          3
#define FD_COLUMN_FORMAT        1
#define FD_BOX_FORMAT           2
#define FD_FORM_FORMAT          0


// if the op is UR_move and i'm moving a roster, i don't want to make a copy of
// the entire roster just for the change in dims; so, i'm going to create a dummy
// roster w/only the uniq name and dims initialized

/* *****************************************************************************
  notes on how i've implemented my undo/redo stack:

  i am in a slightly unusual situation in that boxes and free-standing text (FST)
  items belong to a CDEForm (and the CDELevel structure has NO knowledge of them),
  but the fields and rosters that will be keyed are contained within the CDELevel
  objs (beneath them, actually, in their CDEGroups)

  so if the user selects a mixed bag of entities (boxes, fields, etc), and wants
  to manipulate them, i can't just copy the form on which they lie, as it does not
  contain information on order/traversal or nesting; nor can i copy the CDEGroup
  structure, for as i mentioned above, it doesn't know about boxes and FST

  so what i've done is the following; i will be pushing on to the CUndoStack objs
  of the FormUndoStack type; FormUndoStack tells me [1] what the action was
  the user did (delete the objs, add them, or modify (move/resize/etc) them and
  [2] the array of objs that were effected

  the array of objs (CFormUndoObj) gives me the unique name of the parent (the
  form name if it's a box or FST, the group/roster name if it's a field/roster or
  field roster

  ***************************************************************************** */

enum class FormViewType { Form, Logic, QuestionText, Condition, Questionnaire };


class CFormUndoObj : public CObject
{
// m_pUndoItem  : the CDEForm, CDEField, CDEText, etc. being modified
// m_sParentName: unique name of the obj's parent; if a box or free-standing text (FST),
//                  then parent is a form;
// m_iIndex     : index of item w/in the parent's array; not important if a box or FST;

public:
    enum Action
    {
        UR_unknown,     // unknown undo/redo operation
        UR_delete,      // undo/redo op was deletion
        UR_modify,      // undo/redo op was modified
        UR_add,         // undo/redo op was add
        UR_move         // undo/redo op was to move the item(s); see next comment
    };

private:
    CObject*        m_pUndoItem;
    CString         m_sParentName;
    int             m_iIndex;
    Action          m_eAction;

public:
    CFormUndoObj ();
    CFormUndoObj (Action action, CObject* pUndoItem, int iIndex, const CString& sParentName);
    CFormUndoObj (CFormUndoObj& uo);
    ~CFormUndoObj();

    CFormUndoObj* ToggleUndoRedo(CDEFormFile* pFF);

    CObject*       GetUndoItem()          { return m_pUndoItem; }
    CObject*       GetRedoItem()          { return m_pUndoItem; }  // just for nicer symmetry in code
    const CString& GetParentsName() const { return m_sParentName; }
    int            GetIndex() const       { return m_iIndex; }

    Action      GetAction() const   { return m_eAction; }
    void        SetAction(Action a) { m_eAction = a; }

protected:
    CFormUndoObj* ToggleMoveOp(CDEFormFile* pFF);
    CFormUndoObj* ToggleModifyOp(CDEFormFile* pFF);
    CFormUndoObj* ToggleAddDeleteOp(CDEFormFile* pFF);
};

// *****************************************************************************

class FormUndoStack
{
public:
    FormUndoStack() { }
    FormUndoStack(CFormUndoObj::Action eAction, CObject* pURObj, int iIndex, const CString& sParentName);

    int GetNumURObjs() const { return (int)m_aURObjs.size(); }

    CFormUndoObj* GetURObj(int i) { return ( i < (int)m_aURObjs.size() ) ? m_aURObjs[i].get() : nullptr; }

    void PushUndoObj(CFormUndoObj::Action eAction, CObject* pURObj, int iIndex, const CString& sParentName);
    void PushMoveObj(const CObject* pURObj, int iIndex, const CString& sParentName);

    FormUndoStack ToggleUndoRedo(CDEFormFile* pFF);

private:
    std::vector<std::shared_ptr<CFormUndoObj>> m_aURObjs; // arr of undo/redo objects
};


/////////////////////////////////////////////////////////////////////////////
//
//                               CFormDoc
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZFORMF CFormDoc : public FormFileBasedDoc
{
    DECLARE_DYNCREATE(CFormDoc)

protected: 
    CFormDoc(); // create from serialization only

public:
    ~CFormDoc();

    static constexpr const TCHAR* GetExtensionWithDot() { return FileExtensions::WithDot::Form; }

    const CString& GetClipFile() const   { return m_csClipFile; }
    UINT GetClipBoardFormat(UINT format) { return m_auFormat[format]; }
    void FileToClip(UINT uFormat);
    bool ClipToFile(UINT uFormat);

    bool IsFormLoaded() const { return m_bFormLoaded; }

    void SetFFSpec(const CDEFormFile& form_file) { m_formSpec = form_file; }

    CDEForm* GetForm(int i)   { return m_formSpec.GetForm(i); }
    CDEForm* GetCurForm()     { return m_formSpec.GetForm(m_iCurFormIndex); }

    CDELevel* GetLevel(int i) { return m_formSpec.GetLevel(i); }

    int GetCurFormIndex() const { return m_iCurFormIndex; }
    void SetCurFormIndex(int i) { m_iCurFormIndex = i; }

    void ReleaseDicts();

    CFormTreeCtrl* GetFormTreeCtrl() const         { return m_pFormTreeCtrl; }
    void SetFormTreeCtrl(CFormTreeCtrl* pTreeCtrl) { m_pFormTreeCtrl = pTreeCtrl; }

    HTREEITEM BuildAllTrees(HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);

    bool LoadFormSpecFile(const CString& csFileName);
    bool LoadDictSpecFile(bool bMakeVisible = true);

    bool InitTreeCtrl();

    CView* GetView(FormViewType form_view_type = FormViewType::Form);

//  following funcs facilitate generating default .fmf file from an existing dict
    void GenerateFormFile();

    void SaveAllDictionaries();
    BOOL IsFormModified();

    // undo/redo stack stuff
    UndoStack<FormUndoStack>& GetUndoStack() { return m_undoStack; }
    void PushUndo(FormUndoStack form_undo_stack);
    void UndoChange(bool bCanRedo);
    void RedoChange();

    CapiEditorViewModel& GetCapiEditorViewModel() { return m_capi_editor_view_model; }

    void SetSelectedCapiQuestion(CFormID* form_id);

    CapiQuestionManager* GetCapiQuestionManager()                       { return m_question_manager.get(); }
    std::shared_ptr<CapiQuestionManager> GetSharedCapiQuestionManager() { return m_question_manager; }

    void SetCapiQuestionManager(Application* application, std::shared_ptr<CapiQuestionManager> question_manager);

    // DictionaryBasedDoc overrides
    CTreeCtrl* GetTreeCtrl() override;
    std::shared_ptr<const CDataDict> GetSharedDictionary() const override;

public:
    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
    void DeleteContents() override;
    BOOL SaveModified() override;
    void OnCloseDocument() override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnGenerateFrm();

private:
    void PerformUndoChange(FormUndoStack& form_undo_stack);
    bool PerformUndoDelete(CFormUndoObj* pUndoObj);
    bool PerformUndoAdd(CFormUndoObj* pUndoObj);
    bool PerformUndoMove(CFormUndoObj* pUndoObj);
    bool PerformUndoModify(CFormUndoObj* pUndoObj);

private:
    CDEFormFile& m_formSpec; // smg: deal w/later; shld hold all info on all I/O dicts used
    bool m_bFormLoaded;

    CFormTreeCtrl* m_pFormTreeCtrl;

    CString m_csClipFile;                 // File name for temporary clipbord data
    UndoStack<FormUndoStack> m_undoStack; // Form Designer's undo/redo stacks
    UINT m_auFormat[6];                   // Array for clipboard formats

    int m_iCurFormIndex;

    CapiEditorViewModel m_capi_editor_view_model;
    std::shared_ptr<CapiQuestionManager> m_question_manager;
};
