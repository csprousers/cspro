#include "StdAfx.h"
#include "UniverseDlg.h"
#include "DictionaryTreeCtrl.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/Special.h>
#include <zEdit2O/LogicCtrl.h>
#include <zDictO/ValueSetResponse.h>


IMPLEMENT_DYNAMIC(UniverseDlg, CDialog)

BEGIN_MESSAGE_MAP(UniverseDlg, CDialog)
    ON_MESSAGE(UWM::Interface::FocusOnUniverse, OnFocusOnUniverse)
    ON_LBN_DBLCLK(IDC_ITEM_VALUES, OnItemValuesDoubleClick)
    ON_BN_CLICKED(IDC_CLEAR, OnClearButtonClicked)
    ON_COMMAND_RANGE(IDC_OPERATOR_EQ, IDC_OPERATOR_NOT, OnOperatorButtonClicked)
    ON_BN_CLICKED(IDC_OPERATOR_PARENTHESES, OnParenthesesButtonClicked)
END_MESSAGE_MAP()


UniverseDlg::UniverseDlg(std::shared_ptr<const CDataDict> dictionary, std::wstring universe, ActionResponder& action_responder, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_UNIVERSE, pParent),
        m_dictionary(std::move(dictionary)),
        m_universe(std::move(universe)),
        m_actionResponder(action_responder),
        m_dictionaryTreeCtrl(std::make_unique<DictionaryTreeCtrl>()),
        m_universeLogicCtrl(std::make_unique<CLogicCtrl>())
{
    ASSERT(m_dictionary != nullptr);
}


UniverseDlg::~UniverseDlg()
{
}


void UniverseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DICTIONARY_TREE, *m_dictionaryTreeCtrl);
    DDX_Control(pDX, IDC_ITEM_VALUES, m_itemValues);
    DDX_Control(pDX, IDC_UNIVERSE_TEXT, *m_universeLogicCtrl);
}


BOOL UniverseDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_dictionaryTreeCtrl->Initialize(m_dictionary, true, true, this);

    m_universeLogicCtrl->ReplaceCEdit(this, false, false);
    m_universeLogicCtrl->SetText(m_universe);
    PostMessage(UWM::Interface::FocusOnUniverse, m_universeLogicCtrl->GetTextLength());

    return TRUE;
}


BOOL UniverseDlg::PreTranslateMessage(MSG* pMsg)
{
    // override Ctrl+T (to toggle the dictionary tree control) and Ctrl+K to compile the univesre
    if( pMsg->message == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0 )
    {
        const int key = std::toupper(pMsg->wParam);

        if( key == 'T' )
        {
            m_actionResponder.ToggleNamesInTree();
            m_dictionaryTreeCtrl->Invalidate();
            return TRUE;
        }

        else if( key == 'K' )
        {
            m_actionResponder.CheckSyntax(m_universeLogicCtrl->GetText());
            return TRUE;
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


void UniverseDlg::OnOK()
{
    m_universe = m_universeLogicCtrl->GetText();

    if( m_actionResponder.CheckSyntax(m_universe) )
        CDialog::OnOK();
}


LRESULT UniverseDlg::OnFocusOnUniverse(WPARAM wParam, LPARAM /*lParam*/)
{
    if( wParam >= 0 )
        m_universeLogicCtrl->GotoPos(wParam);

    m_universeLogicCtrl->SetFocus();

    return 0;
}


void UniverseDlg::OnDictionaryTreeDoubleClick(const DictionaryTreeNode& dictionary_tree_node)
{
    // only insert item names
    const CDictItem* dict_item = dictionary_tree_node.GetAssociatedDictItem();

    if( dict_item != nullptr )
        InsertTextToUniverse(dictionary_tree_node.GetLogicName());
}


void UniverseDlg::OnDictionaryTreeSelectionChanged(const DictionaryTreeNode& dictionary_tree_node)
{
    // determine what value set to show
    const CDictItem* dict_item = dictionary_tree_node.GetAssociatedDictItem();
    const DictValueSet* dict_value_set = nullptr;

    if( dictionary_tree_node.GetDictElementType() == DictElementType::Item )
    {
        dict_value_set = dict_item->GetFirstValueSetOrNull();
    }

    else if( dictionary_tree_node.GetDictElementType() == DictElementType::ValueSet )
    {
        dict_value_set = &dictionary_tree_node.GetElement<DictValueSet>();
    }

    // show the values from the value set
    m_itemValues.SetRedraw(false);

    m_itemValues.ResetContent();
    m_logicForItemValues.clear();

    if( dict_value_set != nullptr )
    {
        for( const DictValue& dict_value : dict_value_set->GetValues() )
        {
            std::wstring label = CS2WS(dict_value.GetLabel());
            std::wstring values;
            std::wstring& logic_for_values = m_logicForItemValues.emplace_back();

            if( label.empty() )
                label = _T("<No Label>");

            for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
            {
                if( dict_item->GetContentType() == ContentType::Numeric )
                {
                    ValueSetResponse value_set_response(*dict_item, dict_value, dict_value_pair);
                    std::wstring this_value;

                    if( IsSpecial(value_set_response.GetMinimumValue()) )
                    {
                        this_value = SpecialValues::ValueToString(value_set_response.GetMinimumValue());
                    }

                    else
                    {
                        this_value = ValueSetResponse::FormatValueForDisplay(*dict_item, value_set_response.GetMinimumValue());

                        if( !value_set_response.IsDiscrete() )
                        {
                            this_value.push_back(':');
                            this_value.append(ValueSetResponse::FormatValueForDisplay(*dict_item, value_set_response.GetMaximumValue()));
                        }
                    }

                    SO::AppendWithSeparator(values, this_value, ',');

                    SO::MakeLower(this_value);
                    SO::AppendWithSeparator(logic_for_values, this_value, ',');
                }

                else if( dict_item->GetContentType() == ContentType::Alpha )
                {
                    std::wstring trimmed_value = SO::TrimRight(dict_value_pair.GetFrom());

                    SO::AppendWithSeparator(values, trimmed_value, ',');
                    
                    // wrap the string in quotes
                    const std::wstring escaped_label = Encoders::ToLogicString(std::move(trimmed_value));
                    SO::AppendWithSeparator(logic_for_values, escaped_label, ',');
                }

                else
                {
                    ASSERT(false);
                }
            }

            // a tab character is inserted to avoid issues with the brackets being placed in weird places when using Arabic;
            // the tab character seems to not show on the screen but ends up displaying the text properly
            m_itemValues.AddString(FormatText(_T("%s\t [%s]"), label.c_str(), values.c_str()));
        }
    }

    m_itemValues.SetRedraw(true);
    m_itemValues.Invalidate();
}


void UniverseDlg::OnItemValuesDoubleClick()
{
    const size_t selection_index = static_cast<size_t>(m_itemValues.GetCurSel());

    if( selection_index < m_logicForItemValues.size() )
        InsertTextToUniverse(m_logicForItemValues[selection_index]);
}


void UniverseDlg::OnClearButtonClicked()
{
    m_universeLogicCtrl->ClearAll();
    PostMessage(UWM::Interface::FocusOnUniverse);
}


void UniverseDlg::OnOperatorButtonClicked(const UINT nID)
{
    InsertTextToUniverse(( nID == IDC_OPERATOR_EQ )     ? _T("=") :
                         ( nID == IDC_OPERATOR_NOT_EQ ) ? _T("<>") :
                         ( nID == IDC_OPERATOR_LT )     ? _T("<") :
                         ( nID == IDC_OPERATOR_LTE )    ? _T("<=") :
                         ( nID == IDC_OPERATOR_GTE )    ? _T(">=") :
                         ( nID == IDC_OPERATOR_GT )     ? _T(">") :
                         ( nID == IDC_OPERATOR_IN )     ? _T("in") :
                         ( nID == IDC_OPERATOR_AND )    ? _T("and") :
                         ( nID == IDC_OPERATOR_OR )     ? _T("or") :
                       /*( nID == IDC_OPERATOR_NOT )*/    _T("not"));
}


void UniverseDlg::OnParenthesesButtonClicked()
{
    const Sci_Position selection_start = m_universeLogicCtrl->GetSelectionStart();
    const Sci_Position selection_end = m_universeLogicCtrl->GetSelectionEnd();
    const Sci_Position new_selection_end = selection_end + 2;
    const bool reselect_range = ( selection_start != selection_end );

    const std::wstring new_text = FormatTextCS2WS(_T("(%s)"), m_universeLogicCtrl->GetSelText().c_str());
    m_universeLogicCtrl->ReplaceSel(new_text);

    // select the expanded selection if parentheses were placed around a selection
    if( reselect_range )
        m_universeLogicCtrl->SetSel(selection_start, new_selection_end);

    PostMessage(UWM::Interface::FocusOnUniverse, reselect_range ? -1 : new_selection_end);
}


void UniverseDlg::InsertTextToUniverse(std::wstring text)
{
    const Sci_Position selection_start = m_universeLogicCtrl->GetSelectionStart();
    const Sci_Position selection_end = m_universeLogicCtrl->GetSelectionEnd();

    // insert spaces if the caret is not against writespace
    if( selection_start > 0 && !std::iswspace(static_cast<wchar_t>(m_universeLogicCtrl->GetCharAt(selection_start - 1))) )
        text.insert(text.begin(), ' ');

    if( selection_end < m_universeLogicCtrl->GetTextLength() && !std::iswspace(static_cast<wchar_t>(m_universeLogicCtrl->GetCharAt(selection_end + 1))) )
        text.push_back(' ');

    m_universeLogicCtrl->ReplaceSel(text);

    PostMessage(UWM::Interface::FocusOnUniverse, selection_start + text.length());
}
