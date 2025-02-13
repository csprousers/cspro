#include "stdafx.h"
#include "CppUnitTest.h"
#include <zCaseO/VectorClock.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{
    TEST_CLASS(VectorClockTest)
    {
    public:

        TEST_METHOD(TestIncrement)
        {
            VectorClock c;
            Assert::AreEqual(0, c.getVersion("A"), _T("New clock doesn't have version 0"));
            c.increment("A");
            Assert::AreEqual(1, c.getVersion("A"), _T("First increment not equal 1"));
            c.increment("A");
            Assert::AreEqual(2, c.getVersion("A"), _T("Second increment not equal 2"));
        }

        TEST_METHOD(TestCopy)
        {
            VectorClock c;
            c.increment("A");
            c.increment("A");
            c.increment("B");
            VectorClock c2 = c;
            Assert::AreEqual(2, c.getVersion("A"), _T("Copied clock A not 2"));
            Assert::AreEqual(1, c.getVersion("B"), _T("Copied clock B not 1"));
            Assert::AreEqual(0, c.getVersion("C"), _T("Copied clock C not 0"));
        }

        TEST_METHOD(TestCompareEmpty)
        {
            VectorClock full, empty;
            full.increment("A");
            full.increment("A");
            full.increment("B");

            Assert::IsTrue(empty < full, _T("Empty clock not < full"));
            Assert::IsFalse(full < empty, _T("Full clock < empty"));
        }

        TEST_METHOD(TestCompareDescendant)
        {
            VectorClock parent;
            parent.increment("A");
            parent.increment("A");
            parent.increment("B");

            VectorClock child = parent;
            child.increment("A");

            Assert::IsTrue(parent < child, _T("Parent not < child"));
            Assert::IsFalse(child < parent, _T("Child < parent"));
        }

        TEST_METHOD(TestCompareDescendantNewDev)
        {
            VectorClock parent;
            parent.increment("A");
            parent.increment("A");
            parent.increment("B");

            VectorClock child = parent;
            child.increment("C");

            Assert::IsTrue(parent < child, _T("Parent not < child"));
            Assert::IsFalse(child < parent, _T("Child < parent"));
        }

        TEST_METHOD(TestCompareEquals)
        {
            VectorClock a;
            a.increment("A");
            a.increment("A");
            a.increment("B");

            VectorClock b = a;

            Assert::IsFalse(a < b, _T("Equal vectors are <"));
            Assert::IsFalse(b < a, _T("Equal vectors are <"));
        }

        TEST_METHOD(TestCompareDisjoint)
        {
            VectorClock a;
            a.increment("A");
            a.increment("A");
            a.increment("B");

            VectorClock b;
            b.increment("C");
            b.increment("D");

            Assert::IsFalse(a < b, _T("Disjoint vectors are <"));
            Assert::IsFalse(b < a, _T("Disjoint vectors are <"));
        }

        TEST_METHOD(TestCompareConflicting)
        {
            VectorClock a;
            a.increment("A");
            a.increment("A");
            a.increment("B");

            VectorClock b = a;
            b.increment("B");

            a.increment("A");

            Assert::IsFalse(a < b, _T("Conflicting vectors are <"));
            Assert::IsFalse(b < a, _T("Conflicting vectors are <"));
        }

    };
}
