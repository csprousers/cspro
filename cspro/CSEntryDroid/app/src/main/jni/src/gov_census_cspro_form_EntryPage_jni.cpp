#include <engine/StandardSystemIncludes.h>
#include <android/log.h>
#include <zFormO/FormFile.h>
#include <Zentryo/CoreEntryPage.h>
#include <zDictO/ValueSetResponse.h>
#include "gov_census_cspro_form_EntryPage_jni.h"
#include "JNIHelpers.h"

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetOccurrenceLabel
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_form_EntryPage_GetOccurrenceLabel
  (JNIEnv *pEnv, jobject, jlong reference)
{
    auto* pPage = reinterpret_cast<CoreEntryPage*>(reference);
    return WideToJava(pEnv, pPage->GetOccurrenceLabel());
}

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetBlockName
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_form_EntryPage_GetBlockName
  (JNIEnv *pEnv, jobject, jlong reference)
{
    auto* pPage = reinterpret_cast<CoreEntryPage*>(reference);
    return WideToJava(pEnv, pPage->GetBlockName());
}

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetBlockLabel
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_form_EntryPage_GetBlockLabel
  (JNIEnv *pEnv, jobject, jlong reference)
{
    auto* pPage = reinterpret_cast<CoreEntryPage*>(reference);
    return WideToJava(pEnv, pPage->GetBlockLabel());
}

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetBlockQuestionTextUrl
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_form_EntryPage_GetBlockQuestionTextUrl
  (JNIEnv *pEnv, jobject, jlong reference)
{
    auto* pPage = reinterpret_cast<CoreEntryPage*>(reference);
    return OptionalWideToJava(pEnv, pPage->GetBlockQuestionTextUrl());
}

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetBlockHelpTextUrl
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_form_EntryPage_GetBlockHelpTextUrl
  (JNIEnv *pEnv, jobject, jlong reference)
{
    auto* pPage = reinterpret_cast<CoreEntryPage*>(reference);
    return OptionalWideToJava(pEnv, pPage->GetBlockHelpTextUrl());
}

static jobjectArray GetFieldResponses(JNIEnv *env, const CoreEntryPageField& field)
{
    const auto& responses = field.GetResponses();
    jobjectArray jresponses = env->NewObjectArray(responses.size(),
                                                  JNIReferences::classValuePair, nullptr);
    for (int i = 0; i < responses.size(); ++i) {
        const auto &response = responses[i];

        JNIReferences::scoped_local_ref<jstring> jcode(env, WideToJava(env, response->GetCode()));
        JNIReferences::scoped_local_ref<jstring> jlabel(env, WideToJava(env, response->GetLabel()));
        JNIReferences::scoped_local_ref<jstring> jimage(env, WideToJava(env, response->GetImageFilename()));

        // create the value pair object
        JNIReferences::scoped_local_ref<jobject> vp(env, env->NewObject(
                JNIReferences::classValuePair, JNIReferences::classValuePairConstructor,
                jcode.get(), jlabel.get(),
                response->GetTextColor().ToColorInt(), jimage.get(),
                (jboolean) response->IsDiscrete()));

        env->SetObjectArrayElement(jresponses, i, vp.get());
    }

    return jresponses;
}

/*
 * Class:     gov_census_cspro_form_EntryPage
 * Method:    GetPageFields
 * Signature: (J)[Lgov/census/cspro/form/CDEField;
 */
JNIEXPORT jobjectArray JNICALL Java_gov_census_cspro_form_EntryPage_GetPageFields
  (JNIEnv *pEnv, jobject, jlong reference)
{
    CoreEntryPage* pCorePage = reinterpret_cast<CoreEntryPage*>(reference);
    const auto& fields = pCorePage->GetPageFields();

    auto jFieldArray = (jobjectArray) pEnv->NewObjectArray(fields.size(), JNIReferences::classCDEField, nullptr);

    for( int i = 0; i < fields.size(); ++i )
    {
        const CoreEntryPageField& field = fields[i];
        const CaptureInfo& evaluated_capture_info = field.GetEvaluatedCaptureInfo();

        JNIReferences::scoped_local_ref<jstring> jname(pEnv, WideToJava(pEnv, field.GetName()));
        JNIReferences::scoped_local_ref<jstring> jlabel(pEnv, WideToJava(pEnv, field.GetLabel()));
        JNIReferences::scoped_local_ref<jstring> jquestionTextUrl(pEnv, OptionalWideToJava(pEnv, field.GetQuestionTextUrl()));
        JNIReferences::scoped_local_ref<jstring> jhelpTextUrl(pEnv, OptionalWideToJava(pEnv, field.GetHelpTextUrl()));
        JNIReferences::scoped_local_ref<jstring> jnote(pEnv, WideToJava(pEnv, field.GetNote()));
        jint jinteger_part_length = 0;
        jint jfractional_part_length = 0;
        jint jalpha_length =  0;
        JNIReferences::scoped_local_ref<jstring> jdate_format(pEnv);
        jboolean juppercase = false;
        jboolean jmultiline = false;
        jint jmax_checkbox_selections = 0;
        jdouble jslider_min_value = 0;
        jdouble jslider_max_value = 0;
        jdouble jslider_step = 0;
        JNIReferences::scoped_local_ref<jobjectArray> jresponses(pEnv);
        JNIReferences::scoped_local_ref<jobject> jnumeric_value(pEnv);
        JNIReferences::scoped_local_ref<jstring> jalpha_value(pEnv);
        JNIReferences::scoped_local_ref<jobject> jselected_index(pEnv);
        JNIReferences::scoped_local_ref<jintArray> jselected_responses(pEnv);

        if( field.IsNumeric() )
        {
            jinteger_part_length = field.GetIntegerPartLength();
            jfractional_part_length = field.GetFractionalPartLength();

            double numeric_val = field.GetNumericValue();

            if( numeric_val != NOTAPPL )
                jnumeric_value = pEnv->NewObject(JNIReferences::classDouble, JNIReferences::methodDoubleConstructor, numeric_val);
        }

        else if( field.IsAlpha() )
        {
            jalpha_length = field.GetAlphaLength();

            if( evaluated_capture_info.GetCaptureType() == CaptureType::TextBox || evaluated_capture_info.GetCaptureType() == CaptureType::ComboBox )
                juppercase = field.IsUpperCase();

            if( evaluated_capture_info.GetCaptureType() == CaptureType::TextBox )
                jmultiline = field.IsMultiline();

            jalpha_value = WideToJava(pEnv, field.GetAlphaValue());
        }

        switch( evaluated_capture_info.GetCaptureType() )
		{
            case CaptureType::Date:
            {
                CString date_format = evaluated_capture_info.GetExtended<DateCaptureInfo>().GetFormat();
                date_format.ToLower();
                date_format.Replace(_T('m'), _T('M'));
                jdate_format = WideToJava(pEnv, date_format);
                break;
            }

            case CaptureType::RadioButton:
            case CaptureType::ComboBox:
            case CaptureType::DropDown:
            case CaptureType::ToggleButton:
            {
                jresponses = GetFieldResponses(pEnv, field);
                const auto& selected_responses = field.GetSelectedResponses();
                int selected_response = selected_responses.empty() ? -1 : (int) selected_responses.front();
                jselected_index = pEnv->NewObject(JNIReferences::classInteger, JNIReferences::methodIntegerConstructor, selected_response);
                break;
            }

            case CaptureType::CheckBox:
            {
                jresponses = GetFieldResponses(pEnv, field);
                const auto& selected_responses = field.GetSelectedResponses();
                jselected_responses = pEnv->NewIntArray(selected_responses.size());
                jint* array_ptr = pEnv->GetIntArrayElements(jselected_responses.get(), nullptr);
                for(size_t r = 0; r < selected_responses.size(); r++ )
                    array_ptr[r] = (jint) selected_responses[r];
                pEnv->ReleaseIntArrayElements(jselected_responses.get(), array_ptr, 0);
                jmax_checkbox_selections = field.GetMaxCheckboxSelections();
                break;
            }

            case CaptureType::Slider:
            {
                field.GetSliderProperties(jslider_min_value, jslider_max_value, jslider_step);
                break;
            }

            case CaptureType::TextBox:
            case CaptureType::Barcode:
            case CaptureType::Photo:
            case CaptureType::Signature:
            case CaptureType::Audio:
                break;

			default:
				ASSERT(false);
				break;
        }

        jobject jField = pEnv->NewObject(JNIReferences::classCDEField, JNIReferences::methodCDEFieldConstructor,
                                         (jlong) &field, jname.get(), jlabel.get(), (int)evaluated_capture_info.GetCaptureType(),
										 jquestionTextUrl.get(), jhelpTextUrl.get(),
                                         jnote.get(), field.IsNumeric(), field.IsReadOnly(), jinteger_part_length,
                                         jfractional_part_length, jalpha_length, jdate_format.get(),
                                         juppercase, jmultiline, field.IsMirror(),
                                         jmax_checkbox_selections, jslider_min_value, jslider_max_value, jslider_step,
                                         jresponses.get(), jnumeric_value.get(), jalpha_value.get(),
                                         jselected_index.get(), jselected_responses.get());
        pEnv->SetObjectArrayElement(jFieldArray, i, jField);
    }

    return jFieldArray;
}
