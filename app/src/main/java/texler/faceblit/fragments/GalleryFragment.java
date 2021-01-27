package texler.faceblit.fragments;


import android.content.DialogInterface;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;
import androidx.fragment.app.FragmentTransaction;
import androidx.navigation.Navigation;
import androidx.viewpager.widget.ViewPager;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

import texler.faceblit.R;
import texler.faceblit.utils.ViewExtensions;


public class GalleryFragment extends Fragment {

    public static List<String> EXTENSION_WHITELIST = new ArrayList<String>(Collections.singletonList("JPG"));

    private List<File> mMediaList = null;

    /** Adapter class used to present a fragment containing one photo or video as a page */
    private class MediaPagerAdapter extends FragmentStatePagerAdapter {

        public MediaPagerAdapter(@NonNull FragmentManager fm) {
            super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        }

        @Override
        public int getCount() {
            return mMediaList.size();
        }

        @NonNull
        @Override
        public Fragment getItem(int position) {
            return PhotoFragment.create(mMediaList.get(position));
        }

        @Override
        public int getItemPosition(@NonNull Object object) {
            return POSITION_NONE;
        }
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.setRetainInstance(true); // Mark this as a retain fragment, so the lifecycle does not get restarted on config change
        assert getArguments() != null;
        File rootDirectory = new File(GalleryFragmentArgs.fromBundle(getArguments()).getRootDirectory()); // Get root directory of media from navigation arguments

        // Walk through all files in the root directory. We reverse the order of the list to present the last photos first
        mMediaList = new LinkedList<>(Arrays.asList(Objects.requireNonNull(rootDirectory.listFiles(file ->
                EXTENSION_WHITELIST.contains(file.getName().substring(file.getName().lastIndexOf(".") + 1).toUpperCase(Locale.ROOT))))));
        Collections.sort(mMediaList, Collections.reverseOrder());
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_gallery, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        //Checking media files list
        if (mMediaList.isEmpty()) {
            view.findViewById(R.id.delete_button).setEnabled(false);
            view.findViewById(R.id.share_button).setEnabled(false);
        }

        // Populate the ViewPager and implement a cache of two media items
        ViewPager mediaViewPager = view.findViewById(R.id.photo_view_pager);
        mediaViewPager.setOffscreenPageLimit(2);
        mediaViewPager.setAdapter(new MediaPagerAdapter(this.getChildFragmentManager()));

        // Make sure that the cutout "safe area" avoids the screen notch if any
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            // Use extension method to pad "inside" view containing UI using display cutout's bounds
            ViewExtensions.padWithDisplayCutout(view.findViewById(R.id.cutout_safe_area));
        }

        // Handle back button press
        view.findViewById(R.id.back_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Navigation.findNavController(requireActivity(), R.id.fragment_container).navigateUp();
            }
        });

        // Handle share button press
        // TODO

        // Handle delete button press
        view.findViewById(R.id.delete_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                File mediaFile = mMediaList.get(mediaViewPager.getCurrentItem());
                AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(view.getContext(), android.R.style.Theme_Material_Dialog);
                alertDialogBuilder.setTitle(getString(R.string.delete_title))
                        .setMessage(getString(R.string.delete_dialog))
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setNegativeButton(android.R.string.no, null)
                        .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialogInterface, int position) {
                                mediaFile.delete(); // Delete current photo
                                // Send relevant broadcast to notify other apps of deletion
                                MediaScannerConnection.scanFile(view.getContext(), new String[]{mediaFile.getAbsolutePath()}, null, null);
                                // Notify our view pager
                                mMediaList.remove(mediaViewPager.getCurrentItem());
                                Objects.requireNonNull(mediaViewPager.getAdapter()).notifyDataSetChanged();
                                // If all photos have been deleted, return to camera
                                if (mMediaList.isEmpty()) {
                                    Navigation.findNavController(requireActivity(), R.id.fragment_container).navigateUp();
                                }
                            }
                        });
                AlertDialog alertDialog = alertDialogBuilder.create();
                //alertDialog.show();
                ViewExtensions.showImmersive(alertDialog);
            }
        });
    }


}
