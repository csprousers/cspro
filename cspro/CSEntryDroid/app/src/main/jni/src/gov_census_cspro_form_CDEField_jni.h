#include <jni.h>
/* Header for class gov_census_cspro_form_CDEField */

#ifndef _Included_gov_census_cspro_form_CDEField
#define _Included_gov_census_cspro_form_CDEField

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetNote
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setNote
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetNumericValue
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setBlankValue
  (JNIEnv *, jobject, jlong);

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetNumericValue
 * Signature: (JD)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setNumericValue
  (JNIEnv *, jobject, jlong, jdouble);

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetAlphaValue
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setAlphaValue
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetSelectedIndex
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setSelectedIndex
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     gov_census_cspro_form_CDEField
 * Method:    SetCheckedIndices
 * Signature: (J[I)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_form_CDEField_setCheckedIndices
  (JNIEnv *, jobject, jlong, jintArray);

#ifdef __cplusplus
}
#endif

#endif
