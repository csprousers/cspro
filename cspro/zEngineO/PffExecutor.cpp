#include "stdafx.h"
#include "PffExecutor.h"
#include <zUtilO/ExecutionStack.h>
#include <CSView/InputProcessor.h>
#include <zConcatO/Concatenator.h>
#include <zDiffO/Differ.h>
#include <zIndexO/Indexer.h>
#include <zParadataO/GuiConcatenator.h>
#include <zParadataO/GuiConcatenatorPffWrapper.h>
#include <zPackO/Packer.h>
#include <zReformatO/ToolReformatter.h>
#include <zSortO/Sorter.h>


bool PffExecutor::SetEmbeddedDictionary(const std::wstring& property_name, std::shared_ptr<const CDataDict> dictionary)
{
    if( SO::EqualsNoCase(property_name, PFF_COMMAND_INPUT_DICT) )
    {
        m_inputDictionary = std::move(dictionary);
    }

    else if( SO::EqualsNoCase(property_name, PFF_COMMAND_OUTPUT_DICT) )
    {
        m_outputDictionary = std::move(dictionary);
    }

    else
    {
        return false;
    }

    return true;
}


bool PffExecutor::CanExecute(const APPTYPE app_type)
{
    return ( app_type == APPTYPE::COMPARE_TYPE )         ||
           ( app_type == APPTYPE::CONCAT_TYPE )          ||
           ( app_type == APPTYPE::INDEX_TYPE )           ||
           ( app_type == APPTYPE::PACK_TYPE )            ||
           ( app_type == APPTYPE::PARADATA_CONCAT_TYPE ) ||
           ( app_type == APPTYPE::REFORMAT_TYPE )        ||
           ( app_type == APPTYPE::SORT_TYPE )            ||
           ( app_type == APPTYPE::VIEW_TYPE );
}


bool PffExecutor::Execute(const PFF& pff)
{
    ASSERT(CanExecute(pff.GetAppType()));

    const ExecutionStackEntry execution_stack_entry = ExecutionStack::AddEntry(&pff);

    return ( pff.GetAppType() == APPTYPE::COMPARE_TYPE )         ?    ExecuteCSDiff(pff) :
           ( pff.GetAppType() == APPTYPE::CONCAT_TYPE )          ?    ExecuteCSConcat(pff) :
           ( pff.GetAppType() == APPTYPE::INDEX_TYPE )           ?    ExecuteCSIndex(pff) :
           ( pff.GetAppType() == APPTYPE::PACK_TYPE )            ?    ExecuteCSPack(pff) :
           ( pff.GetAppType() == APPTYPE::PARADATA_CONCAT_TYPE ) ?    ExecuteParadataConcat(pff) :
           ( pff.GetAppType() == APPTYPE::REFORMAT_TYPE )        ?    ExecuteCSReFmt(pff) :
           ( pff.GetAppType() == APPTYPE::SORT_TYPE )            ?    ExecuteCSSort(pff) :
         /*( pff.GetAppType() == APPTYPE::VIEW_TYPE )            ? */ ExecuteCSView(pff);
}



// --------------------------------------------------------------------------
// CSConcat
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSConcat(const PFF& pff)
{
    Concatenator::RunSuccess run_success = Concatenator().Run(pff, true, m_inputDictionary);
    return ( run_success == Concatenator::RunSuccess::Success ||
             run_success == Concatenator::RunSuccess::SuccessWithErrors );
}



// --------------------------------------------------------------------------
// CSDiff
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSDiff(const PFF& pff)
{
    return Differ().Run(pff, true, m_inputDictionary);
}



// --------------------------------------------------------------------------
// CSIndex
// --------------------------------------------------------------------------

namespace
{
    class PffExecutorIndexer : public Indexer
    {
    protected:
        bool SupportsInteractiveMode() const override
        {
            return false;
        }

        void DisplayInteractiveModeMessage(NullTerminatedString /*message*/) const override
        {
            throw ProgrammingErrorException();
        }

        void ChooseDuplicate(std::vector<DuplicateInfo>& /*case_duplicates*/, size_t /*duplicate_index*/, size_t /*number_duplicates*/) const override
        {
            throw ProgrammingErrorException();
        }

        bool RethrowTerminatingException() const override
        {
            return false;
        }
    };
}


bool PffExecutor::ExecuteCSIndex(const PFF& pff)
{
    PffExecutorIndexer().Run(pff, true, m_inputDictionary);
    return true;
}



// --------------------------------------------------------------------------
// CSReFmt
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSReFmt(const PFF& pff)
{
    return ToolReformatter().Run(pff, true, m_inputDictionary, m_outputDictionary);
}



// --------------------------------------------------------------------------
// CSSort
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSSort(const PFF& pff)
{
    Sorter::RunSuccess run_success = Sorter().Run(pff, true, m_inputDictionary);
    return ( run_success == Sorter::RunSuccess::Success ||
             run_success == Sorter::RunSuccess::SuccessWithStructuralErrors );
}



// --------------------------------------------------------------------------
// ParadataConcat
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteParadataConcat(const PFF& pff)
{
    Paradata::GuiConcatenatorPffWrapper pff_wrapper(pff);
    return Paradata::GuiConcatenator::Run(pff_wrapper);
}



// --------------------------------------------------------------------------
// CSPack
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSPack(const PFF& pff)
{
    Packer::Run(pff, true);
    return true;
}



// --------------------------------------------------------------------------
// CSView
// --------------------------------------------------------------------------

bool PffExecutor::ExecuteCSView(const PFF& pff)
{
    CSViewInputProcessor input_processor(pff);

    Viewer viewer;
    viewer.UseEmbeddedViewer()
          .UseSharedHtmlLocalFileServer()
          .ViewFileInEmbeddedBrowser(input_processor.GetFilename());

    return true;
}
