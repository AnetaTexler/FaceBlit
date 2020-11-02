#include <jni.h>
#include <string>
#include "style_transfer.h"
#include <opencv2/core.hpp>

#include "common_utils.h"
#include "time_helper.h"

extern "C" JNIEXPORT jstring JNICALL
Java_texler_faceblit_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_texler_faceblit_JavaNativeInterface_getStylizedData(JNIEnv *env, jobject /*instance*/,
                                                         jstring modelPath,
                                                         jstring styleLandmarks,
                                                         jbyteArray lookupTableBytes,
                                                         jbyteArray styleImgBytes,
                                                         jbyteArray targetImgBytes,
                                                         jint width, jint height,
                                                         jboolean rotateRight,
                                                         jboolean rotateLeft,
                                                         jboolean horizontalFlip,
                                                         jboolean stylizeFaceOnly) {

    Log_e("HACK", "#######################");
    TimeMeasure t_getByteArrays;

    int targetLength = env->GetArrayLength(targetImgBytes);
    auto* targetImgChars = new unsigned char[targetLength];
    env->GetByteArrayRegion(targetImgBytes, 0, targetLength, reinterpret_cast<jbyte*>(targetImgChars));

    unsigned char* styleImgChars = nullptr;
    if(styleImgBytes != nullptr) {
        int styleLength = env->GetArrayLength(styleImgBytes);
        styleImgChars = new unsigned char[styleLength];
        env->GetByteArrayRegion(styleImgBytes, 0, styleLength, reinterpret_cast<jbyte*>(styleImgChars));
    }

    unsigned char* lookupTableChars = nullptr;
    if(lookupTableBytes != nullptr) {
        int cubeLength = env->GetArrayLength(lookupTableBytes);
        lookupTableChars = new unsigned char[cubeLength];
        env->GetByteArrayRegion(lookupTableBytes, 0, cubeLength, reinterpret_cast<jbyte*>(lookupTableChars));
    }

    Log_e("HACK", std::string() + "t_getByteArrays time: " + std::to_string(t_getByteArrays.elapsed_milliseconds()));

    const char* nativeModelPath = nullptr;
    if(modelPath != nullptr) {
        nativeModelPath = env->GetStringUTFChars(modelPath, 0);
    }

    const char* nativeStyleLandmarks = nullptr;
    if(styleLandmarks != nullptr) {
        nativeStyleLandmarks = env->GetStringUTFChars(styleLandmarks, 0);
    }

    // --- Calling C method -----------------------------------------------------------------------------------------
    unsigned char* outImgChars = stylize(nativeModelPath,
                                         nativeStyleLandmarks,
                                         lookupTableChars,
                                         styleImgChars,
                                         targetImgChars,
                                         width, height,
                                         stylizeFaceOnly);
    // --------------------------------------------------------------------------------------------------------------

    jbyteArray javaOutImgBytes = nullptr;
    if (outImgChars != nullptr) {
        javaOutImgBytes = env->NewByteArray(targetLength);
        env->SetByteArrayRegion(javaOutImgBytes, 0, targetLength, reinterpret_cast<jbyte*>(outImgChars));
    }

    // --- Release memory ------------------------------------------------------------------------
    if(modelPath != nullptr) {
        env->ReleaseStringUTFChars(modelPath, nativeModelPath);
    }

    if(styleLandmarks != nullptr) {
        env->ReleaseStringUTFChars(styleLandmarks, nativeStyleLandmarks);
    }

    env->ReleaseByteArrayElements(targetImgBytes, (jbyte*)targetImgChars, JNI_ABORT);

    if(styleImgBytes != nullptr) {
        env->ReleaseByteArrayElements(styleImgBytes, (jbyte*)styleImgChars, JNI_ABORT);
    }

    if(lookupTableBytes != nullptr){
        env->ReleaseByteArrayElements(lookupTableBytes, (jbyte*)lookupTableChars, JNI_ABORT);
    }
    // -------------------------------------------------------------------------------------------

    return javaOutImgBytes;
}

