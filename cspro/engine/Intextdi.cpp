//---------------------------------------------------------------------------
//
// INTEXTDI.cpp : external dictionary functions
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "INTERPRE.H"
#include "ProgramControl.h"
#include "SelcaseManager.h"
#include "SelectDlgHelper.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Dictionaries.h>
#include <zEngineO/Nodes/File.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/ConnectionString.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zParadataO/Logger.h>
#include <ZBRIDGEO/npff.h>
#include <zCapiO/SelectCtrl.h>


// this function evaluates the parameters to the loadcase/delcase functions; it returns true if:
// - there were no parameters
// - the length of the parameters was equal to the key length
// - the length of the parameters was less than the key length (in which case the parameters will be expanded to fit the key length)
template<typename T>
bool CIntDriver::EvaluateCaseFunctionParameters(const CDataDict& dictionary, const T& key_arguments_list_node_or_function_node, std::wstring& key)
{
    ASSERT(key.empty());

    int number_arguments;
    const int* arguments;

    if constexpr(std::is_same_v<T, int>)
    {
        const Nodes::List& key_arguments_list_node = GetListNode(key_arguments_list_node_or_function_node);
        number_arguments = key_arguments_list_node.number_elements;
        arguments = key_arguments_list_node.elements;
    }

    else
    {
        number_arguments = key_arguments_list_node_or_function_node.number_arguments - 1;
        arguments = key_arguments_list_node_or_function_node.arguments + 1;
    }

    // if no arguments were used, quit out
    if( number_arguments == 0 )
        return true;

    size_t key_length = dictionary.GetKeyLength();

    // initalize the key with spaces so that if the provided key is too short,
    // is will appear as space filled to match the key length
    key.resize(key_length, ' ');

    TCHAR* key_buffer = key.data();
    size_t current_length = 0;

    double dIndex[DIM_MAXDIM];
    int aIndex[DIM_MAXDIM];

    for( int i = 0; i < number_arguments; ++i )
    {
        // string expression
        if( arguments[i] < 0 )
        {
            std::wstring key_component = EvalAlphaExpr(-1 * arguments[i]);

            if( ( current_length + key_component.length() ) > key_length )
                return false;

            _tmemcpy(key_buffer + current_length, key_component.data(), key_component.length());

            current_length += key_component.length();
        }

        // dictionary variable (or a working variable in pre-7.4 .pen files)
        else
        {
            SVAR_NODE* psvar = (SVAR_NODE*)PPT(arguments[i]);
            MVAR_NODE* pmvar = (MVAR_NODE*)PPT(arguments[i]);

            bool bIsSvar = ( psvar->m_iVarType == SVAR_CODE );
            int iThisVar = bIsSvar ? psvar->m_iVarIndex : pmvar->m_iVarIndex;

            VART* pVarT = VPT(iThisVar);
            VARX* pVarX = pVarT->GetVarX();

            int iThisParmLength = pVarT->GetLogicStringPtr() ? pVarT->GetLogicStringPtr()->GetLength() : pVarT->GetLength();

            if( ( current_length + iThisParmLength ) > key_length )
                return false;

            if( bIsSvar )
            {
                for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
                    dIndex[iDim] = 0;
            }

            else
            {
                ASSERT(psvar->m_iVarType == MVAR_CODE);
                mvarGetSubindexes(pmvar, dIndex);
            }

            for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
                aIndex[iDim] = 0;

            pVarX->RemapIndexes(aIndex,dIndex);

            if( pVarT->IsNumeric() )
            {
                double dValue = 0;

                if( bIsSvar )
                {
                    dValue = pVarX->varoutval();
                }

                else
                {
                    CNDIndexes theIndex(ZERO_BASED,aIndex);
                    dValue = pVarX->varoutval(theIndex);
                }

                pVarT->dvaltochar(dValue, key_buffer + current_length);
            }

            else
            {
                TCHAR* pszValue = nullptr;

                if( bIsSvar )
                {
                    pszValue = pVarT->GetLogicStringPtr() ? pVarT->GetLogicStringPtr()->GetBuffer() : (TCHAR*)svaraddr(pVarX);
                }

                else
                {
                    pszValue = (TCHAR*)mvaraddr(pVarX,dIndex);
                }

                ASSERT(pszValue != nullptr);

                _tmemcpy(key_buffer + current_length, pszValue, iThisParmLength);
            }

            current_length += iThisParmLength;
        }
    }

    ASSERT(key.length() == key_length);

    return true;
}


//----------------------------------------------------------------------
//
//  exloadcase      ejecuta 'LOADCASE'
//
//----------------------------------------------------------------------
double CIntDriver::exloadcase(int program_index)
{
    const auto& case_io_node = GetNode<Nodes::CaseIO>(program_index);
    Symbol& symbol = NPT_Ref(case_io_node.data_repository_dictionary_symbol_index);

    if( !symbol.IsA(SymbolType::Dictionary) )
        return exloadcase_pre80(program_index);

    EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
    EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();

    EngineCase& engine_case = ( case_io_node.case_dictionary_symbol_index == -1 ) ?
        engine_dictionary.GetEngineCase() :
        GetSymbolEngineDictionary(case_io_node.case_dictionary_symbol_index).GetEngineCase();

    std::wstring loadcase_key;

    if( !EvaluateCaseFunctionParameters(engine_dictionary.GetDictionary(), case_io_node.key_arguments_list_node, loadcase_key) )
    {
        // a key of invalid length was provided
        return 0;
    }

    try
    {
        // if no key is supplied, load the next case from an iterator
        if( loadcase_key.empty() )
        {
            // if no action has occurred yet on the dictionary, load the next case
            if( !engine_data_repository.IsCaseIteratorActive() )
                engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromCurrentPosition);

            if( !engine_data_repository.StepCaseIterator(engine_case) )
                return 0;
        }

        // otherwise load a case based on the key
        else
        {
            // calling loadcase with a key resets any iterator that may exist
            engine_data_repository.StopCaseIterator();

            engine_data_repository.ReadCase(engine_case, WS2CS(loadcase_key));
        }

        // ENGINECR_TODO(loadcase) is there anything in ParseCaseLevel that needs to be copied?  m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(), pDicT);

        return 1;
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exloadcase_pre80(int program_index)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    DICT* pDicT = DPT(va_with_size_node.arguments[0]);
    DICX* pDicX = pDicT->GetDicX();

    std::wstring loadcase_key;

    if( !EvaluateCaseFunctionParameters(*pDicT->GetDataDict(), va_with_size_node, loadcase_key) )
        return 0; // a key of invalid length was provided

    try
    {
        // if no key is supplied, load the next case from an iterator
        if( loadcase_key.empty() )
        {
            // if no action has occurred yet on the dictionary, load the next case
            if( !pDicX->IsCaseIteratorActive() )
                pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromCurrentPosition);

            if( !pDicX->StepCaseIterator() )
                return 0;
        }

        // otherwise load a case based on the key
        else
        {
            // calling loadcase with a key resets any iterator that may exist
            pDicX->StopCaseIterator();

            pDicX->GetDataRepository().ReadCasetainer(pDicX->GetCase(), WS2CS(loadcase_key));
        }

        m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                        &pDicX->GetCase().GetRootCaseLevel(), pDicT);

        return 1;
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
    }

    return 0;
}


//----------------------------------------------------------------------
//
//  exwritecase     ejecuta 'WRITECASE'
//
//----------------------------------------------------------------------
double CIntDriver::exwritecase(int program_index)
{
    const auto& case_io_node = GetNode<Nodes::CaseIO>(program_index);
    Symbol& symbol = NPT_Ref(case_io_node.data_repository_dictionary_symbol_index);

    if( !symbol.IsA(SymbolType::Dictionary) )
        return exwritecase_pre80(program_index);

    EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
    EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();
    DataRepository& data_repository = engine_data_repository.GetDataRepository();

    EngineCase& engine_case = ( case_io_node.case_dictionary_symbol_index == -1 ) ?
        engine_dictionary.GetEngineCase() :
        GetSymbolEngineDictionary(case_io_node.case_dictionary_symbol_index).GetEngineCase();

    // ENGINECR_TODO(writecase) is there anything in CopyLevelToRepository that needs to be copied?  m_pEngineDriver->CopyLevelToRepository(pDicT,&pDicX->GetCase(),pRootLevel);

    /* ENGINECR_TODO(writecase) how to handle the special output key?
    try
    {
        pCase->FinalizeLevel(pRootLevel, true, true, pDicX->GetCase().GetCaseConstructionReporter().get());

        // ENGINECR_TODO(writecase) is there anything in UpdateCaseNotesLevelKeys_pre80 that needs to be copied?  m_pEngineDriver->UpdateCaseNotesLevelKeys_pre80(pDicX);
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        CString csWriteCaseKey = m_pEngineDriver->key_string(pDicT);
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, csWriteCaseKey);
    }*/

    try
    {
        engine_data_repository.ClearLastSearchedKey();
        engine_data_repository.StopCaseIterator();

        // make sure that writes in a loop get processed in a transaction
        GetLoopStack().AddDataTransactionToLoopStack(data_repository);

        // mark the case as not deleted
        Case& data_case = engine_case.GetCase();
        data_case.SetDeleted(false);

        // add any required records
        data_case.AddRequiredRecords(true);

        data_repository.WriteCase(data_case);

        // update the case's key
        engine_case.CalculateInitialCaseKey();

        return 1;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exwritecase_pre80(int program_index)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    DICT* pDicT = DPT(va_with_size_node.arguments[0]);
    DICX* pDicX = pDicT->GetDicX();

    Pre74_Case* pCase = pDicX->GetCase().GetPre74_Case();
    Pre74_CaseLevel* pRootLevel = pCase->GetRootLevel();

    pRootLevel->Reset();
    m_pEngineDriver->CopyLevelToRepository(pDicT,&pDicX->GetCase(),pRootLevel);

    try
    {
        pCase->FinalizeLevel(pRootLevel, true, true, pDicX->GetCase().GetCaseConstructionReporter());

        m_pEngineDriver->UpdateCaseNotesLevelKeys_pre80(pDicX);
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        CString csWriteCaseKey = m_pEngineDriver->key_string(pDicT);
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, csWriteCaseKey);
    }

    try
    {
        pDicX->ClearLastSearchedKey();
        pDicX->StopCaseIterator();

        // make sure that writes in a loop get processed in a transaction
        GetLoopStack().AddDataTransactionToLoopStack(pDicX->GetDataRepository());

        // mark the case as not deleted
        pDicX->GetCase().SetDeleted(false);

        pDicX->GetDataRepository().WriteCasetainer(&pDicX->GetCase());

        // update the current key
        _tcscpy(pDicX->current_key, pDicX->GetCase().GetKey());

        return 1;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning,10104,exception.GetErrorMessage().c_str());
    }

    return 0;
}


//----------------------------------------------------------------------
//
//  exdelcase       ejecuta 'DELCASE'
//
//----------------------------------------------------------------------
double CIntDriver::exdelcase(int program_index)
{
    const auto& case_io_node = GetNode<Nodes::CaseIO>(program_index);
    Symbol& symbol = NPT_Ref(case_io_node.data_repository_dictionary_symbol_index);

    if( !symbol.IsA(SymbolType::Dictionary) )
        return exdelcase_pre80(program_index);

    EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
    EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();
    DataRepository& data_repository = engine_data_repository.GetDataRepository();

    std::wstring delcase_key;

    if( !EvaluateCaseFunctionParameters(engine_dictionary.GetDictionary(), case_io_node.key_arguments_list_node, delcase_key) )
    {
        // a key of invalid length was provided
        return 0;
    }

    std::optional<double> position_in_repository;

    // if no values were provided, then use the ID values in memory
    if( delcase_key.empty() )
    {
        bool using_case_override = ( case_io_node.case_dictionary_symbol_index != -1 &&
                                     case_io_node.case_dictionary_symbol_index != case_io_node.data_repository_dictionary_symbol_index );

        EngineCase& engine_case = using_case_override ? GetSymbolEngineDictionary(case_io_node.case_dictionary_symbol_index).GetEngineCase() :
                                                        engine_dictionary.GetEngineCase();

        delcase_key = engine_case.GetCase().GetKey();

        // if deleting the currently loaded case, use the position in the repository
        // instead of the key, which will allow for deleting specific cases when there
        // are multiple cases with the same key
        if( !using_case_override && engine_data_repository.IsLastSearchedCaseKeyDefined() &&
            SO::Equals(delcase_key, engine_data_repository.GetLastSearchedCaseKey()->GetKey()) )
        {
            position_in_repository = engine_data_repository.GetLastSearchedCaseKey()->GetPositionInRepository();
        }
    }

    try
    {
        engine_data_repository.ClearLastSearchedKey();
        engine_data_repository.StopCaseIterator();

        // make sure that deletes in a loop get processed in a transaction
        GetLoopStack().AddDataTransactionToLoopStack(data_repository);

        // delete the case
        if( position_in_repository.has_value() )
        {
            data_repository.DeleteCase(*position_in_repository);
        }

        else
        {
            data_repository.DeleteCase(WS2CS(delcase_key));
        }

        return 1;
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exdelcase_pre80(int program_index)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    DICT* pDicT = DPT(va_with_size_node.arguments[0]);
    DICX* pDicX = pDicT->GetDicX();

    std::wstring delcase_key;
    std::optional<double> position_in_repository;

    if( !EvaluateCaseFunctionParameters(*pDicT->GetDataDict(), va_with_size_node, delcase_key) )
        return 0; // a key of invalid length was provided

    // if no values were provided, then use the ID values in memory
    if( delcase_key.empty() )
    {
        delcase_key = m_pEngineDriver->key_string(pDicT);

        // if deleting the currently loaded case, use the position in the repository
        // instead of the key, which will allow for deleting specific cases when there
        // are multiple cases with the same key
        if( pDicX->IsLastSearchedCaseKeyDefined() && SO::Equals(delcase_key, pDicX->GetLastSearchedCaseKey()->GetKey()) )
            position_in_repository = pDicX->GetLastSearchedCaseKey()->GetPositionInRepository();
    }

    try
    {
        pDicX->ClearLastSearchedKey();
        pDicX->StopCaseIterator();

        // make sure that deletes in a loop get processed in a transaction
        GetLoopStack().AddDataTransactionToLoopStack(pDicX->GetDataRepository());

        // delete the case
        if( position_in_repository.has_value() )
        {
            pDicX->GetDataRepository().DeleteCase(*position_in_repository);
        }

        else
        {
            pDicX->GetDataRepository().DeleteCase(WS2CS(delcase_key));
        }

        return 1;
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }

    return 0;
}


//----------------------------------------------------------------------
//
//  exfind_locate          ejecuta 'FIND' / 'LOCATE'
//
//----------------------------------------------------------------------
double CIntDriver::exfind_locate(int program_index)
{
    const auto& case_search_node = GetNode<Nodes::CaseSearch>(program_index);
    bool locate_function = ( case_search_node.function_code == FunctionCode::FNLOCATE_CODE );
    Symbol& symbol = NPT_Ref(case_search_node.dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);

        bool valid_operation = true;

        if( engine_dictionary.GetSubType() == SymbolSubType::Input )
        {
            valid_operation = ( !locate_function || Issamod != ModuleType::Entry );
        }

        else if( engine_dictionary.GetSubType() != SymbolSubType::External )
        {
            valid_operation = false;
        }

        if( !valid_operation )
            return 0;

        std::optional<CaseKey> optional_case_key;

        try
        {
            EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();
            DataRepository& data_repository = engine_data_repository.GetDataRepository();

            CString search_key = EvalAlphaExpr<CString>(case_search_node.key_expression);

            // find with equals can be solved easily
            if( !locate_function && case_search_node.operator_code == TOKEQOP )
                return data_repository.ContainsCase(search_key) ? 1 : 0;

            bool search_by_uuid = ( case_search_node.operator_code == Nodes::CaseSearch::SearchByUuidCode );

            // search by UUID or key
            if( search_by_uuid || case_search_node.operator_code == TOKEQOP )
            {
                CString key = search_by_uuid ? CString() : search_key;
                CString uuid = search_by_uuid ? search_key : CString();
                double position_in_repository;

                try
                {
                    data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);
                    optional_case_key.emplace(key, position_in_repository);
                }

                catch( const DataRepositoryException::CaseNotFound& ) { }
            }

            // search by key prefix
            else if( case_search_node.operator_code == Nodes::CaseSearch::SearchByKeyPrefixCode )
            {
                optional_case_key = data_repository.FindCaseKey(CaseIterationMethod::KeyOrder,
                    engine_data_repository.GetDictionaryAccessParameters().case_iteration_order,
                    CaseIteratorParameters { CaseIterationStartType::GreaterThanEquals, CString(), search_key });
            }

            // search by operator
            else
            {
                bool search_is_gte_or_gt = ( case_search_node.operator_code == TOKGEOP || case_search_node.operator_code == TOKGTOP );

                // speed up the common operation of searching for the first case
                if( search_key.IsEmpty() && search_is_gte_or_gt )
                    optional_case_key = data_repository.FindCaseKey(CaseIterationMethod::KeyOrder, CaseIterationOrder::Ascending);

                else
                {
                    CaseIterationStartType iteration_start_type =
                        ( case_search_node.operator_code == TOKLTOP ) ?   CaseIterationStartType::LessThan :
                        ( case_search_node.operator_code == TOKLEOP ) ?   CaseIterationStartType::LessThanEquals :
                        ( case_search_node.operator_code == TOKGEOP ) ?   CaseIterationStartType::GreaterThanEquals :
                      /*( case_search_node.operator_code == TOKGTOP ) ? */CaseIterationStartType::GreaterThan;

                    CaseIterationOrder iteration_order = search_is_gte_or_gt ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending;

                    optional_case_key = data_repository.FindCaseKey(CaseIterationMethod::KeyOrder,
                        iteration_order, CaseIteratorParameters { iteration_start_type, search_key, std::nullopt });
                }
            }

            if( locate_function )
            {
                engine_data_repository.SetLastSearchedCaseKey(optional_case_key);

                if( engine_data_repository.IsLastSearchedCaseKeyDefined() )
                {
                    engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromLastSearchedCaseKey);
                }

                else
                {
                    engine_data_repository.StopCaseIterator();
                }
            }
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return optional_case_key.has_value() ? 1 : 0;
    }

    else
    {
        DICT* pDicT = assert_cast<DICT*>(&symbol);
        DICX* pDicX = pDicT->GetDicX();

        bool valid_operation = true;

        if( pDicT->GetSubType() == SymbolSubType::Input )
        {
            valid_operation = ( !locate_function || Issamod != ModuleType::Entry );
        }

        else if( pDicT->GetSubType() != SymbolSubType::External )
        {
            valid_operation = false;
        }

        if( !valid_operation )
            return 0;

        std::optional<CaseKey> optional_case_key;

        try
        {
            CString search_key = EvalAlphaExpr<CString>(case_search_node.key_expression);

            // find with equals can be solved easily
            if( !locate_function && case_search_node.operator_code == TOKEQOP )
                return pDicX->GetDataRepository().ContainsCase(search_key) ? 1 : 0;

            bool search_by_uuid = ( case_search_node.operator_code == Nodes::CaseSearch::SearchByUuidCode );

            // search by UUID or key
            if( search_by_uuid || case_search_node.operator_code == TOKEQOP )
            {
                CString key = search_by_uuid ? CString() : search_key;
                CString uuid = search_by_uuid ? search_key : CString();
                double position_in_repository;

                try
                {
                    pDicX->GetDataRepository().PopulateCaseIdentifiers(key, uuid, position_in_repository);
                    optional_case_key.emplace(key, position_in_repository);
                }

                catch( const DataRepositoryException::CaseNotFound& ) { }
            }

            // search by key prefix
            else if( case_search_node.operator_code == Nodes::CaseSearch::SearchByKeyPrefixCode )
            {
                optional_case_key = pDicX->GetDataRepository().FindCaseKey(CaseIterationMethod::KeyOrder,
                    pDicX->GetCaseIterationOrder(), CaseIteratorParameters { CaseIterationStartType::GreaterThanEquals, CString(), search_key });
            }

            // search by operator
            else
            {
                bool search_is_gte_or_gt = ( case_search_node.operator_code == TOKGEOP || case_search_node.operator_code == TOKGTOP );

                // speed up the common operation of searching for the first case
                if( search_key.IsEmpty() && search_is_gte_or_gt )
                    optional_case_key = pDicX->GetDataRepository().FindCaseKey(CaseIterationMethod::KeyOrder, CaseIterationOrder::Ascending);

                else
                {
                    CaseIterationStartType iteration_start_type =
                        ( case_search_node.operator_code == TOKLTOP ) ?   CaseIterationStartType::LessThan :
                        ( case_search_node.operator_code == TOKLEOP ) ?   CaseIterationStartType::LessThanEquals :
                        ( case_search_node.operator_code == TOKGEOP ) ?   CaseIterationStartType::GreaterThanEquals :
                      /*( case_search_node.operator_code == TOKGTOP ) ? */CaseIterationStartType::GreaterThan;

                    CaseIterationOrder iteration_order = search_is_gte_or_gt ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending;

                    optional_case_key = pDicX->GetDataRepository().FindCaseKey(CaseIterationMethod::KeyOrder,
                        iteration_order, CaseIteratorParameters { iteration_start_type, search_key, std::nullopt });
                }
            }

            if( locate_function )
            {
                pDicX->SetLastSearchedCaseKey(optional_case_key);

                if( pDicX->IsLastSearchedCaseKeyDefined() )
                    pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromLastSearchedCaseKey);

                else
                    pDicX->StopCaseIterator();
            }
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return optional_case_key.has_value() ? 1 : 0;
    }
}


//----------------------------------------------------------------------
//
//  exretrieve      ejecuta 'RETRIEVE'
//
//----------------------------------------------------------------------
double CIntDriver::exretrieve(int iExpr)
{
    const auto& retrieve_node = GetNode<Nodes::Retrieve>(iExpr);
    Symbol& symbol = NPT_Ref(retrieve_node.data_repository_dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();

        try
        {
            // if no action has occurred yet on the dictionary, have retrieve load the first case
            if( !engine_data_repository.IsLastSearchedCaseKeyDefined() )
            {
                engine_data_repository.SetLastSearchedCaseKey(engine_data_repository.GetDataRepository().FindCaseKey(
                    engine_data_repository.GetDictionaryAccessParameters().case_iteration_method, CaseIterationOrder::Ascending));
            }

            if( engine_data_repository.IsLastSearchedCaseKeyDefined() )
            {
                engine_data_repository.StopCaseIterator();

                EngineCase& engine_case = GetSymbolEngineDictionary(retrieve_node.case_dictionary_symbol_index).GetEngineCase();

                engine_data_repository.ReadCase(engine_case, engine_data_repository.GetLastSearchedCaseKey()->GetPositionInRepository());

                // ENGINECR_TODO(retrieve) is there anything in ParseCaseLevel that needs to be copied?  m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(), pDicT);

                return 1;
            }
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return 0;
    }

    else
    {
        DICT* pDicT = assert_cast<DICT*>(&symbol);
        DICX* pDicX = pDicT->GetDicX();

        try
        {
            // if no action has occurred yet on the dictionary, have retrieve load the first case
            if( !pDicX->IsLastSearchedCaseKeyDefined() )
                pDicX->SetLastSearchedCaseKey(pDicX->GetDataRepository().FindCaseKey(pDicX->GetCaseIterationMethod(), CaseIterationOrder::Ascending));

            if( pDicX->IsLastSearchedCaseKeyDefined() )
            {
                pDicX->StopCaseIterator();

                pDicX->GetDataRepository().ReadCasetainer(pDicX->GetCase(), pDicX->GetLastSearchedCaseKey()->GetPositionInRepository());

                m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                &pDicX->GetCase().GetRootCaseLevel(), pDicT);

                return 1;
            }
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return 0;
    }
}


//----------------------------------------------------------------------
//
//  exdictaccess        ejecuta 'SET ACCESS', 'SET FIRST', 'SET LAST'
//
//----------------------------------------------------------------------
double CIntDriver::exdictaccess(int program_index)
{
    const auto& set_access_first_last_node = GetNode<Nodes::SetAccessFirstLast>(program_index);

    return exdictaccess(set_access_first_last_node);
}


double CIntDriver::exdictaccess(const Nodes::SetAccessFirstLast& set_access_first_last_node)
{
    ASSERT(set_access_first_last_node.function_code == FunctionCode::SET_DICT_ACCESS_CODE);

    Symbol& symbol = NPT_Ref(set_access_first_last_node.dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();

        try
        {
            // set access
            if( set_access_first_last_node.set_action == SetAction::Access )
            {
                engine_data_repository.SetDictionaryAccessParameters(set_access_first_last_node.dictionary_access);

                // restart the iterator from the current position
                engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromCurrentPosition);
            }

            // set first / set last
            else
            {
                bool set_first = ( set_access_first_last_node.set_action == SetAction::First );
                ASSERT(set_first || set_access_first_last_node.set_action == SetAction::Last);

                std::optional<CaseKey> case_key = engine_data_repository.GetDataRepository().FindCaseKey(
                    engine_data_repository.GetDictionaryAccessParameters().case_iteration_method,
                    set_first ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending);

                engine_data_repository.SetLastSearchedCaseKey(case_key);

                if( engine_data_repository.IsLastSearchedCaseKeyDefined() )
                    engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromLastSearchedCaseKey);
            }

            return true;
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return false;
    }

    else
    {
        DICT* pDicT = static_cast<DICT*>(&symbol);
        DICX* pDicX = pDicT->GetDicX();

        try
        {
            // set access
            if( set_access_first_last_node.set_action == SetAction::Access )
            {
                // pre-7.4 compilation only has the order set
                if( set_access_first_last_node.dictionary_access <= 1 )
                {
                    pDicX->SetCaseIterationOrder(( set_access_first_last_node.dictionary_access == 1 ) ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending);
                }

                else
                {
                    auto dictionary_access_parameters = pDicX->GetDictionaryAccessParameters(set_access_first_last_node.dictionary_access);
                    pDicX->SetCaseIterationMethod(std::get<0>(dictionary_access_parameters));
                    pDicX->SetCaseIterationOrder(std::get<1>(dictionary_access_parameters));
                    pDicX->SetCaseIterationCaseStatus(std::get<2>(dictionary_access_parameters));
                }

                // restart the iterator from the current position
                pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromCurrentPosition);
            }

            // set first / set last
            else
            {
                bool set_first = ( set_access_first_last_node.set_action == SetAction::First );
                ASSERT(set_first || set_access_first_last_node.set_action == SetAction::Last);

                pDicX->SetLastSearchedCaseKey(pDicX->GetDataRepository().FindCaseKey(pDicX->GetCaseIterationMethod(),
                    set_first ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending));

                if( pDicX->IsLastSearchedCaseKeyDefined() )
                    pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromLastSearchedCaseKey);
            }

            return true;
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return false;
    }
}


//----------------------------------------------------------------------
//
//  exkey           ejecuta funcion 'KEY'
//
//----------------------------------------------------------------------
double CIntDriver::exkey(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    const Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
        const EngineCase& engine_case = engine_dictionary->GetEngineCase();

        if( fn8_node.function_code == FunctionCode::FNKEY_CODE )
        {
            const std::optional<CaseKey>& initial_case_key = engine_case.GetInitialCaseKey();
            return initial_case_key.has_value() ? AssignAlphaValue(initial_case_key->GetKey()) :
                                                  AssignBlankAlphaValue();
        }

        else
        {
            ASSERT(fn8_node.function_code == FunctionCode::FNCURRENTKEY_CODE); // ENGINECR_TODO(currentkey) test once the IDs are modifiable
            return AssignAlphaValue(engine_case.GetCase().GetKey());
        }
    }

    else
    {
        const DICX* pDicX = DPX(fn8_node.symbol_index);

        if( fn8_node.function_code == FunctionCode::FNKEY_CODE )
            return AssignAlphaValue(pDicX->GetCase().GetPre74_Case()->GetKey());

        else
            return AssignAlphaValue(m_pEngineDriver->key_string(pDicX->GetDicT()));
    }
}


//--------------------------------------------------------
//
//  exkeylist
//
//--------------------------------------------------------
double CIntDriver::exkeylist(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary->GetEngineDataRepository();
        DataRepository& data_repository = engine_data_repository.GetDataRepository();
        DictionaryAccessParameters dictionary_access_parameters = engine_data_repository.GetDictionaryAccessParameters(fn8_node.dictionary_access);
        std::unique_ptr<CaseIteratorParameters> start_parameters;

        if( fn8_node.starts_with_expression != -1 )
            start_parameters = std::make_unique<CaseIteratorParameters>(CaseIterationStartType::GreaterThanEquals, CString(), EvalAlphaExpr<CString>(fn8_node.starts_with_expression));

        try
        {
            // if they don't specify a list, return the number of cases
            if( fn8_node.extra_parameter < 0 )
                return data_repository.GetNumberCases(dictionary_access_parameters.case_iteration_status, start_parameters.get());

            // fill in the list
            LogicList& logic_list = GetSymbolLogicList(fn8_node.extra_parameter);

            if( logic_list.IsReadOnly() )
            {
                issaerror(MessageType::Error, 965, logic_list.GetName().c_str());
                return DEFAULT;
            }

            logic_list.Reset();

            CaseKey case_key;
            auto case_key_iterator = data_repository.CreateIterator(CaseIterationContent::CaseKey,
                dictionary_access_parameters.case_iteration_status, dictionary_access_parameters.case_iteration_method,
                dictionary_access_parameters.case_iteration_order, start_parameters.get());

            while( case_key_iterator->NextCaseKey(case_key) )
                logic_list.AddString(CS2WS(case_key.GetKey()));

            return logic_list.GetCount();
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return DEFAULT;
    }

    else
    {
        DICT* pDicT = assert_cast<DICT*>(symbol);
        DICX* pDicX = pDicT->GetDicX();
        auto dictionary_access_parameters = pDicX->GetDictionaryAccessParameters(fn8_node.dictionary_access);
        std::unique_ptr<CaseIteratorParameters> start_parameters;

        if( fn8_node.starts_with_expression != -1 )
            start_parameters = std::make_unique<CaseIteratorParameters>(CaseIterationStartType::GreaterThanEquals, CString(), EvalAlphaExpr<CString>(fn8_node.starts_with_expression));

        try
        {
            // if they don't specify a list, return the number of cases
            if( fn8_node.extra_parameter < 0 )
                return pDicX->GetDataRepository().GetNumberCases(std::get<2>(dictionary_access_parameters), start_parameters.get());

            // fill in the list
            LogicList& logic_list = GetSymbolLogicList(fn8_node.extra_parameter);

            if( logic_list.IsReadOnly() )
            {
                issaerror(MessageType::Error, 965, logic_list.GetName().c_str());
                return DEFAULT;
            }

            logic_list.Reset();

            CaseKey case_key;
            auto case_key_iterator = pDicX->GetDataRepository().CreateIterator(CaseIterationContent::CaseKey,
                std::get<2>(dictionary_access_parameters), std::get<0>(dictionary_access_parameters),
                std::get<1>(dictionary_access_parameters), start_parameters.get());

            while( case_key_iterator->NextCaseKey(case_key) )
                logic_list.AddString(CS2WS(case_key.GetKey()));

            return logic_list.GetCount();
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return DEFAULT;
    }
}


//----------------------------------------------------------------------
//
//  exselcase        ejecuta 'SELCASE'
//
//----------------------------------------------------------------------
double CIntDriver::exselcase(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exselcase_pre77(iExpr);

    const auto& selcase_node = GetNode<FNS_NODE>(iExpr);

    DICT* pDicT = DPT(selcase_node.idict);
    DICX* pDicX = pDicT->GetDicX();

    SelcaseMarkType mark_type = (SelcaseMarkType)selcase_node.mark_type;
    const bool need_to_load_case = ( selcase_node.include_vars[0] >= 0 || selcase_node.fn_where_exp >= 0 );
    const int key_display_offset = ( selcase_node.wind_offs >= 0 ) ? std::max(0, evalexpr<int>(selcase_node.wind_offs)) : 0;
    std::unique_ptr<SelectDlg> select_dlg;

    // initialize the dialog
    if( mark_type != SelcaseMarkType::MultipleAutomark )
    {
        const bool single_selection = ( mark_type == SelcaseMarkType::None );

        // determine the number of columns; there will be at least one (the key)
        size_t number_columns = 1;

        for( int i = 0; i < FNSEL_VARS && selcase_node.include_vars[i] >= 0; ++i )
            ++number_columns;

        select_dlg = std::make_unique<SelectDlg>(single_selection, number_columns);

        select_dlg->SetTitle(( selcase_node.heading >= 0 ) ? EvalAlphaExpr(selcase_node.heading) :
                                                             _T("Select Case"));
    }

    try
    {
        constexpr DICX::CaseIteratorStyle case_iterator_style = DICX::CaseIteratorStyle::FromBoundary;
        std::optional<CString> starting_key = EvalAlphaExpr<CString>(selcase_node.key_expr);

        if( starting_key->IsEmpty() )
            starting_key.reset();

        SelcaseDictionaryManager* dictionary_manager = nullptr;

        if( mark_type != SelcaseMarkType::None )
        {
            if( m_selcaseManager == nullptr )
                m_selcaseManager = std::make_unique<SelcaseManager>();

            dictionary_manager = &m_selcaseManager->GetDictionaryManager(selcase_node.idict);
            dictionary_manager->Reset();
        }

        std::vector<double> positions_in_repository;

        // iterate through the cases; if not needing to load the case contents, a CaseKey iterator can be used, which will be much faster
        CaseKey case_key;
        CaseKey& case_or_case_key = need_to_load_case ? pDicX->GetCase() : case_key;

        pDicX->CreateCaseIterator(case_iterator_style, std::nullopt, selcase_node.dictionary_access, starting_key,
                                  need_to_load_case ? CaseIterationContent::Case : CaseIterationContent::CaseKey);

        auto step_iterator = [&]() -> bool { return need_to_load_case ? pDicX->StepCaseIterator() : pDicX->StepCaseKeyIterator(case_key); };

        while( step_iterator() )
        {
            CString formatted_key = case_or_case_key.GetKey();

            if( key_display_offset > 0 )
            {
                if( key_display_offset < formatted_key.GetLength() )
                {
                    formatted_key = formatted_key.Mid(key_display_offset);
                }

                else
                {
                    formatted_key.Empty();
                }
            }

            std::optional<std::vector<std::wstring>> row_data;

            if( need_to_load_case )
            {
                m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                &pDicX->GetCase().GetRootCaseLevel(), pDicT);

                // evaluate the where expression (if necessary)
                if( selcase_node.fn_where_exp >= 0 && ConditionalValueIsFalse(evalexpr(selcase_node.fn_where_exp)) )
                    continue;

                if( select_dlg != nullptr )
                {
                    row_data.emplace({ CS2WS(formatted_key) });
                }

                // load any include variables
                for( int i = 0; i < FNSEL_VARS && selcase_node.include_vars[i] >= 0; ++i )
                {
                    ASSERT(row_data.has_value());

                    VART* pVarT = VPT(selcase_node.include_vars[i]);
                    m_pEngineDriver->prepvar(pVarT, NO_VISUAL_VALUE);

                    // these variables are always singly occurring
                    if( pVarT->IsNumeric() )
                    {
                        const CDictItem* pDictItem = pVarT->GetDictItem();
                        const double* pdValue = GetVarFloatAddr(pVarT->GetVarX());

                        // format the value so that it isn't zero-filled and shows a decimal point (if applicable)
                        std::wstring& text_value = row_data->emplace_back(pDictItem->GetCompleteLen(), '\0');

                        dvaltochar(*pdValue,
                            text_value.data(),
                            pDictItem->GetCompleteLen(),
                            pDictItem->GetDecimal(),
                            false,
                            pDictItem->GetDecimal() > 0);
                    }

                    else
                    {
                        const TCHAR* pszVariableBuffer = GetVarAsciiAddr(pVarT->GetVarX()); // these variables are always singly occurring
                        row_data->emplace_back(pszVariableBuffer, pVarT->GetLength());
                    }
                }
            }

            if( select_dlg != nullptr )
            {
                if( row_data.has_value() )
                {
                    select_dlg->AddRow(std::move(*row_data));
                }

                else
                {
                    // this will occur if the there was no where/include clause
                    select_dlg->AddRow(CS2WS(formatted_key));
                }
            }

            // add the key to the list of cases that met the search criteria
            positions_in_repository.emplace_back(case_or_case_key.GetPositionInRepository());

            // if the number of keys exceeds the number allowed by the select dialog, stop the iteration
            // (because too many key have been added, a runtime error will occur, issued by the select dialog)
            if( positions_in_repository.size() >= MAX_ITEMS && mark_type != SelcaseMarkType::MultipleAutomark )
                break;
        }

        ASSERT(select_dlg == nullptr || select_dlg->GetRows().size() == positions_in_repository.size());

        // return if there was nothing to select
        if( positions_in_repository.empty() )
            return 0;

        // if automark, simply mark all of the keys and return
        if( mark_type == SelcaseMarkType::MultipleAutomark )
        {
            dictionary_manager->Add(positions_in_repository);
            return 1;
        }

        // otherwise, show the selection dialog

        // set up the column names
        std::vector<std::wstring> column_headings { _T("Key") };

        for( int i = 0; i < FNSEL_VARS && selcase_node.include_vars[i] >= 0; ++i )
        {
            const VART* pVarT = VPT(selcase_node.include_vars[i]);
            column_headings.emplace_back(pVarT->GetName());
        }

        ASSERT(column_headings.size() == select_dlg->GetRows().front().column_texts.size());

        select_dlg->SetHeader(std::move(column_headings));


        SelectDlgHelper select_dlg_helper(*this, *select_dlg, Paradata::OperatorSelectionEvent::Source::SelCase);

        if( mark_type == SelcaseMarkType::None )
        {
            const int selection = select_dlg_helper.GetSingleSelection();

            if( selection == 0 )
                return 0;

            pDicX->GetDataRepository().ReadCasetainer(pDicX->GetCase(), positions_in_repository[selection - 1]);
            m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                            &pDicX->GetCase().GetRootCaseLevel(), pDicT);

            return 1;
        }

        else
        {
            const std::optional<std::set<size_t>>& selected_rows = select_dlg_helper.GetMultipleSelections();

            if( !selected_rows.has_value() )
                return 0;

            std::vector<bool> marked_selections(positions_in_repository.size(), false);

            for( size_t selected_row : *selected_rows )
                marked_selections[selected_row] = true;

            dictionary_manager->Add(positions_in_repository, &marked_selections);

            return 1;
        }
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exselcase_pre77(int iExpr)
{
    const auto& function_node = (const FNS_NODE*)PPT(iExpr);

    DICT* pDicT = DPT(function_node->idict);
    DICX* pDicX = pDicT->GetDicX();

    SelcaseMarkType eMarkType = (SelcaseMarkType)function_node->mark_type;
    bool need_to_load_case = ( function_node->include_vars[0] >= 0 || function_node->fn_where_exp >= 0 );
    int iKeyDisplayOffset = 0;
    CString csHeading = _T("Select Case");

    if( function_node->wind_offs >= 0 )
    {
        iKeyDisplayOffset = evalexpr<int>(function_node->wind_offs);
        iKeyDisplayOffset = std::max(0,iKeyDisplayOffset);
    }

    if( function_node->heading >= 0 )
        csHeading = EvalAlphaExpr<CString>(function_node->heading);

    try
    {
        DICX::CaseIteratorStyle case_iterator_style = DICX::CaseIteratorStyle::FromBoundary;
        std::optional<CString> starting_key = EvalAlphaExpr<CString>(function_node->key_expr);

        if( starting_key->IsEmpty() )
            starting_key.reset();

        SelcaseDictionaryManager* dictionary_manager = nullptr;

        if( eMarkType != SelcaseMarkType::None )
        {
            if( m_selcaseManager == nullptr )
                m_selcaseManager = std::make_unique<SelcaseManager>();

            dictionary_manager = &m_selcaseManager->GetDictionaryManager(function_node->idict);
            dictionary_manager->Reset();
        }

        // set up the objects for the SelectDlgHelper
        std::vector<double> positions_in_repository;
        std::vector<std::vector<CString>*>* paData = NULL;

        if( eMarkType != SelcaseMarkType::MultipleAutomark )
            paData = new std::vector<std::vector<CString>*>;

        // iterate through the cases; if not needing to load the case contents, a CaseKey iterator can be used, which will be much faster
        CaseKey case_key;
        CaseKey& case_or_case_key = need_to_load_case ? pDicX->GetCase() : case_key;

        pDicX->CreateCaseIterator(case_iterator_style, std::nullopt, function_node->dictionary_access, starting_key,
            need_to_load_case ? CaseIterationContent::Case : CaseIterationContent::CaseKey);

        auto step_iterator = [&]() -> bool { return need_to_load_case ? pDicX->StepCaseIterator() : pDicX->StepCaseKeyIterator(case_key); };

        while( step_iterator() )
        {
            CString formatted_key = case_or_case_key.GetKey();

            if( iKeyDisplayOffset > 0 )
            {
                if( iKeyDisplayOffset < formatted_key.GetLength() )
                    formatted_key = formatted_key.Mid(iKeyDisplayOffset);

                else
                    formatted_key.Empty();
            }

            std::vector<CString>* paRowData = NULL;

            if( need_to_load_case )
            {
                m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                &pDicX->GetCase().GetRootCaseLevel(), pDicT);

                // evaluate the where expression (if necessary)
                if( function_node->fn_where_exp >= 0 && ConditionalValueIsFalse(evalexpr(function_node->fn_where_exp)) )
                    continue;

                if( paData != NULL )
                {
                    paRowData = new std::vector<CString>;
                    paRowData->push_back(formatted_key);
                }

                // load any include variables
                for( int i = 0; i < FNSEL_VARS && function_node->include_vars[i] >= 0; i++ )
                {
                    int iSymVar = function_node->include_vars[i];
                    VART* pVarT = VPT(iSymVar);

                    m_pEngineDriver->prepvar(pVarT,NO_VISUAL_VALUE);

                    ASSERT(paRowData != NULL);

                    // these variables are always singly occurring
                    if( pVarT->IsNumeric() )
                    {
                        const CDictItem* pDictItem = pVarT->GetDictItem();
                        double* pdValue = GetVarFloatAddr(pVarT->GetVarX());

                        // format the value so that it isn't zero-filled and shows a decimal point (if applicable)
                        CString csValue;
                        dvaltochar(*pdValue,
                            csValue.GetBufferSetLength(pDictItem->GetCompleteLen()),
                            pDictItem->GetCompleteLen(),
                            pDictItem->GetDecimal(),
                            false,
                            pDictItem->GetDecimal() > 0);

                        paRowData->push_back(csValue);
                    }

                    else
                    {
                        TCHAR* pszVariableBuffer = GetVarAsciiAddr(pVarT->GetVarX()); // these variables are always singly occurring

                        CString csValue(pszVariableBuffer,pVarT->GetLength());

                        paRowData->push_back(csValue);
                    }
                }
            }

            if( paData != NULL )
            {
                if( paRowData == NULL ) // this will occur if the there was no where/include clause
                {
                    paRowData = new std::vector<CString>;
                    paRowData->push_back(formatted_key);
                }

                paData->push_back(paRowData);
            }

            // add the key to the list of cases that met the search criteria
            positions_in_repository.push_back(case_or_case_key.GetPositionInRepository());

            // if the number of keys exceeds the number allowed by the select dialog, stop the iteration
            // (because too many key have been added, a runtime error will occur, issued by the select dialog)
            if( positions_in_repository.size() >= MAX_ITEMS && eMarkType != SelcaseMarkType::MultipleAutomark )
                break;
        }

        ASSERT(paData == NULL || paData->size() == positions_in_repository.size());

        // return if there was nothing to select
        if( positions_in_repository.size() == 0 )
        {
            SAFE_DELETE(paData);
            return 0;
        }


        // if automark, simply mark all of the keys
        if( eMarkType == SelcaseMarkType::MultipleAutomark )
            dictionary_manager->Add(positions_in_repository);

        else // otherwise, show the selection dialog
        {
            int iRet = 0;
            std::unique_ptr<std::vector<bool>> marked_selections;

            // set up the column names
            std::vector<CString> aColumnTitles;
            aColumnTitles.push_back(_T("Key"));

            for( int i = 0; i < FNSEL_VARS && function_node->include_vars[i] >= 0; i++ )
            {
                VART* pVarT = VPT(function_node->include_vars[i]);
                aColumnTitles.push_back(WS2CS(pVarT->GetName()));
            }

            ASSERT(aColumnTitles.size() == paData->at(0)->size());

            if( eMarkType == SelcaseMarkType::Multiple )
            {
                marked_selections = std::make_unique<std::vector<bool>>();
                marked_selections->resize(paData->size(), false);
            }

            iRet = SelectDlgHelper_pre77(FNSELCASE_CODE, &csHeading, paData, &aColumnTitles, marked_selections.get(), nullptr);

            for( std::vector<std::vector<CString>*>::size_type i = 0; i < paData->size(); i++ )
                delete paData->at(i);

            delete paData;

            if( iRet == 0 ) // return if the user canceled the dialog
                return 0;

            if( eMarkType == SelcaseMarkType::None )
            {
                pDicX->GetDataRepository().ReadCasetainer(pDicX->GetCase(), positions_in_repository[iRet - 1]);

                m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                &pDicX->GetCase().GetRootCaseLevel(), pDicT);
            }

            else
                dictionary_manager->Add(positions_in_repository, marked_selections.get());
        }

        return 1;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
    }

    return 0;
}


//----------------------------------------------------------------------
//
//  exnmembers       ejecuta 'NMEMBERS'
//
//----------------------------------------------------------------------
double CIntDriver::exnmembers(int iExpr)
{
    const auto& nmembers_node = GetNode<Nodes::NMembers>(iExpr);

    if( m_selcaseManager == nullptr )
        return 0;

    SelcaseDictionaryManager& dictionary_manager = m_selcaseManager->GetDictionaryManager(nmembers_node.dictionary_symbol_index);

    return ( nmembers_node.query_type == SelcaseQueryType::Marked )   ? dictionary_manager.GetNumMarked() :
           ( nmembers_node.query_type == SelcaseQueryType::Unmarked ) ? dictionary_manager.GetNumUnmarked() :
                                                                        dictionary_manager.GetNumAll();
}


//--------------------------------------------------------
//
//  exfor_dict:  for-dict LIST operation
//
//--------------------------------------------------------
double CIntDriver::exfor_dict(int iExpr)
{
    // quit out if there has not been a successful selcase call
    if( m_selcaseManager == nullptr )
        return 0;

    const auto& for_dictionary_node = GetNode<Nodes::ForDictionary>(iExpr);
    DICT* pDicT = DPT(for_dictionary_node.data_repository_dictionary_symbol_index);
    DICX* pDicX = pDicT->GetDicX();

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::ForDictionary, pDicT);
    if( !loop_stack_entry.IsValid() )
        return 0;

    SelcaseDictionaryManager& dictionary_manager = m_selcaseManager->GetDictionaryManager(for_dictionary_node.data_repository_dictionary_symbol_index);

    SelcaseQueryType query_type = static_cast<SelcaseQueryType>(for_dictionary_node.query_type_or_where_expression);
    bool iterate_marked = ( query_type != SelcaseQueryType::Unmarked );
    bool iterate_unmarked = ( query_type != SelcaseQueryType::Marked );

    double position_in_repository;

    dictionary_manager.StartIterator(iterate_marked, iterate_unmarked);

    while( dictionary_manager.GetNextPosition(position_in_repository) )
    {
        try
        {
            pDicX->GetDataRepository().ReadCasetainer(pDicX->GetCase(), position_in_repository);
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
            // it is possible that the repository has changed since the selcase, so skip past any cases not found
            continue;
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
            return 0;
        }

        m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                        &pDicX->GetCase().GetRootCaseLevel(), pDicT);

        // run the logic in the for block
        try
        {
            bool request_issued = ExecuteProgramStatements(for_dictionary_node.block_expression);

            if( request_issued )
            {
                // break out of the current proc
                m_iSkipStmt = TRUE;
                break;
            }
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        if( m_iStopExec )
            break;
    }

    return 0;
}


//--------------------------------------------------------
//
//  exforcase
//
//--------------------------------------------------------
double CIntDriver::exforcase(int iExpr)
{
    const auto& for_dictionary_node = GetNode<Nodes::ForDictionary>(iExpr);
    Symbol& symbol = NPT_Ref(for_dictionary_node.data_repository_dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& data_repository_engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = data_repository_engine_dictionary.GetEngineDataRepository();

        LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::ForCase, &data_repository_engine_dictionary);
        if( !loop_stack_entry.IsValid() )
            return 0;

        EngineCase& engine_case = GetSymbolEngineDictionary(for_dictionary_node.case_dictionary_symbol_index).GetEngineCase();

        try
        {
            std::optional<CString> key_prefix;
            bool use_where_expression = ( for_dictionary_node.query_type_or_where_expression != -1 );

            if( for_dictionary_node.starts_with_expression != -1 )
                key_prefix = EvalAlphaExpr<CString>(for_dictionary_node.starts_with_expression);

            engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromBoundary,
                std::nullopt, for_dictionary_node.dictionary_access, key_prefix);

            while( engine_data_repository.StepCaseIterator(engine_case) )
            {
                std::optional<CaseKey> this_case_key = engine_data_repository.GetLastLoadedCaseKey();
                ASSERT(this_case_key.has_value());

                // ENGINECR_TODO(forcase) is there anything in ParseCaseLevel that needs to be copied? m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(), pDicT);

                if( use_where_expression && ConditionalValueIsFalse(evalexpr(for_dictionary_node.query_type_or_where_expression)) )
                    continue;

                try
                {
                    // run the logic in the for block
                    bool request_issued = ExecuteProgramStatements(for_dictionary_node.block_expression);

                    if( request_issued )
                    {
                        // break out of the current proc
                        m_iSkipStmt = TRUE;
                        break;
                    }
                }

                catch( const NextProgramControlException& )  { }
                catch( const BreakProgramControlException& ) { break; }

                if( m_iStopExec )
                    break;

                // a delcase or writecase call may stop the iterator, in which case we will start it
                // again at the case following the one loaded in this iteration of the loop
                if( !engine_data_repository.IsCaseIteratorActive() )
                {
                    engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromNextKey,
                        this_case_key, for_dictionary_node.dictionary_access, key_prefix);
                }
            }
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return 0;
    }

    else
    {
        DICT* pDicT = assert_cast<DICT*>(&symbol);
        DICX* pDicX = pDicT->GetDicX();

        LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::ForCase, pDicT);
        if( !loop_stack_entry.IsValid() )
            return 0;

        try
        {
            std::optional<CString> key_prefix;
            bool use_where_expression = ( for_dictionary_node.query_type_or_where_expression != -1 );

            if( for_dictionary_node.starts_with_expression != -1 )
                key_prefix = EvalAlphaExpr<CString>(for_dictionary_node.starts_with_expression);

            pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromBoundary, std::nullopt,
                for_dictionary_node.dictionary_access, key_prefix);

            while( pDicX->StepCaseIterator() )
            {
                CaseKey this_case_key = pDicX->GetCase();

                m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                &pDicX->GetCase().GetRootCaseLevel(), pDicT);

                if( use_where_expression && ConditionalValueIsFalse(evalexpr(for_dictionary_node.query_type_or_where_expression)) )
                    continue;

                try
                {
                    // run the logic in the for block
                    bool request_issued = ExecuteProgramStatements(for_dictionary_node.block_expression);

                    if( request_issued )
                    {
                        // break out of the current proc
                        m_iSkipStmt = TRUE;
                        break;
                    }
                }

                catch( const NextProgramControlException& )  { }
                catch( const BreakProgramControlException& ) { break; }

                if( m_iStopExec )
                    break;

                // a delcase or writecase call may stop the iterator, in which case we will start it
                // again at the case following the one loaded in this iteration of the loop
                if( !pDicX->IsCaseIteratorActive() )
                {
                    pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromNextKey, this_case_key,
                        for_dictionary_node.dictionary_access, key_prefix);
                }
            }
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
        }

        return 0;
    }
}


//--------------------------------------------------------
//
//  excountcases
//
//--------------------------------------------------------
double CIntDriver::excountcases(int iExpr)
{
    const auto& countcases_node = GetNode<Nodes::CountCases>(iExpr);
    Symbol& symbol = NPT_Ref(countcases_node.data_repository_dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        std::optional<CString> key_prefix;

        if( countcases_node.starts_with_expression != -1 )
            key_prefix = EvalAlphaExpr<CString>(countcases_node.starts_with_expression);

        EngineDictionary& data_repository_engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = data_repository_engine_dictionary.GetEngineDataRepository();

        double number_cases = 0;

        try
        {
            // simply return the number of cases if not using a where expression
            if( countcases_node.where_expression == -1 )
            {
                DictionaryAccessParameters dictionary_access_parameters = engine_data_repository.GetDictionaryAccessParameters(countcases_node.dictionary_access);
                std::unique_ptr<CaseIteratorParameters> start_parameters;

                if( key_prefix.has_value() )
                    start_parameters = std::make_unique<CaseIteratorParameters>(CaseIterationStartType::GreaterThanEquals, CString(), *key_prefix);

                number_cases = engine_data_repository.GetDataRepository().GetNumberCases(
                    dictionary_access_parameters.case_iteration_status, start_parameters.get());
            }

            else
            {
                LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::CountCases, &data_repository_engine_dictionary);
                if( !loop_stack_entry.IsValid() )
                    return 0;

                EngineCase& engine_case = GetSymbolEngineDictionary(countcases_node.case_dictionary_symbol_index).GetEngineCase();

                engine_data_repository.CreateCaseIterator(EngineDataRepository::CaseIteratorStyle::FromBoundary,
                    std::nullopt, countcases_node.dictionary_access, key_prefix);

                while( engine_data_repository.StepCaseIterator(engine_case) )
                {
                    // ENGINECR_TODO(countcases) is there anything in ParseCaseLevel that needs to be copied? m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(), pDicT);

                    if( evalexpr(countcases_node.where_expression) != 0 )
                        ++number_cases;

                    // quit out if the logic in the where condition stopped the iterator (or stopped the application)
                    if( !engine_data_repository.IsCaseIteratorActive() || m_iStopExec )
                        break;
                }
            }
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
            number_cases = DEFAULT;
        }

        return number_cases;
    }

    else
    {
        std::optional<CString> key_prefix;

        if( countcases_node.starts_with_expression != -1 )
            key_prefix = EvalAlphaExpr<CString>(countcases_node.starts_with_expression);

        DICT* pDicT = DPT(countcases_node.data_repository_dictionary_symbol_index);
        DICX* pDicX = pDicT->GetDicX();

        double number_cases = 0;

        try
        {
            // simply return the number of cases if not using a where expression
            if( countcases_node.where_expression < 0 )
            {
                auto dictionary_access_parameters = pDicX->GetDictionaryAccessParameters(countcases_node.dictionary_access);
                std::unique_ptr<CaseIteratorParameters> start_parameters;

                if( key_prefix.has_value() )
                    start_parameters = std::make_unique<CaseIteratorParameters>(CaseIterationStartType::GreaterThanEquals, CString(), *key_prefix);

                number_cases = pDicX->GetDataRepository().GetNumberCases(std::get<2>(dictionary_access_parameters), start_parameters.get());
            }

            else
            {
                LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::CountCases, pDicT);
                if( !loop_stack_entry.IsValid() )
                    return 0;

                pDicX->CreateCaseIterator(DICX::CaseIteratorStyle::FromBoundary, std::nullopt,
                    countcases_node.dictionary_access, key_prefix);

                while( pDicX->StepCaseIterator() )
                {
                    m_pEngineDriver->ParseCaseLevel(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case()->GetRootLevel(),
                                                    &pDicX->GetCase().GetRootCaseLevel(), pDicT);

                    if( evalexpr(countcases_node.where_expression) != 0 )
                        ++number_cases;

                    // quit out if the logic in the where condition stopped the iterator (or stopped the application)
                    if( !pDicX->IsCaseIteratorActive() || m_iStopExec )
                        break;
                }
            }
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
        }

        catch( const DataRepositoryException::Error& exception )
        {
            issaerror(MessageType::Warning, 10103, exception.GetErrorMessage().c_str());
            number_cases = DEFAULT;
        }

        return number_cases;
    }
}


//--------------------------------------------------------
//
//  exsetoutput
//
//--------------------------------------------------------
double CIntDriver::exsetoutput(int iExpr)
{
    if( Appl.ApplicationType != ModuleType::Batch )
        return DEFAULT;

    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    // evaluate the connection strings
    std::vector<ConnectionString> output_connection_strings;

    auto add_connection_string = [&](std::wstring text)
    {
        auto& output_connection_string = output_connection_strings.emplace_back(std::move(text));
        MakeFullPathFileName(output_connection_string);
    };

    // string value
    if( va_node.arguments[0] >= 0 )
    {
        add_connection_string(EvalAlphaExpr(va_node.arguments[0]));
    }

    // string list
    else
    {
        const LogicList& setoutput_list = GetSymbolLogicList(-1 * va_node.arguments[0]);
        size_t list_count = setoutput_list.GetCount();

        for( size_t i = 1; i <= list_count; ++i )
            add_connection_string(setoutput_list.GetString(i));
    }


    // if the requested connection strings are identical to the ones currently being used, don't reopen them
    if( output_connection_strings.size() == m_pEngineDriver->m_batchOutputRepositories.size() )
    {
        size_t matches = std::count_if(output_connection_strings.cbegin(), output_connection_strings.cend(),
            [&](const auto& output_connection_string)
            {
                const auto& search = std::find_if(m_pEngineDriver->m_batchOutputRepositories.cbegin(), m_pEngineDriver->m_batchOutputRepositories.cend(),
                    [&](const std::shared_ptr<DataRepository>& data_repository)
                {
                    return data_repository->GetConnectionString().Equals(output_connection_string);
                });

                return ( search != m_pEngineDriver->m_batchOutputRepositories.cend() );
            });

        if( matches == output_connection_strings.size() )
            return 1;
    }

    return m_pEngineDriver->OpenBatchOutputRepositories(output_connection_strings, true);
}


//----------------------------------------------------------------------
//
//  exfilename      ejecuta funcion 'FILENAME'
//
//----------------------------------------------------------------------
double CIntDriver::exfilename(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);

    // the paradata log
    if( fn8_node.symbol_index == -2 )
        return AssignAlphaValue(Paradata::Logger::GetFilename());

    // symbols
    Symbol* symbol = GetFromSymbolOrEngineItem(fn8_node.symbol_index,
                                               Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? fn8_node.extra_parameter : -1);

    if( symbol == nullptr )
        return AssignBlankAlphaValue();

    // dictionary
    if( symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary& engine_dictionary = assert_cast<const EngineDictionary&>(*symbol);
        const DataRepository& data_repository = engine_dictionary.GetEngineDataRepository().GetDataRepository();
        return AssignAlphaValue(data_repository.GetName(DataRepositoryNameType::Full));
    }

    else if( symbol->IsA(SymbolType::Pre80Dictionary) )
    {
        const DICT* pDicT = assert_cast<const DICT*>(symbol);
        const DICX* pDicX = pDicT->GetDicX();
        return AssignAlphaValue(pDicX->GetDataRepository().GetName(DataRepositoryNameType::Full));
    }

    // File
    else if( symbol->IsA(SymbolType::File) )
    {
        const LogicFile& logic_file = assert_cast<const LogicFile&>(*symbol);
        return AssignAlphaValue(logic_file.GetFilename());
    }

    // Pff
    else if( symbol->IsA(SymbolType::Pff) )
    {
        LogicPff& logic_pff = assert_cast<LogicPff&>(*symbol);
        return AssignAlphaValue(logic_pff.GetRunnableFilename());
    }

    // Report
    else if( symbol->IsA(SymbolType::Report) )
    {
        const Report& report = assert_cast<const Report&>(*symbol);
        return AssignAlphaValue(report.GetFilename());
    }

    // Audio, Document, Geometry, Image
    else if( BinarySymbol::IsBinarySymbol(*symbol) )
    {
        return AssignAlphaValue(assert_cast<const BinarySymbol&>(*symbol).GetPath());
    }

    return AssignBlankAlphaValue();
}


//----------------------------------------------------------------------
//
// exopen
//
//----------------------------------------------------------------------
double CIntDriver::exopen(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    Symbol* symbol = NPT(fn8_node.symbol_index);
    double return_value = 1;

    bool create = ( fn8_node.extra_parameter == static_cast<int>(Nodes::SetFile::Mode::Create) );
    bool append = ( fn8_node.extra_parameter == static_cast<int>(Nodes::SetFile::Mode::Append) );

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary->GetEngineDataRepository();

        ConnectionString connection_string = engine_data_repository.GetLastClosedConnectionString().IsDefined() ?
            engine_data_repository.GetLastClosedConnectionString() :
            engine_data_repository.GetDataRepository().GetConnectionString();

        return_value = exsetfile_dictionary(*engine_dictionary, connection_string, create, append);
    }

    else if( symbol->IsA(SymbolType::Pre80Dictionary) )
    {
        DICT* pDicT = assert_cast<DICT*>(symbol);
        DICX* pDicX = pDicT->GetDicX();

        ConnectionString connection_string = pDicX->GetLastClosedConnectionString().IsDefined() ?
            pDicX->GetLastClosedConnectionString() : pDicX->GetDataRepository().GetConnectionString();

        return_value = exsetfile_dictionary(pDicT, connection_string, create, append);
    }

    else if( symbol->IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(*symbol);

        if( !logic_file.Open(create, append, true) )
            return_value = 0;
    }

    return return_value;
}


//----------------------------------------------------------------------
//
// exclose
//
//----------------------------------------------------------------------
double CIntDriver::exclose(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    Symbol* symbol = NPT(fn8_node.symbol_index);
    double return_value = 1;

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary->GetEngineDataRepository();

        // set to a null repository (OpenRepository will close the current repository)
        m_pEngineDriver->OpenRepository(engine_data_repository, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, true);

        if( Issamod == ModuleType::Entry && engine_dictionary->GetSubType() == SymbolSubType::Input )
            EntryInputRepositoryChangingActions();
    }

    else if( symbol->IsA(SymbolType::Pre80Dictionary) )
    {
        DICT* pDicT = assert_cast<DICT*>(symbol);
        DICX* pDicX = pDicT->GetDicX();

        // set to a null repository (OpenRepository will close the current repository)
        m_pEngineDriver->OpenRepository(pDicX, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, true);

        if( Issamod == ModuleType::Entry && pDicT->GetSubType() == SymbolSubType::Input )
            EntryInputRepositoryChangingActions();
    }

    else if( symbol->IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(*symbol);

        if( !logic_file.Close() )
            return_value = 0;
    }

    return return_value;
}


double CIntDriver::exsetfile_dictionary(EngineDictionary& engine_dictionary, const ConnectionString& connection_string,
                                        bool create_new, bool open_or_create)
{
    EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();
    double return_value = 1;

    // if setting the file to the currently open repository (and it is not the batch input), do nothing
    if( !create_new && engine_dictionary.GetSubType() != SymbolSubType::Input &&
        connection_string.Equals(engine_data_repository.GetDataRepository().GetConnectionString()) )
    {
        return return_value;
    }

    // try to open the new repository
    try
    {
        DataRepositoryOpenFlag open_flag = create_new     ? DataRepositoryOpenFlag::CreateNew :
                                           open_or_create ? DataRepositoryOpenFlag::OpenOrCreate :
                                                            DataRepositoryOpenFlag::OpenMustExist;

        // OpenRepository will close the current repository
        m_pEngineDriver->OpenRepository(engine_data_repository, connection_string, open_flag, true);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        // on error, set the repository to a null repository
        issaerror(MessageType::Error, 10102, engine_dictionary.GetName().c_str(), exception.GetErrorMessage().c_str());
        issaerror(MessageType::Warning, 10106, engine_dictionary.GetName().c_str());

        m_pEngineDriver->OpenRepository(engine_data_repository, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, false);
        return_value = 0;
    }

    if( Issamod == ModuleType::Entry && engine_dictionary.GetSubType() == SymbolSubType::Input )
        EntryInputRepositoryChangingActions();

    return return_value;
}


double CIntDriver::exsetfile_dictionary(DICT* pDicT, const ConnectionString& connection_string, bool createNew, bool bOpenOrCreate)
{
    DICX* pDicX = pDicT->GetDicX();
    double return_value = 1;

    // if setting the file to the currently open repository (and it is not the batch input), do nothing
    if( !createNew && pDicT->GetSubType() != SymbolSubType::Input &&
        connection_string.Equals(pDicX->GetDataRepository().GetConnectionString()) )
    {
        return return_value;
    }

    // try to open the new repository
    try
    {
        DataRepositoryOpenFlag open_flag = createNew     ? DataRepositoryOpenFlag::CreateNew :
                                           bOpenOrCreate ? DataRepositoryOpenFlag::OpenOrCreate :
                                                           DataRepositoryOpenFlag::OpenMustExist;

        // OpenRepository will close the current repository
        m_pEngineDriver->OpenRepository(pDicX, connection_string, open_flag, true);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        // on error, set the repository to a null repository
        issaerror(MessageType::Error, 10102, pDicT->GetName().c_str(), exception.GetErrorMessage().c_str());
        issaerror(MessageType::Warning, 10106, pDicT->GetName().c_str());

        m_pEngineDriver->OpenRepository(pDicX, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, false);
        return_value = 0;
    }

    if( Issamod == ModuleType::Entry && pDicT->GetSubType() == SymbolSubType::Input )
        EntryInputRepositoryChangingActions();

    return return_value;
}


void CIntDriver::EntryInputRepositoryChangingActions()
{
#ifdef WIN_DESKTOP
    // update the case listing and the title bar
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_CHANGE_INPUT_REPOSITORY);
#endif

    // clear any write case parameter in use
    m_pEngineDriver->ClearWriteCaseParameter();

    // refresh the listing file
    const DICX* pDicX = DIX(0);
    m_pEngineDriver->m_pPifFile->SetSingleInputDataConnectionString(pDicX->GetDataRepository().GetConnectionString());

    m_pEngineDriver->StopLister();
    m_pEngineDriver->StartLister();
}


//----------------------------------------------------------------------
//
//  exclrcase       ejecuta funcion 'CLEAR'
//
//----------------------------------------------------------------------
double CIntDriver::exclrcase(int iExpr)
{
    const auto& fnc_node = GetNode<FNC_NODE>(iExpr);
    Symbol* symbol = NPT(fnc_node.isymb);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);
        EngineCase& engine_case = engine_dictionary->GetEngineCase();
        engine_case.ClearCase();
    }

    else
    {
        ASSERT(symbol->IsA(SymbolType::Pre80Dictionary));

        DICT* pDicT = assert_cast<DICT*>(symbol);
        DICX* pDicX = pDicT->GetDicX();

        pDicX->ResetCaseObjects();

        if( pDicT->GetSubType() == SymbolSubType::External )
        {
            for( int iLevel = 1; iLevel <= pDicT->maxlevel; iLevel++ )
                m_pEngineDriver->ClearLevelNode( pDicT, iLevel );
        }

        else if( pDicT->GetSubType() == SymbolSubType::Work )
        {
            int iSymSec = pDicT->SYMTfsec;
            SECT* pSecT = ( iSymSec > 0 ) ? SPT(iSymSec) : NULL;

            while( pSecT != NULL )
            {
                // reset the record's occurrence counts to the maximum
                GROUPT* pGroupT = pSecT->GetGroup(0);

                if( pGroupT != nullptr )
                {
                    pGroupT->SetCurrentOccurrences(pGroupT->GetMaxOccs());
                    pGroupT->SetTotalOccurrences(pGroupT->GetMaxOccs());
                }

                // clear the record's variables
                m_pEngineDriver->initsect(pSecT);

                pSecT = (SECT*)pSecT->next_symbol;
            }
        }

        *pDicX->current_key = *pDicX->last_key = 0;
    }

    return 1;
}
