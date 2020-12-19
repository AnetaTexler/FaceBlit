package texler.faceblit;

public class JavaNativeInterface {
    /**
     * A native method that is implemented by the 'native-lib' native library, which is packaged with this application.
     */
    public static native byte[] getStylizedData(String modelPath,
                                                String styleLandmarks,
                                                byte[] lookupCubeBytes,
                                                byte[] styleBytes,
                                                byte[] targetBytes,
                                                int w,
                                                int h,
                                                int lensFacing,
                                                boolean stylizeFaceOnly);
}
