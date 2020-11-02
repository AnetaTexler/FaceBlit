package texler.faceblit;

public class JavaNativeInterface {
    /**
     * A native method that is implemented by the 'native-lib' native library, which is packaged with this application.
     */
    public static native byte[] getStylizedData(String modelPath,
                                                String styleLandmarks,
                                                byte[] lookupTableBytes,
                                                byte[] styleBytes,
                                                byte[] targetBytes,
                                                int w,
                                                int h,
                                                boolean rotateRight,
                                                boolean rotateLeft,
                                                boolean horizontalFlip,
                                                boolean stylizeFaceOnly);
}
