package texler.faceblit;


import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.LifecycleOwner;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.FrameLayout;

import java.io.File;

import texler.faceblit.utils.BitmapHelper;
import texler.faceblit.utils.ResourceHelper;
import texler.faceblit.utils.ViewExtensions;


/**
 * Main entry point into our app. This app follows the single-activity pattern, and all
 * functionality is implemented in the form of fragments.
 */
public class MainActivity extends AppCompatActivity implements IRecyclerViewCallback {

    public static final String KEY_EVENT_ACTION = "key_event_action";
    public static final String KEY_EVENT_EXTRA = "key_event_extra";
    private static final long IMMERSIVE_FLAG_TIMEOUT = 500L;

    private FrameLayout mContainer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mContainer = findViewById(R.id.fragment_container);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Before setting full screen flags, we must wait a bit to let UI settle; otherwise, we may
        // be trying to set app to immersive mode before it's ready and the flags do not stick
        mContainer.postDelayed(new Runnable() {
            @Override
            public void run() {
                mContainer.setSystemUiVisibility(ViewExtensions.FLAGS_FULLSCREEN);
            }
        }, IMMERSIVE_FLAG_TIMEOUT);
    }

    /** When key down event is triggered, relay it via local broadcast so fragments can handle it */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            Intent intent = new Intent(KEY_EVENT_ACTION).putExtra(KEY_EVENT_EXTRA, keyCode);
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            return true;
        }
        else {
            return super.onKeyDown(keyCode, event);
        }
    }

    /** Use external media if it is available, our app's file directory otherwise */
    public static File getOutputDirectory(Context context) {
        Context appContext = context.getApplicationContext();
        File mediaDir = new File(context.getExternalMediaDirs()[0], appContext.getResources().getString(R.string.app_name));
        if (mediaDir.mkdirs()) {
            return mediaDir;
        }
        else {
            return appContext.getFilesDir();
        }
    }

    @Override
    public void onStyleImageClick(String imageName) {
        // TODO create function instead...maybe new singleton class StylizationManager where will be the implementation
        int[] resources = ResourceHelper.getRelatedResources(imageName); // [0] - id of a style img, [1] - id of its landmarks, [2] - id of its lookup table
        String mStyleLandmarks = ResourceHelper.getLandmarksFromResource(resources[1], this);
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false; // suppress automatic scaling
        Bitmap mStyleBitmap = BitmapFactory.decodeResource(getResources(), resources[0], options);
        byte[] mStyleBitmapBytes = BitmapHelper.bitmapToBytes(mStyleBitmap);
        byte[] mCubeBytes = ResourceHelper.getCubeFromResource(resources[2], this);
        boolean mStyleChanged = true;
    }













}