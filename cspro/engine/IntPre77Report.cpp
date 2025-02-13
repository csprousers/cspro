#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Array.h>
#include <zEngineO/List.h>
#include <zEngineO/Nodes/Query.h>
#include <zJson/Json.h>
#include <ZBRIDGEO/npff.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/SQLiteRepository.h>
#include <zDataO/TextRepository.h>
#include <zParadataO/Logger.h>
#include <zReportO/Pre77ReportException.h>
#include <zReportO/Pre77ReportManager.h>


namespace
{
    constexpr int PRE77_SETREPORTDATA_SOURCE_CLEAR = 1;
    constexpr int PRE77_SETREPORTDATA_SOURCE_STRING_EXPRESSION = 2;
    constexpr int PRE77_SETREPORTDATA_SOURCE_SYMBOL = 3;
    constexpr int PRE77_SETREPORTDATA_SOURCE_SQLQUERY = 4;
}


class LogicReportManagerAssistant : public Pre77Report::IReportManagerAssistant
{
public:
    LogicReportManagerAssistant(CIntDriver* pIntDriver)
        :   m_pIntDriver(pIntDriver),
            m_pEngineArea(pIntDriver->m_pEngineArea)
    {
    }

    ~LogicReportManagerAssistant()
    {
    }

    sqlite3* GetSqlite(CString csDataSourceName)
    {
        sqlite3* db = nullptr;

        if( csDataSourceName.CompareNoCase(_T("paradata")) == 0 )
        {
            db = Paradata::Logger::GetSqlite();
        }

        // search for a dictionary with the data source name
        else
        {
            int dictionary_symbol_index = m_pEngineArea->SymbolTableSearch(csDataSourceName, { SymbolType::Pre80Dictionary });

            if( dictionary_symbol_index != 0 )
            {
                DICT* pDicT = DPT(dictionary_symbol_index);
                DICX* pDicX = pDicT->GetDicX();
                db = DataRepositoryHelpers::GetSqliteDatabase(pDicX->GetDataRepository());
            }
        }

        if( db != nullptr )
        {
            // register any user-specified logic functions as SQL functions
            m_pIntDriver->RegisterSqlCallbackFunctions(db);
        }

        return db;
    }

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_pEngineArea->GetSymbolTable(); }

private:
    CIntDriver* m_pIntDriver;
    CEngineArea* m_pEngineArea;
};


void CreateReportManager(CIntDriver* pIntDriver)
{
    if( pIntDriver->m_pre77reportManager == nullptr )
        pIntDriver->m_pre77reportManager = std::make_unique<Pre77Report::ReportManager>(std::make_unique<LogicReportManagerAssistant>(pIntDriver));
}


template<typename JsonStringWriter>
void JsonWriterOutput(JsonStringWriter& jsw, const double& dValue)
{
    if( dValue == NOTAPPL )
        jsw.Write("");

    else
        jsw.Write(dValue);
}


double CIntDriver::expre77_setreportdata(int iExpr)
{
    const FNVARIOUS_NODE* pFunc = &GetNode<FNVARIOUS_NODE>(iExpr);
    int iAttributeName = pFunc->fn_expr[0];
    int iSourceType = pFunc->fn_expr[1];
    int iSourceSymbol = pFunc->fn_expr[2];

    CreateReportManager(this);

    if( iSourceType == PRE77_SETREPORTDATA_SOURCE_CLEAR )
    {
        m_pre77reportManager->ClearReportData();
    }

    else
    {
        // check that the attribute is a valid name
        CString csAttribute = ( iAttributeName >= 0 ) ? EvalAlphaExpr<CString>(iAttributeName) : WS2CS(NPT(iSourceSymbol)->GetName());

        if( !CIMSAString::IsName(csAttribute) )
        {
            issaerror(MessageType::Error, 8293, csAttribute.GetString());
            return 0;
        }

        // process SQL queries, which will get created by the ReportManager
        if( iSourceType == PRE77_SETREPORTDATA_SOURCE_SQLQUERY )
        {
            ASSERT(GetNode<Nodes::SqlQuery>(iSourceSymbol).destination_symbol_index == Nodes::SqlQuery::SetReportDataDestinationJson);

            std::function<double(sqlite3*, const std::string&)> setreportdata_callback =
                [&](sqlite3* db, const std::string& sql_query) -> double
                {
                    try
                    {
                        m_pre77reportManager->SetReportData(csAttribute, db, sql_query);
                        return 1;
                    }

                    catch( const CSProException& exception )
                    {
                        issaerror(MessageType::Error, 8294, exception.GetErrorMessage().c_str());
                        return DEFAULT;
                    }
                };

            if( exsqlquery(iSourceSymbol, &setreportdata_callback) == DEFAULT )
                return 0;
        }

        // otherwise, create the JSON string in this method
        else
        {
            auto jsw = Json::CreateStringWriter<char>();

            jsw->BeginObject();

            jsw->Key(csAttribute);

            // process string expressions
            if( iSourceType == PRE77_SETREPORTDATA_SOURCE_STRING_EXPRESSION )
            {
                jsw->Write(EvalAlphaExpr<CString>(iSourceSymbol));
            }


            // process symbols
            else if( iSourceType == PRE77_SETREPORTDATA_SOURCE_SYMBOL )
            {
                Symbol* pSymbol = NPT(iSourceSymbol);

                // process an array
                if( pSymbol->GetType() == SymbolType::Array )
                {
                    const LogicArray* logic_array = assert_cast<const LogicArray*>(pSymbol);

                    // don't output the (0) indexed entries
                    std::vector<size_t> indices(logic_array->GetNumberDimensions(), 1);

                    std::function<void(size_t)> array_writer =
                        [&](size_t dimension_updating)
                        {
                            ASSERT(indices[dimension_updating] == 1);

                            while( indices[dimension_updating] < logic_array->GetDimension(dimension_updating) )
                            {
                                ASSERT(logic_array->IsValidIndex(indices));

                                // if on the final dimension, output the values
                                if( dimension_updating == ( indices.size() - 1 ) )
                                {
                                    if( logic_array->IsNumeric() )
                                    {
                                        double value = logic_array->GetValue<double>(indices);
                                        JsonWriterOutput(*jsw, value);
                                    }

                                    else
                                    {
                                        const std::wstring& value = logic_array->GetValue<std::wstring>(indices);
                                        jsw->Write(SO::TrimRight(value));
                                    }
                                }

                                // otherwise iterate on the next dimension
                                else
                                {
                                    jsw->BeginArray();
                                    array_writer(dimension_updating + 1);
                                    jsw->EndArray();
                                }

                                ++indices[dimension_updating];
                            }

                            // reset the index back to 1
                            indices[dimension_updating] = 1;
                        };

                    jsw->BeginArray();
                    array_writer(0);
                    jsw->EndArray();
                }


                // process a list
                else if( pSymbol->GetType() == SymbolType::List )
                {
                    const LogicList* pList = assert_cast<const LogicList*>(pSymbol);

                    jsw->BeginArray();

                    for( size_t i = 0; i < pList->GetCount(); i++ )
                    {
                        if( pList->IsNumeric( ) )
                            JsonWriterOutput(*jsw,pList->GetValue(i + 1));

                        else
                            jsw->Write(pList->GetString(i + 1));

                    }

                    jsw->EndArray();
                }


                // process a record
                else
                {
                    SECT* pSecT = (SECT*)pSymbol;
                    int iCurrentRecordOccurrences = exsoccurs(pSecT);

                    jsw->BeginArray();

                    for( int iRecordOcc = 0; iRecordOcc < iCurrentRecordOccurrences; iRecordOcc++ )
                    {
                        jsw->BeginObject();

                        int iSymVar = pSecT->SYMTfvar;

                        while( iSymVar > 0 )
                        {
                            VART* pVarT = VPT(iSymVar);
                            VARX* pVarX = pVarT->GetVarX();
                            bool bItemHasOccurrences = ( pVarT->GetMaxOccs() > 1 );

                            jsw->Key(WS2CS(pVarT->GetName()));

                            if( bItemHasOccurrences )
                                jsw->BeginArray();

                            for( int iItemOcc = 0; iItemOcc < pVarT->GetMaxOccs(); iItemOcc++ )
                            {
                                double* pdValue = nullptr;
                                TCHAR* lpszValue = nullptr;

                                if( pVarT->IsArray() )
                                {
                                    CNDIndexes theIndex(ZERO_BASED);
                                    theIndex.setAtOrigin();

                                    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
                                    {
                                        CDimension::VDimType eDimType = pVarT->GetDimType(iDim);
                                        int iOccurrence = 0;

                                        if( eDimType == CDimension::VDimType::Record )
                                            iOccurrence = iRecordOcc;

                                        else if( eDimType != CDimension::VDimType::VoidDim )
                                            iOccurrence = iItemOcc;

                                        theIndex.setIndexValue(eDimType,iOccurrence);
                                    }

                                    if( pVarT->IsNumeric() )
                                        pdValue = GetMultVarFloatAddr(pVarX,theIndex);

                                    else
                                        lpszValue = GetMultVarAsciiAddr(pVarX,theIndex);
                                }

                                else // singly occurring value
                                {
                                    if( pVarT->IsNumeric() )
                                        pdValue = GetSingVarFloatAddr(pVarX);

                                    else
                                        lpszValue = GetSingVarAsciiAddr(pVarX);
                                }

                                if( pdValue != nullptr )
                                {
                                    // for numerics with long fractional values, truncate the fraction based
                                    // on the VART setting
                                    if( !IsSpecial(*pdValue) )
                                    {
                                        *pdValue *= Power10[pVarT->GetDecimals()];
                                        *pdValue = floor(*pdValue);
                                        *pdValue /= Power10[pVarT->GetDecimals()];
                                    }

                                    JsonWriterOutput(*jsw, *pdValue);
                                }

                                else
                                {
                                    jsw->Write(CString(lpszValue,pVarT->GetLength()).TrimRight());
                                }
                            }

                            if( bItemHasOccurrences )
                                jsw->EndArray();

                            iSymVar = pVarT->SYMTfwd;
                        }

                        jsw->EndObject();
                    }

                    jsw->EndArray();
                }
            }


            // set the report data
            jsw->EndObject();

            m_pre77reportManager->SetReportData(csAttribute, jsw->GetString());
        }
    }

    return 1;
}


double CIntDriver::expre77_report(int iExpr)
{
    const FNN_NODE* pFun = &GetNode<FNN_NODE>(iExpr);
    bool bViewMode = true;
    double dRetVal = 1;

    CString csTemplateFilename = WS2CS(EvalFullPathFileName(pFun->fn_expr[0]));
    CString csOutputFilename;

    if( pFun->fn_nargs > 1 )
    {
        bViewMode = false;
        csOutputFilename = WS2CS(EvalFullPathFileName(pFun->fn_expr[1]));
    }

    CreateReportManager(this);

    try
    {
        m_pre77reportManager->CreateReport(csTemplateFilename,&csOutputFilename);

        if( bViewMode )
        {
            Viewer viewer;
            viewer.UseEmbeddedViewer()
                  .ViewFile(CS2WS(csOutputFilename));
        }
    }

    catch( const Pre77Report::Exception& exception )
    {
        issaerror(MessageType::Error, 8294, exception.GetErrorMessage().c_str());
        dRetVal = 0;
    }

    return dRetVal;
}
