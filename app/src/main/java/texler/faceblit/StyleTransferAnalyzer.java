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
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

import texler.faceblit.fragments.StyleSelectorFragment;
import texler.faceblit.utils.BitmapHelper;


public class StyleTransferAnalyzer implements ImageAnalysis.Analyzer {

    private static final String TAG = "StyleTransferAnalyzer";
    private static final Size STYLE_SIZE = new Size(768, 1024);

    private final String mModelPath; // path to face landmarks pre-trained model
    private byte[] mStylizedBytes; // stylized image
    private Bitmap mStylizedBitmap; // stylized image
    private BitmapFactory.Options mBitmapOptions;

    private int mLensFacing;
    private boolean mStylizeFaceOnly = true;
    private ImageView mImageView;
    private TextView mTextView;
    private StringBuilder mStatistics;

    private final Handler mHandler;
    //private AtomicBoolean isAnalyzing = new AtomicBoolean(false);

    private final int mFrameRateWindow = 8;
    private final ArrayDeque<Long> mFrameTimestamps;
    //private long mLastAnalyzedTimestamp = 0L;
    //private double mFPS = -1.0;

    private final DecimalFormat mDecimalFormat;


    public StyleTransferAnalyzer(int lensFacing, ImageView imageView, TextView textView) {

        this.mLensFacing = lensFacing;
        this.mImageView = imageView;
        this.mTextView = textView;
        this.mStatistics = new StringBuilder();
        this.mModelPath = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "models").getAbsolutePath();
        this.mHandler = new Handler(Looper.getMainLooper());
        this.mFrameTimestamps = new ArrayDeque<>(5);
        this.mDecimalFormat = new DecimalFormat("#.##");
        this.mBitmapOptions = new BitmapFactory.Options();
        this.mBitmapOptions.outConfig = Bitmap.Config.ARGB_8888;
    }

    @SuppressLint("UnsafeExperimentalUsageError") // ImageProxy.getImage() is experimental
    @Override
    public void analyze(@NonNull ImageProxy imageProxy) { // CameraX produces images in YUV_420_888 format. The default analyzer resolution is 640x480
        if (imageProxy == null) return;
        //if (isAnalyzing.get()) return;
        //isAnalyzing.set(true);
        if (StyleSelectorFragment.getInstance().getStyleLandmarks() == null) { // style is not set yet
            imageProxy.close();
            return;
        }

        // Keep track of frames analyzed and compute FPS
        long currentTime = System.currentTimeMillis();
        mFrameTimestamps.push(currentTime);
        mStatistics.setLength(0); // clear the StringBuilder
        mStatistics.append("FPS: ").append(mDecimalFormat.format(getFPS(currentTime)));

        Bitmap targetBitmap = BitmapHelper.imageToBitmap(imageProxy.getImage());
        byte[] targetBytes = BitmapHelper.bitmapToBytes(targetBitmap); // pixels

        /*if (mLensFacing == CameraSelector.LENS_FACING_FRONT) { // 0
            targetBitmap = BitmapHelper.landscapeToPortraitRotationRight(targetBitmap);
            targetBitmap = BitmapHelper.bitmapHorizontalFlip(targetBitmap);
        }
        if (mLensFacing == CameraSelector.LENS_FACING_BACK) { // 1
            targetBitmap = BitmapHelper.landscapeToPortraitRotationLeft(targetBitmap);
        }*/

        mStylizedBytes = JavaNativeInterface.getStylizedData(
                mModelPath,
                StyleSelectorFragment.getInstance().getStyleLandmarks(),
                StyleSelectorFragment.getInstance().getLookupCubeBytes(),
                StyleSelectorFragment.getInstance().getStyleBitmapBytes(),
                targetBytes,
                imageProxy.getWidth(),
                imageProxy.getHeight(),
                mLensFacing,
                mStylizeFaceOnly);

        StyleSelectorFragment.getInstance().setStyleChanged(false);

        // Convert bytes to bitmap (override mStylizedBitmap by stylized result only when stylization was successful)
        // An ARGB_8888 Bitmap (on pre Honeycomb versions) is natively stored in the RGBA format.
        if (mStylizedBytes != null)
        {
            mStylizedBitmap = BitmapHelper.bytesToBitmap(mStylizedBytes, 768/*targetBitmap.getWidth()*/, 1024/*targetBitmap.getHeight()*/, targetBitmap.getConfig());
            //mStylizedBitmap = BitmapFactory.decodeByteArray(mStylizedBytes, 0, mStylizedBytes.length);
        }

        mHandler.post(new Runnable() { // run on the next run loop on the main thread.
            @Override
            public void run() {
                mImageView.setImageBitmap(mStylizedBitmap);
                mTextView.setText(mStatistics);
            }
        });

        imageProxy.close(); // IMPORTANT!
    }


    private double getFPS(long currentTime) { // Compute the FPS using a moving average
        while (mFrameTimestamps.size() >= mFrameRateWindow)
            mFrameTimestamps.removeLast();
        long timestampFirst = mFrameTimestamps.isEmpty() ? currentTime : mFrameTimestamps.peekFirst();
        long timestampLast = mFrameTimestamps.isEmpty() ? currentTime : mFrameTimestamps.peekLast();
        return 1.0 / (Math.max(1, (timestampFirst - timestampLast)) / Math.max(1.0, mFrameTimestamps.size())) * 1000.0;
    }
}
