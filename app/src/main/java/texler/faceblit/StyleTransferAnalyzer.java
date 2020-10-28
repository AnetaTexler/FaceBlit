package texler.faceblit;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.media.Image;
import android.util.Size;

import androidx.annotation.NonNull;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageProxy;


import texler.faceblit.utils.BitmapHelper;


public class StyleTransferAnalyzer implements ImageAnalysis.Analyzer {

    private static final String TAG = "StyleTransferAnalyzer";
    private static final Size STYLE_SIZE = new Size(768, 1024);

    private int mLensFacing;

    public StyleTransferAnalyzer(int lensFacing) {
        this.mLensFacing = lensFacing;
    }

    @Override
    public void analyze(@NonNull ImageProxy imageProxy) {
        // TODO

        byte[] imageData = imageProxy.getPlanes()[0].getBuffer().array();
        // JNI - mLensFacing, imageProxy.getWidth(), imageProxy.getHeight()

        @SuppressLint("UnsafeExperimentalUsageError") Image image = imageProxy.getImage();
        Bitmap bitmap = BitmapHelper.imageToBitmap(image);
        if (mLensFacing == CameraSelector.LENS_FACING_FRONT) {
            bitmap = BitmapHelper.landscapeToPortraitRotationRight(bitmap);
            bitmap = BitmapHelper.bitmapHorizontalFlip(bitmap);
        }
        if (mLensFacing == CameraSelector.LENS_FACING_BACK) {
            bitmap = BitmapHelper.landscapeToPortraitRotationLeft(bitmap);
        }
        imageProxy.close();

        // JNI
        // statistics

        // mImageView.setImageBitmap(mTargetBitmap);
        // mTextView.setText(mStatisticsBuilder);
    }
}
