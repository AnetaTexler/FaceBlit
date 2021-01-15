package texler.faceblit.fragments;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.hardware.display.DisplayManager;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.webkit.MimeTypeMap;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.camera.core.AspectRatio;
import androidx.camera.core.Camera;
import androidx.camera.core.CameraInfoUnavailableException;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageCapture;
import androidx.camera.core.ImageCaptureException;
import androidx.camera.core.Preview;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.camera.view.PreviewView;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.navigation.Navigation;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.RequestOptions;
import com.google.common.util.concurrent.ListenableFuture;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import texler.faceblit.MainActivity;
import texler.faceblit.R;
import texler.faceblit.StyleTransferAnalyzer;
import texler.faceblit.utils.ViewExtensions;



public class CameraFragment extends Fragment {

    private static final String TAG = "CameraFragment";
    private static final String FILENAME = "yyyyMMdd_HHmmss";
    private static final String PHOTO_EXTENSION = ".jpg";
    private static final Double RATIO_4_3_VALUE = 4.0 / 3.0;
    private static final Double RATIO_16_9_VALUE = 16.0 / 9.0;
    private static final Size STYLE_SIZE = new Size(768, 1024);

    private ConstraintLayout mContainer;
    private PreviewView mPreviewView;
    private File mOutputDir;
    private LocalBroadcastManager mLocalBroadcastManager;

    private int mDisplayId = -1;
    private int mLensFacing = CameraSelector.LENS_FACING_FRONT;
    private Preview mPreview = null;
    private ImageCapture mImageCapture = null;
    private ImageAnalysis mImageAnalysis = null;
    private Camera mCamera = null;
    private ProcessCameraProvider mCameraProvider = null;
    private DisplayManager mDisplayManager = null;
    private AlphaAnimation mAlphaAnimation = null;
    private ImageView mImageView = null; // image view for displaying a stylization result
    private TextView mTextView = null; // test view for displaying a stylization statistics

    /** Blocking camera operations are performed using this executor */
    private ExecutorService mCameraExecutor;

    /** Volume down button receiver used to trigger shutter */
    private BroadcastReceiver mVolumeDownReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // When the volume down button is pressed, simulate a shutter button click
            if (intent.getIntExtra(MainActivity.KEY_EVENT_EXTRA, KeyEvent.KEYCODE_UNKNOWN) == KeyEvent.KEYCODE_VOLUME_DOWN) {
                ImageButton shutter = mContainer.findViewById(R.id.camera_capture_button);
                ViewExtensions.simulateClick(shutter);
            }
        }
    };

    /**
     * We need a display listener for orientation changes that do not trigger a configuration
     * change, for example if we choose to override config change in manifest or for 180-degree
     * orientation changes.
     */
    private DisplayManager.DisplayListener mDisplayListener = new DisplayManager.DisplayListener() {
        @Override
        public void onDisplayAdded(int displayId) {
        }

        @Override
        public void onDisplayRemoved(int displayId) {
        }

        @Override
        public void onDisplayChanged(int displayId) {
            if (displayId == mDisplayId) {
                Log.d(TAG, "Rotation changed: ${getView().getDisplay().getRotation()}");
                mImageCapture.setTargetRotation(getView().getDisplay().getRotation());
                mImageAnalysis.setTargetRotation(getView().getDisplay().getRotation());
            }
        }
    };


    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mDisplayManager = (DisplayManager) requireContext().getSystemService(Context.DISPLAY_SERVICE);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        //return super.onCreateView(inflater, container, savedInstanceState);
        return inflater.inflate(R.layout.fragment_camera, container, false);
    }

    @SuppressLint("MissingPermission")
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mContainer = (ConstraintLayout) view;
        mPreviewView = mContainer.findViewById(R.id.view_finder);
        mCameraExecutor = Executors.newSingleThreadExecutor(); // Initialize our background executor
        mLocalBroadcastManager = LocalBroadcastManager.getInstance(view.getContext());
        // Set up the intent filter that will receive events from our main activity
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(MainActivity.KEY_EVENT_ACTION);
        mLocalBroadcastManager.registerReceiver(mVolumeDownReceiver, intentFilter);
        // Every time the orientation of device changes, update rotation for use cases
        mDisplayManager.registerDisplayListener(mDisplayListener, null);
        // Determine the output directory
        mOutputDir = MainActivity.getOutputDirectory(requireContext());
        // Wait for the views to be properly laid out
        mPreviewView.post(new Runnable() {
            @Override
            public void run() {
                mDisplayId = mPreviewView.getDisplay().getDisplayId(); // Keep track of the display in which this view is attached
                updateCameraUi(); // Build UI controls
                setUpCamera(); // Set up the camera and its use cases
            }
        });
        mAlphaAnimation = new AlphaAnimation(1F, 0.4F);
        mImageView = mContainer.findViewById(R.id.stylized_image_view);
        mTextView = mContainer.findViewById(R.id.text_view);
    }

    @Override
    public void onResume() {
        super.onResume();
        // Make sure that all permissions are still present, since the user could have removed them while the app was in paused state.
        if (!PermissionsFragment.allPermissionsGranted(requireContext())) {
            Navigation.findNavController(requireActivity(), R.id.fragment_container).navigate(CameraFragmentDirections.actionCameraToPermissions());
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        // Shut down our background executor
        mCameraExecutor.shutdown();
        // Unregister the broadcast receivers and listeners
        mLocalBroadcastManager.unregisterReceiver(mVolumeDownReceiver);
        mDisplayManager.unregisterDisplayListener(mDisplayListener);
    }

    /**
     * Inflate camera controls and update the UI manually upon config changes to avoid removing
     * and re-adding the view finder from the view hierarchy; this provides a seamless rotation
     * transition on devices that support it.
     *
     * NOTE: The flag is supported starting in Android 8 but there still is a small flash on the
     * screen for devices that run Android 9 or below.
     */
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        updateCameraUi(); // Redraw the camera UI controls
        updateCameraSwitchButton(); // Enable or disable switching between cameras
    }

    /** Initialize CameraX, and prepare to bind the camera use cases  */
    private void setUpCamera() {
        ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(requireContext());
        cameraProviderFuture.addListener(new Runnable() {
            @Override
            public void run() {
                try {
                    mCameraProvider = cameraProviderFuture.get();
                    // Select lensFacing depending on the available cameras
                    if (hasFrontCamera()) {
                        mLensFacing = CameraSelector.LENS_FACING_FRONT;
                    }
                    else if (hasBackCamera()) {
                        mLensFacing = CameraSelector.LENS_FACING_BACK;
                    }
                    else {
                        throw new IllegalStateException("Back and front camera are unavailable.");
                    }

                    updateCameraSwitchButton(); // Enable or disable switching between cameras

                } catch (ExecutionException | InterruptedException | CameraInfoUnavailableException e) {
                    e.printStackTrace();
                }

                bindCameraUseCases(); // Build and bind the camera use cases
            }
        }, ContextCompat.getMainExecutor(requireContext()));
    }

    /** Declare and bind preview, capture and analysis use cases */
    private void bindCameraUseCases() {
        // Get screen metrics used to setup camera for full screen resolution
        DisplayMetrics displayMetrics = new DisplayMetrics();
        mPreviewView.getDisplay().getRealMetrics(displayMetrics);
        Log.d(TAG, "Screen metrics: " + displayMetrics.widthPixels + " x " + displayMetrics.heightPixels);

        int screenAspectRatio = aspectRatio(displayMetrics.widthPixels, displayMetrics.heightPixels);
        Log.d(TAG, "Preview aspect ratio: " + screenAspectRatio);

        int rotation = mPreviewView.getDisplay().getRotation();

        // CameraProvider
        ProcessCameraProvider cameraProvider = mCameraProvider;
        if (cameraProvider == null) throw new IllegalStateException("Camera initialization failed.");

        // CameraSelector
        CameraSelector cameraSelector = new CameraSelector.Builder()
                .requireLensFacing(mLensFacing)
                .build();

        // Preview
        mPreview = new Preview.Builder()
                .setTargetAspectRatio(screenAspectRatio) // We request aspect ratio but no resolution
                .setTargetRotation(rotation) // Set initial target rotation
                .build();

        // ImageCapture
        mImageCapture = new ImageCapture.Builder()
                .setCaptureMode(ImageCapture.CAPTURE_MODE_MINIMIZE_LATENCY)
                .setTargetAspectRatio(screenAspectRatio) // We request aspect ratio but no resolution to match preview config, but letting CameraX optimize for whatever specific resolution best fits our use cases
                .setTargetRotation(rotation) // Set initial target rotation, we will have to call this again if rotation changes during the lifecycle of this use case
                .build();

        // ImageAnalysis
        mImageAnalysis = new ImageAnalysis.Builder()
                .setTargetAspectRatio(screenAspectRatio) // We request aspect ratio but no resolution (not possible to set both)
                .setTargetRotation(rotation) // Set initial target rotation, we will have to call this again if rotation changes during the lifecycle of this use case
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST) // In this non-blocking mode, the executor receives the last available frame from the camera at the time that the analyze() method is called. If the method takes longer than the latency of a single frame at the current FPS, some frames might be skipped
                //.setTargetResolution(STYLE_SIZE)
                .build();
        mImageAnalysis.setAnalyzer(mCameraExecutor, new StyleTransferAnalyzer(mLensFacing, mImageView, mTextView));


        // Must unbind the use-cases before rebinding them
        cameraProvider.unbindAll();

        try {
            // A variable number of use-cases can be passed here - camera provides access to CameraControl & CameraInfo
            mCamera = cameraProvider.bindToLifecycle(this, cameraSelector, mPreview, mImageCapture, mImageAnalysis);
            // Attach the viewfinder's surface provider to preview use case
            mPreview.setSurfaceProvider(mPreviewView.getSurfaceProvider());
        }
        catch (Exception e) {
            Log.e(TAG, "Use case binding failed.", e);
        }

    }

    /** Method used to re-draw the camera UI controls, called every time configuration changes. */
    private void updateCameraUi() {
        // Remove previous UI if any
        ConstraintLayout ui;
        if ((ui = mContainer.findViewById(R.id.camera_ui_container)) != null) {
            mContainer.removeView(ui);
        }
        // Inflate a new view containing all UI for controlling the camera
        View controlsView = View.inflate(requireContext(), R.layout.camera_ui_container, mContainer);

        // In the background, load latest photo taken (if any) for gallery thumbnail
        mCameraExecutor.execute(new Runnable() {
            @Override
            public void run() {
                List<File> files = Arrays.asList(Objects.requireNonNull(mOutputDir.listFiles(file ->
                        GalleryFragment.EXTENSION_WHITELIST.contains(file.getName().substring(file.getName().lastIndexOf(".") + 1).toUpperCase(Locale.ROOT)))));
                if (!files.isEmpty()) {
                    Collections.sort(files, Collections.reverseOrder());
                    setGalleryThumbnail(Uri.fromFile(files.get(0)));
                }
            }
        });

        // Listener for button used to capture photo
        controlsView.findViewById(R.id.camera_capture_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // Get a stable reference of the modifiable image capture use case
                assert mImageCapture != null;
                File photoFile = createFile(mOutputDir, FILENAME, PHOTO_EXTENSION); // Create output file to hold the image
                // Setup image capture metadata
                ImageCapture.Metadata metadata = new ImageCapture.Metadata();
                metadata.setReversedHorizontal(mLensFacing == CameraSelector.LENS_FACING_FRONT); // Mirror image when using the front camera
                // Create output options object which contains file + metadata
                ImageCapture.OutputFileOptions outputOptions = new ImageCapture.OutputFileOptions.Builder(photoFile)
                        .setMetadata(metadata)
                        .build();

                // Setup image capture listener which is triggered after photo has been taken
                mImageCapture.takePicture(outputOptions, mCameraExecutor, new ImageCapture.OnImageSavedCallback() {
                    @Override
                    public void onImageSaved(@NonNull ImageCapture.OutputFileResults outputFileResults) {
                        Uri savedUri = (outputFileResults.getSavedUri() != null) ? outputFileResults.getSavedUri() : Uri.fromFile(photoFile);
                        Log.d(TAG, "Photo capture succeeded: " + savedUri);
                        // We can only change the foreground Drawable using API level 23+ API
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                            setGalleryThumbnail(savedUri); // Update the gallery thumbnail with latest picture taken
                        }
                        // Implicit broadcasts will be ignored for devices running API level >= 24 so if you only target API level 24+ you can remove this statement
                        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
                            requireActivity().sendBroadcast(new Intent(android.hardware.Camera.ACTION_NEW_PICTURE, savedUri));
                        }
                        // If the folder selected is an external media directory, this is unnecessary but otherwise other apps will not be able to access our images unless we scan them using [MediaScannerConnection]
                        String mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(savedUri.getPath().substring(savedUri.getPath().lastIndexOf(".") + 1));
                        MediaScannerConnection.scanFile(getContext(), new String[] { savedUri.getPath() }, new String[] { mimeType }, null);
                    }

                    @Override
                    public void onError(@NonNull ImageCaptureException exception) {
                        Log.e(TAG, "Photo capture failed: " + exception.getMessage(), exception);
                    }
                });

                // We can only change the foreground Drawable using API level 23+ API
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    // Display flash animation to indicate that photo was captured
                    mContainer.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            mContainer.setForeground(new ColorDrawable(Color.argb(0.1f,1,1,1)));
                            mContainer.postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    mContainer.setForeground(null);
                                }
                            }, ViewExtensions.ANIMATION_FAST_MILLIS);
                        }
                    }, ViewExtensions.ANIMATION_SLOW_MILLIS);
                }
            }
        });

        // Setup for button used to switch cameras
        ImageButton switchButton = controlsView.findViewById(R.id.camera_switch_button);
        switchButton.setEnabled(false); // Disable the button until the camera is set up
        // Listener for button used to switch cameras. Only called if the button is enabled
        switchButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(mAlphaAnimation);
                mLensFacing = (CameraSelector.LENS_FACING_FRONT == mLensFacing) ? CameraSelector.LENS_FACING_BACK : CameraSelector.LENS_FACING_FRONT;
                bindCameraUseCases(); // Re-bind use cases to update selected camera
            }
        });

        // Listener for button used to view the most recent photo
        controlsView.findViewById(R.id.photo_view_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(mAlphaAnimation);
                // Only navigate when the gallery has photos
                if (Objects.requireNonNull(mOutputDir.listFiles()).length > 0) {
                    Navigation.findNavController(requireActivity(), R.id.fragment_container)
                            .navigate(CameraFragmentDirections.actionCameraToGallery(mOutputDir.getAbsolutePath()));
                }
            }
        });

        // Listener for button used to display the styles selector (recycler view)
        controlsView.findViewById(R.id.style_view_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(mAlphaAnimation);
                StyleSelectorFragment styleSelectorFragment = StyleSelectorFragment.getInstance();
                FragmentTransaction fragmentTransaction = requireActivity().getSupportFragmentManager().beginTransaction()
                        .setCustomAnimations(R.anim.anim_translate_open, R.anim.anim_translate_close, R.anim.anim_translate_open, R.anim.anim_translate_close);
                // The first click - add fragment
                if (!styleSelectorFragment.isAdded()) {
                    fragmentTransaction.add(R.id.fragment_container, styleSelectorFragment)
                            .addToBackStack(null)
                            .commit();
                    return;
                }
                // Toggle between show and hide
                if (styleSelectorFragment.isHidden()) {
                    fragmentTransaction.show(styleSelectorFragment)
                            .addToBackStack(null);
                }
                else {
                    fragmentTransaction.hide(styleSelectorFragment);
                }
                fragmentTransaction.commit();
            }
        });

    }

    /** Returns true if the device has an available back camera. False otherwise */
    private boolean hasBackCamera() throws CameraInfoUnavailableException {
        return mCameraProvider.hasCamera(CameraSelector.DEFAULT_BACK_CAMERA);
    }

    /** Returns true if the device has an available front camera. False otherwise */
    private boolean hasFrontCamera() throws CameraInfoUnavailableException {
        return mCameraProvider.hasCamera(CameraSelector.DEFAULT_FRONT_CAMERA);
    }

    /** Enabled or disabled a button to switch cameras depending on the available cameras */
    private void updateCameraSwitchButton() {
        ImageButton switchCamerasButton = mContainer.findViewById(R.id.camera_switch_button);
        try {
            switchCamerasButton.setEnabled(hasBackCamera() && hasFrontCamera());
        } catch (CameraInfoUnavailableException e) {
            switchCamerasButton.setEnabled(false);
        }
    }

    /**
     *  [androidx.camera.core.ImageAnalysisConfig] requires enum value of
     *  [androidx.camera.core.AspectRatio]. Currently it has values of 4:3 & 16:9.
     *
     *  Detecting the most suitable ratio for dimensions provided in @params by counting absolute
     *  of preview ratio to one of the provided values.
     *
     *  @param width - preview width
     *  @param height - preview height
     *  @return suitable aspect ratio
     */
    private int aspectRatio(int width, int height) {
        //double previewRatio = (double) Math.max(width, height) / Math.min(width, height);
        //if (Math.abs(previewRatio - RATIO_4_3_VALUE) <= Math.abs(previewRatio - RATIO_16_9_VALUE)) {
            return AspectRatio.RATIO_4_3;
        //}
        //return AspectRatio.RATIO_16_9;
    }

    private void setGalleryThumbnail(Uri uri) {
        // Reference of the view that holds the gallery thumbnail
        ImageButton thumbnail = mContainer.findViewById(R.id.photo_view_button);
        // Run the operations in the view's thread
        thumbnail.post(new Runnable() {
            @Override
            public void run() {
                // Remove thumbnail padding
                thumbnail.setPadding((int)getResources().getDimension(R.dimen.stroke_small),
                        (int)getResources().getDimension(R.dimen.stroke_small),
                        (int)getResources().getDimension(R.dimen.stroke_small),
                        (int)getResources().getDimension(R.dimen.stroke_small));
                // Load thumbnail into circular button using Glide
                Glide.with(thumbnail).load(uri).apply(RequestOptions.circleCropTransform()).into(thumbnail);
            }
        });
    }

    /** Helper function used to create a timestamped file */
    private static File createFile(File baseFolder, String format, String extension) {
        SimpleDateFormat mDateFormat = new SimpleDateFormat(format, Locale.US);
        return new File(baseFolder, mDateFormat.format(System.currentTimeMillis()) + extension);
    }

}
