//yuv420sp2rgb.h

static inline void color_convert_common(
    unsigned char *pY, unsigned char *pUV,
    int width, int height, int texture_size,
    unsigned char *buffer,
    int size, /* buffer size in bytes */
    int gray,
    int rotate);

JNIEXPORT void JNICALL Java_com_android_bluetooth_robocam_PreviewSurface_yuv420sp2rgb
  (JNIEnv *, jobject, jbyteArray, jint, jint, jint, jbyteArray);
