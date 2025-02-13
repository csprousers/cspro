#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "VTSTRUCT.h"


std::shared_ptr<VTSTRUCT> CIntDriver::GetVTStructFromPool()
{
    auto set_null_at_grow = [&](size_t index)
    {
        if( m_arrVtStructPool.size() <= index )
            m_arrVtStructPool.resize(index + 1);
    };

    if(m_arrVtStructPool.empty()){
        m_iVTPool =0;
        set_null_at_grow(m_iVTPool);
        m_iVTPool++;
        return std::make_shared<VTSTRUCT>();
    }
    else {
        if((int)m_arrVtStructPool.size() > m_iVTPool){
            if(m_arrVtStructPool[m_iVTPool] == NULL){
                m_iVTPool++;
                return std::make_shared<VTSTRUCT>();
            }
            else {
                std::shared_ptr<VTSTRUCT> pRetStruct = m_arrVtStructPool[m_iVTPool];
                m_arrVtStructPool[m_iVTPool] = nullptr;
                m_iVTPool++;
                return pRetStruct;

            }
        }
        else {//add a new one
            set_null_at_grow(m_iVTPool);
            m_iVTPool++;
            return std::make_shared<VTSTRUCT>();
        }
    }
}


void CIntDriver::AddVTStructToPool(std::shared_ptr<VTSTRUCT> vtStruct)
{
    m_iVTPool--;
    m_arrVtStructPool[m_iVTPool] = std::move(vtStruct);
}


std::shared_ptr<std::vector<int>> CIntDriver::GetIntArrFromPool()
{
    auto set_null_at_grow = [&](size_t index)
    {
        if( m_arrIntsPool.size() <= index )
            m_arrIntsPool.resize(index + 1);
    };

    if(m_arrIntsPool.empty()){
        m_iIntPool = 0;
        set_null_at_grow(m_iIntPool);
        m_iIntPool++;
        return std::make_shared<std::vector<int>>();
    }
    else {
        if((int)m_arrIntsPool.size() > m_iIntPool){
            if(m_arrIntsPool[m_iIntPool] == NULL){
                m_iIntPool++;
                return std::make_shared<std::vector<int>>();
            }
            else {
                std::shared_ptr<std::vector<int>> pIntArr = m_arrIntsPool[m_iIntPool];
                m_arrIntsPool[m_iIntPool] = nullptr;
                m_iIntPool++;
                return pIntArr;
            }
        }
        else {//add a new one
            set_null_at_grow(m_iIntPool);
            m_iIntPool++;
            return std::make_shared<std::vector<int>>();
        }
    }
}


void CIntDriver::AddIntArrToPool(std::shared_ptr<std::vector<int>> pIntArr)
{
    //m_arrIntsPool.Add(pIntArr);
    m_iIntPool--;
    m_arrIntsPool[m_iIntPool] = std::move(pIntArr);
}
