package texler.faceblit.utils;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.media.Image;

import org.jetbrains.annotations.NotNull;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

public class BitmapHelper {

    public static Bitmap imageToBitmap(@NotNull Image image) {
        ByteBuffer yBuffer = image.getPlanes()[0].getBuffer();
        ByteBuffer uBuffer = image.getPlanes()[1].getBuffer();
        ByteBuffer vBuffer = image.getPlanes()[2].getBuffer();

        int ySize = yBuffer.remaining();
        int uSize = uBuffer.remaining();
        int vSize = vBuffer.remaining();

        byte[] nv21 = new byte[ySize + uSize + vSize];
        //U and V are swapped
        yBuffer.get(nv21, 0, ySize);
        vBuffer.get(nv21, ySize, vSize);
        uBuffer.get(nv21, ySize + vSize, uSize);

        YuvImage yuvImage = new YuvImage(nv21, ImageFormat.NV21, image.getWidth(), image.getHeight(), null);
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        yuvImage.compressToJpeg(new Rect(0, 0, yuvImage.getWidth(), yuvImage.getHeight()), 75, out);

        byte[] imageBytes = out.toByteArray();
        return BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);
        /*
        final ByteBuffer byteBuffer = image.getPlanes()[0].getBuffer(); // Image.Plane[] - color channels of a pixel
        byteBuffer.rewind();
        final byte[] bytes = new byte[byteBuffer.capacity()];
        byteBuffer.get(bytes);
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false; // suppress automatic scaling
        //options.outColorSpace = Bitmap.Config.ARGB_8888;
        return BitmapFactory.decodeByteArray(bytes, 0, bytes.length, null);*/
    }

    public static Bitmap landscapeToPortraitRotationRight(Bitmap src) {
        Matrix matrix = new Matrix();
        matrix.postRotate(-90);
        return Bitmap.createBitmap(src, 0, 0, src.getWidth(), src.getHeight(), matrix, true);
    }

    public static Bitmap landscapeToPortraitRotationLeft(Bitmap src) {
        Matrix matrix = new Matrix();
        matrix.postRotate(90);
        return Bitmap.createBitmap(src, 0, 0, src.getWidth(), src.getHeight(), matrix, true);
    }

    public static Bitmap bitmapHorizontalFlip(Bitmap src) {
        Matrix matrix = new Matrix();
        matrix.setScale(-1, 1);
        return Bitmap.createBitmap(src, 0, 0, src.getWidth(), src.getHeight(), matrix, true);
    }

    @NotNull
    public static byte[] bitmapToBytes(@NotNull Bitmap bitmap) {
        int size = bitmap.getRowBytes() * bitmap.getHeight();
        ByteBuffer buffer = ByteBuffer.allocate(size);
        bitmap.copyPixelsToBuffer(buffer);
        return buffer.array();
    }

    @NotNull
    public static Bitmap bytesToBitmap(byte[] bitmapBytes, int width, int height, Bitmap.Config config) {
        //Bitmap.Config config = Bitmap.Config.valueOf(mTargetBitmap.getConfig().name());
        Bitmap bitmap = Bitmap.createBitmap(width, height, config);
        ByteBuffer buffer = ByteBuffer.wrap(bitmapBytes);
        bitmap.copyPixelsFromBuffer(buffer);
        return bitmap;
    }
}
