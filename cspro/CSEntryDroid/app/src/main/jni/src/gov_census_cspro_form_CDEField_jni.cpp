#include <engine/StandardSystemIncludes.h>
#include <zDictO/ValueSetResponse.h>
#include <zFormO/FormFile.h>
#include <Zentryo/CoreEntryPageField.h>
#include "gov_census_cspro_form_CDEField_jni.h"
#include "JNIHelpers.h"

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetNote
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setNote
        (JNIEnv *pEnv, jobject, jlong reference, jstring note)
{
    if(reference != 0)
    {
        auto* pField = (CoreEntryPageField *)reference;
        std::wstring sNote = JavaToWSZ(pEnv, note);
        pField->SetNote(CString(sNote));
    }
}

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetBlankValue
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setBlankValue
        (JNIEnv*, jobject, jlong reference)
{
    if (reference != 0)
    {
        auto* pField = reinterpret_cast<CoreEntryPageField*>(reference);
		
		if( pField->IsNumeric() )
			pField->SetNumericValue(NOTAPPL);
		
		else
			pField->SetAlphaValue(SO::EmptyCString);
    }
}

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetNumericValue
 * Signature: (JD)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setNumericValue
        (JNIEnv *, jobject object, jlong reference, jdouble value)
{
    if (reference != 0)
    {
        auto* pField = reinterpret_cast<CoreEntryPageField*>(reference);
        pField->SetNumericValue(value);
    }
}

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetAlphaValue
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setAlphaValue
        (JNIEnv *pEnv, jobject, jlong reference, jstring value)
{
    if(reference != 0)
    {
        auto* pField = (CoreEntryPageField *)reference;
        pField->SetAlphaValue(JavaToWSZ(pEnv, value));
    }
}

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetSelectedIndex
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setSelectedIndex
        (JNIEnv *pEnv, jobject, jlong reference, jint index)
{
    if (reference != 0)
    {
        auto* pField = reinterpret_cast<CoreEntryPageField*>(reference);
        pField->SetSelectedResponses((size_t)index);
    }
}

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetCheckedIndices
 * Signature: (J[I)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setCheckedIndices
        (JNIEnv *pEnv, jobject, jlong reference, jintArray indices)
{
    if (reference != 0)
    {
        jsize len = pEnv->GetArrayLength(indices);
        std::vector<size_t> checkedIndices(len);

        jint* array_ptr = pEnv->GetIntArrayElements(indices, nullptr);

        for( auto i = 0; i < len; i++ )
            checkedIndices[i] = (size_t)array_ptr[i];

        pEnv->ReleaseIntArrayElements(indices, array_ptr, 0);

        auto* pField = reinterpret_cast<CoreEntryPageField*>(reference);
        pField->SetSelectedResponses(checkedIndices);
    }
}
