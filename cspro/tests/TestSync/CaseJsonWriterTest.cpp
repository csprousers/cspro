#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "CppUnitTest.h"
#include <zAppO/SyncTypes.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zSyncO/CaseJsonWriter.h>
#include <fstream>
#include <sstream>

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }

        }
    }
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{
    TEST_CLASS(CaseJsonWriterTest)
    {
    public:
        TEST_METHOD(TestCaseToJson)
        {
            ConnectionString test_file("ThreeLevelTest.csdb");
            auto dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(test_file);
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
            std::unique_ptr<DataRepository> repository = DataRepository::CreateAndOpen(case_access, test_file, DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);
            auto case_iterator = repository->CreateCaseIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);
            Case data_case(repository->GetCaseAccess()->GetCaseMetadata());
            case_iterator->NextCase(data_case);
            std::ostringstream os;
            CaseJsonWriter case_writer;
            case_writer.ToJson(os, data_case);
            auto actual_json = os.str();
            auto expected_json = R"({"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":{"id":{"PROVINCE":1,"DISTRICT":1,"VILLAGE":"001","EA":1,"UR":1,"COMPOUND":1},"COMPOUND_INFO_REC":{"LATITUDE":234.0,"LONGITUDE":234234.567,"HOUSING_UNITS":[1,2,3,4,5],"NUM_HUS":1},"level-2":[{"id":{"HOUSING_UNIT_NUMBER":1},"HOUSING_UNIT_REC":{"HU01_TYPE":1,"HU02_WALL":1,"HU03_ROOF":1,"HU04_FLOOR":1,"HU05_NUM_HOUSEHOLDS":1},"level-3":[{"id":{"HOUSEHOLD_NUMBER":1},"HOUSEHOLD_REC":{"H06_TENURE":1,"H07_RENT":123,"H08_TOILET":4,"H09_BATH":3,"H10_WATER":2,"H11_LIGHT":1,"H12_FUEL":2,"H13_PERSONS":1},"PERSON_REC":[{"FIRST_NAME":"newdude","P02_REL":1,"P03_SEX":8,"P04_DOB":2000101,"P04DOB_YEAR":200,"P04DOB_MONTH":1,"P04DOB_DAY":1,"P04_AGE":99,"P05_MS":1,"P06_MOTHER":2,"P07_BIRTH":3,"P08_RES95":3,"P09_ATTEND":2,"P11_LITERACY":1,"P12_WORKING":2,"P13_LOOKING":1,"P14_WHY_NOT":2,"P15_OCC":1,"P15A_OCC":0,"SINGLE_PARENT_OF_REPEATING_SUBITEM":"12*4","REPEATING_SUBITEM":[1,2,"DEFAULT"],"NOT_REPEATING_SUBITEM":4,"REPEATING_PARENT":["","2"],"SUB_ONE":["",2],"P25_MORE":2}]}]}]}})";
            Assert::AreEqual(expected_json, actual_json.c_str());
        }
    };
}
