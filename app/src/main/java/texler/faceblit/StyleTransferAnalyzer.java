package texler.faceblit;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.Image;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.Size;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageProxy;


import java.io.File;
import java.nio.ByteBuffer;
import java.text.DecimalFormat;
import java.util.ArrayDeque;
import java.util.concurrent.atomic.AtomicBoolean;

import texler.faceblit.fragments.StyleSelectorFragment;
import texler.faceblit.utils.BitmapHelper;


public class StyleTransferAnalyzer implements ImageAnalysis.Analyzer {

    private static final String TAG = "StyleTransferAnalyzer";
    private static final Size STYLE_SIZE = new Size(768, 1024);

    private final String mModelPath; // path to face landmarks pre-trained model
    private byte[] mStylizedBytes; // stylized image
    private Bitmap mStylizedBitmap; // stylized image

    private int mLensFacing;
    private boolean mStylizeFaceOnly = false;
    private ImageView mImageView;
    private TextView mTextView;
    private StringBuilder mStatistics;

    private Handler mHandler;
    //private AtomicBoolean isAnalyzing = new AtomicBoolean(false);

    private int mFrameRateWindow = 8;
    private ArrayDeque<Long> mFrameTimestamps;
    //private long mLastAnalyzedTimestamp = 0L;
    //private double mFPS = -1.0;

    private DecimalFormat mDecimalFormat;


    public StyleTransferAnalyzer(int lensFacing, ImageView imageView, TextView textView) {

        this.mLensFacing = lensFacing;
        this.mImageView = imageView;
        this.mTextView = textView;
        this.mStatistics = new StringBuilder();
        this.mModelPath = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "models").getAbsolutePath();
        this.mHandler = new Handler(Looper.getMainLooper());
        this.mFrameTimestamps = new ArrayDeque<>(5);
        this.mDecimalFormat = new DecimalFormat("#.##");
    }

    @Override
    public void analyze(@NonNull ImageProxy imageProxy) { // CameraX produces images in YUV_420_888 format
        if (imageProxy == null) return;
        //if (isAnalyzing.get()) return;
        //isAnalyzing.set(true);
        //if (StyleSelectorFragment.getInstance().getStyleLandmarks() == null) // style is not set yet
        //    return;

        // Keep track of frames analyzed and compute FPS
        long currentTime = System.currentTimeMillis();
        mFrameTimestamps.push(currentTime);
        mStatistics.append("FPS: ").append(mDecimalFormat.format(getFPS(currentTime)));

        ByteBuffer byteBuffer = imageProxy.getPlanes()[0].getBuffer();
        //byteBuffer.rewind();
        byte[] targetBytes = new byte[byteBuffer.remaining()]; // target image
        byteBuffer.get(targetBytes);
        Bitmap b = BitmapFactory.decodeByteArray(targetBytes, 0, targetBytes.length);

        int width = imageProxy.getWidth();
        int height = imageProxy.getHeight();


        //@SuppressLint("UnsafeExperimentalUsageError") Image image = imageProxy.getImage();
        //Bitmap bitmap = BitmapHelper.imageToBitmap(image);

        //if (mLensFacing == CameraSelector.LENS_FACING_FRONT) {
        //    bitmap = BitmapHelper.landscapeToPortraitRotationRight(bitmap);
        //    bitmap = BitmapHelper.bitmapHorizontalFlip(bitmap);
        //}
        //if (mLensFacing == CameraSelector.LENS_FACING_BACK) {
        //    bitmap = BitmapHelper.landscapeToPortraitRotationLeft(bitmap);
        //}

        /*mStylizedBytes = JavaNativeInterface.getStylizedData(
                mModelPath,
                StyleSelectorFragment.getInstance().getStyleLandmarks(),
                StyleSelectorFragment.getInstance().getLookupCubeBytes(),
                StyleSelectorFragment.getInstance().getStyleBitmapBytes(),
                targetBytes,
                width,
                height,
                mLensFacing,
                mStylizeFaceOnly);*/

        //mStylizedBytes = targetBytes;

        //StyleSelectorFragment.getInstance().setStyleChanged(false);
        // Convert bytes to bitmap
        //if (mStylizedBytes != null) // Override mStylizedBitmap by stylized result only when stylization was successful
            //mStylizedBitmap = BitmapFactory.decodeByteArray(mStylizedBytes, 0, mStylizedBytes.length);
            //mStylizedBitmap = BitmapHelper.bytesToBitmap(mStylizedBytes, width, height, Bitmap.Config.ARGB_8888);

        mHandler.post(new Runnable() { // run on the next run loop on the main thread.
            @Override
            public void run() {
                mImageView.setImageBitmap(b);
                mTextView.setText(mStatistics);
            }
        });

        imageProxy.close(); // IMPORTANT!
        mStatistics.setLength(0); // clear the StringBuilder
    }


    private double getFPS(long currentTime) { // Compute the FPS using a moving average
        while (mFrameTimestamps.size() >= mFrameRateWindow)
            mFrameTimestamps.removeLast();
        long timestampFirst = mFrameTimestamps.isEmpty() ? currentTime : mFrameTimestamps.peekFirst();
        long timestampLast = mFrameTimestamps.isEmpty() ? currentTime : mFrameTimestamps.peekLast();
        return 1.0 / (Math.max(1, (timestampFirst - timestampLast)) / Math.max(1.0, mFrameTimestamps.size())) * 1000.0;
    }
}
