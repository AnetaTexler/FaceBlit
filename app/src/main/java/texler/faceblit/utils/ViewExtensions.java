package texler.faceblit.utils;

import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.widget.ImageButton;
import android.os.Build;
import android.view.DisplayCutout;
import android.view.WindowManager;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AlertDialog;

import org.jetbrains.annotations.NotNull;


public class ViewExtensions {

    /** Combination of all flags required to put activity into immersive mode. */
    public static final int FLAGS_FULLSCREEN =
            View.SYSTEM_UI_FLAG_LOW_PROFILE |
                    View.SYSTEM_UI_FLAG_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;

    /** Milliseconds used for UI animations */
    public static final long ANIMATION_FAST_MILLIS = 50L;
    public static final long ANIMATION_SLOW_MILLIS = 100L;


    /** Simulate a button click, including a small delay while it is being pressed to trigger the appropriate animations. */
    public static void simulateClick(@NotNull ImageButton imageButton, long delay) {

        imageButton.performClick();
        imageButton.setPressed(true);
        imageButton.invalidate();
        imageButton.postDelayed(new Runnable() {
            @Override
            public void run() {
                imageButton.invalidate();
                imageButton.setPressed(false);
            }
        }, delay);
    }

    public static void simulateClick(@NotNull ImageButton imageButton) {
        simulateClick(imageButton, ANIMATION_FAST_MILLIS);
    }

    /** Pad this view with the insets provided by the device cutout (i.e. notch) */
    @RequiresApi(Build.VERSION_CODES.P)
    public static void padWithDisplayCutout(@NotNull View view) {
        // Apply padding using the display cutout designated "safe area"
        DisplayCutout dc = view.getRootWindowInsets().getDisplayCutout();
        if (dc != null) {
            doPadding(view, dc);
        }

        // Set a listener for window insets since view.rootWindowInsets may not be ready yet
        view.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
            @Override
            public WindowInsets onApplyWindowInsets(View view, WindowInsets windowInsets) {
                DisplayCutout dc = windowInsets.getDisplayCutout();
                if (dc != null) {
                    doPadding(view, dc);
                }
                return windowInsets;
            }
        });
    }

    /** Helper method that applies padding from cutout's safe insets */
    private static void doPadding(@NotNull View view, @NotNull DisplayCutout cutout) {
         view.setPadding(cutout.getSafeInsetLeft(), cutout.getSafeInsetTop(), cutout.getSafeInsetRight(), cutout.getSafeInsetBottom());
    }

    /** Same as [AlertDialog.show] but setting immersive mode in the dialog's window */
    public static void showImmersive(@NotNull AlertDialog alertDialog) {
        Window window = alertDialog.getWindow();
        assert window != null;
        window.setFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE); // Set the dialog to not focusable
        window.getDecorView().setSystemUiVisibility(FLAGS_FULLSCREEN); // Make sure that the dialog's window is in full screen
        alertDialog.show(); // Show the dialog while still in immersive mode
        window.clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE); // Set the dialog to focusable again
    }

}
