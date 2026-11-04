#include "Lab.h"
#include <jni.h>
#include <stdexcept>
extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordehybridlab_MainActivity_runNative(JNIEnv* env,jobject,jstring path,jint width,jint height) {
    const char* raw=env->GetStringUTFChars(path,nullptr);
    std::string output(raw);env->ReleaseStringUTFChars(path,raw);
    try {auto result=RunHordeHybridLab(output,width,height);return env->NewStringUTF(result.c_str());}
    catch(const std::exception& e) {std::string error="FAILED: ";error+=e.what();return env->NewStringUTF(error.c_str());}
}
