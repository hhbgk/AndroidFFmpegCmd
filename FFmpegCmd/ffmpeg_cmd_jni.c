/**
 * reference https://github.com/mabeijianxi/FFmpeg4Android
 */
#include "ffmpeg.h"
#include <jni.h>
#include <android/log.h>

#define tag "JLMedia"
#define logi(TAG, ...)  ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define logw(TAG, ...)  ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define loge(TAG, ...)  ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

#define IJK_LOG_VERBOSE     ANDROID_LOG_VERBOSE
#define IJK_LOG_DEBUG       ANDROID_LOG_DEBUG
#define IJK_LOG_INFO        ANDROID_LOG_INFO
#define IJK_LOG_WARN        ANDROID_LOG_WARN
#define IJK_LOG_ERROR       ANDROID_LOG_ERROR
#define IJK_LOG_FATAL       ANDROID_LOG_FATAL
#define IJK_LOG_SILENT      ANDROID_LOG_SILENT
#define ALOG(level, TAG, ...)    ((void)__android_log_print(level, TAG, __VA_ARGS__))
#define CLASS_PATH "tv/danmaku/ijk/media/player/ffmpeg/cmd/FFmpeg"

static JavaVM *gJVM = NULL;
static jclass gClass = NULL;
static jobject gObject;
static jmethodID gOn_time_progress_id;

static void jni_init(JNIEnv *env, jobject thiz) {
   gObject = (*env)->NewGlobalRef(env, thiz);
   jclass clazz = (*env)->GetObjectClass(env, thiz);
   if (clazz == NULL) {
       (*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
   }
   gOn_time_progress_id = (*env)->GetMethodID(env, clazz, "onNativeProgress", "(Ljava/lang/String;)V");
   if(!gOn_time_progress_id) {
       logw(tag, "Register native callback failure");
   }
}
inline static int log_level_av_to_ijk(int av_level)
{
    int ijk_level = IJK_LOG_VERBOSE;
    if      (av_level <= AV_LOG_PANIC)      ijk_level = IJK_LOG_FATAL;
    else if (av_level <= AV_LOG_FATAL)      ijk_level = IJK_LOG_FATAL;
    else if (av_level <= AV_LOG_ERROR)      ijk_level = IJK_LOG_ERROR;
    else if (av_level <= AV_LOG_WARNING)    ijk_level = IJK_LOG_WARN;
    else if (av_level <= AV_LOG_INFO)       ijk_level = IJK_LOG_INFO;
        // AV_LOG_VERBOSE means detailed info
    else if (av_level <= AV_LOG_VERBOSE)    ijk_level = IJK_LOG_INFO;
    else if (av_level <= AV_LOG_DEBUG)      ijk_level = IJK_LOG_DEBUG;
    else if (av_level <= AV_LOG_TRACE)      ijk_level = IJK_LOG_VERBOSE;
    else                                    ijk_level = IJK_LOG_VERBOSE;
    return ijk_level;
}
static void on_cmd_progress(char *ret) {
	static char timeStr[10] = "time=";
	char *q = strstr(ret, timeStr);
	if(q != NULL) {//日志信息中若包含"time="字符串
    	char str[32] = {0};
    	strncpy(str, q, 16);
    	char *p = &str[5];
         //time=00:00:11.70
        //str=time=00:00:11, progress: 11
        //loge(tag, "str=%s, progress: %d, p=%s", str, result, p);
        JNIEnv *env = NULL;
        jboolean isAttached = JNI_FALSE;

        if ((*gJVM)->GetEnv(gJVM, (void**) &env, JNI_VERSION_1_4) < 0)
        {
            if ((*gJVM)->AttachCurrentThread(gJVM, &env, NULL) < 0)
            {
                loge(tag, "AttachCurrentThread failed");
                return ;
            }
            isAttached = JNI_TRUE;
        }
        jstring jstr = (*env)->NewStringUTF(env, p);
        (*env)->CallVoidMethod (env, gObject, gOn_time_progress_id, jstr);
        (*env)->DeleteLocalRef(env, jstr);
        if (isAttached)
            (*gJVM)->DetachCurrentThread(gJVM);
    }
}
static void ffp_log_callback_report(void *ptr, int level, const char *fmt, va_list vl)
{
    if (level > av_log_get_level())
        return;
    int ffplv __unused = log_level_av_to_ijk(level);

    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);

    on_cmd_progress(line);
    //logi(tag, "%s", line);
    ALOG(ffplv, tag, "%s", line);
}

static void jni_enable_log(JNIEnv *env, jobject type, jboolean enable) {
    if (enable) {
        av_log_set_callback(ffp_log_callback_report);
    } else {
        av_log_set_callback(NULL);
    }
}

static jboolean jni_run_command(JNIEnv *env, jobject type, jobjectArray commands){
    int argc = (*env)->GetArrayLength(env, commands);
    char *argv[argc];
    jstring jstr[argc];

    for (int i = 0; i < argc; i++) {
        jstr[i] = (jstring) (*env)->GetObjectArrayElement(env,commands, i);
        argv[i] = (char *) (*env)->GetStringUTFChars(env, jstr[i], 0);
    }
    jboolean status = (jboolean) (execute_cmd(argc, argv) == 0);
    for (int i = 0; i < argc; ++i)
    {
        (*env)->ReleaseStringUTFChars(env, jstr[i], argv[i]);
    }
    return status;
}
static JNINativeMethod native_methods[] = {
        { "init",                   "()V",        (void *) jni_init},
        { "enableLog",              "(Z)V",    (void *) jni_enable_log},
        { "runCommand",             "([Ljava/lang/String;)Z", (void *) jni_run_command},

};
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        loge(tag, "GetEnv failed!");
        return JNI_ERR;
    }

    jclass klass = (*env)->FindClass (env, CLASS_PATH);
    if (klass == NULL)
    {
        loge(tag, "FindClass class failed!!");
        return JNI_ERR;
    }
    gClass = (*env)->NewGlobalRef(env, klass);
    (*env)->GetJavaVM(env, &gJVM);
    (*env)->RegisterNatives (env, klass, native_methods, (int)(sizeof(native_methods)/sizeof(native_methods[0])));

    return JNI_VERSION_1_4;
}