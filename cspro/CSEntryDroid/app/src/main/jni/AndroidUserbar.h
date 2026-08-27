#pragma once

#include <zEngineF/PortableUserbar.h>
#include "JNIHelpers.h"


class AndroidUserbar : public PortableUserbar
{
public:
    UserFunctionArgumentEvaluator* GetFunctionForButton(size_t userbar_index);

protected:
    void OnDisplayOrItemChange() override;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline UserFunctionArgumentEvaluator* AndroidUserbar::GetFunctionForButton(const size_t userbar_index)
{
    ASSERT(userbar_index < m_items.size());
    const PortableUserbarItem& item = m_items[userbar_index];

    m_lastActivatedItemId = item.id;

    if( item.action.has_value() && std::holds_alternative<std::unique_ptr<UserFunctionArgumentEvaluator>>(*item.action) )
        return std::get<std::unique_ptr<UserFunctionArgumentEvaluator>>(*item.action).get();

    return nullptr;
}


inline void AndroidUserbar::OnDisplayOrItemChange()
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobjectArray> jButtonTexts(env,
        env->NewObjectArray(m_items.size(), JNIReferences::classString, env->NewStringUTF(""))
    );

    for( size_t i = 0; i < m_items.size(); ++i )
    {
        JNIReferences::scoped_local_ref<jstring> jButtonText(env, JavaString::ToJava(*env, m_items[i].text));
        env->SetObjectArrayElement(jButtonTexts.get(), i, jButtonText.get());
    }

    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceUserbar,
                              m_shownState, jButtonTexts.get());
}
