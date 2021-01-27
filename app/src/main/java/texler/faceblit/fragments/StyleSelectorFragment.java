package texler.faceblit.fragments;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import texler.faceblit.IRecyclerViewCallback;
import texler.faceblit.R;
import texler.faceblit.RecyclerViewAdapter;
import texler.faceblit.utils.BitmapHelper;
import texler.faceblit.utils.ResourceHelper;

public class StyleSelectorFragment extends Fragment { //implements IRecyclerViewCallback {

    private static final String TAG = "StyleSelectorFragment";

    private static StyleSelectorFragment mInstance = null;
    private RecyclerView mRecyclerView;
    private String mStyleLandmarks = null;
    private byte[] mStyleBitmapBytes = null;
    private byte[] mLookupCubeBytes = null;
    private boolean mStyleChanged = false;
    private boolean mVisibleBeforeGalleryEntrance = false;

    // Singleton
    public static StyleSelectorFragment getInstance() {
        if (mInstance == null) {
            mInstance = new StyleSelectorFragment();
        }
        return mInstance;
    }
    private StyleSelectorFragment() { }


    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        //return super.onCreateView(inflater, container, savedInstanceState);
        return inflater.inflate(R.layout.fragment_style_selector, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRecyclerView = view.findViewById(R.id.style_recycler_view);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity(), LinearLayoutManager.HORIZONTAL, false));
        mRecyclerView.setAdapter(new RecyclerViewAdapter(getActivity(), ResourceHelper.getRecyclerViewItemsNames()));
    }

    //@Override
    public void onStyleImageClick(String styleName) {
        setStyleData(styleName);
    }

    synchronized public void setStyleData(String styleName)    {
        int[] resources = ResourceHelper.getRelatedResources(styleName); // [0] - id of a style img, [1] - id of its landmarks, [2] - id of its lookup table

        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false; // suppress automatic scaling
        Bitmap mStyleBitmap = BitmapFactory.decodeResource(getResources(), resources[0], options);
        mStyleBitmapBytes = BitmapHelper.bitmapToBytes(mStyleBitmap);

        mStyleLandmarks = ResourceHelper.getLandmarksFromResource(resources[1], requireContext());
        mLookupCubeBytes = ResourceHelper.getCubeFromResource(resources[2], requireContext());
        mStyleChanged = true;
    }

    // Getters
    public String getStyleLandmarks() {
        return mStyleLandmarks;
    }

    public byte[] getStyleBitmapBytes() {
        return mStyleBitmapBytes;
    }

    public byte[] getLookupCubeBytes() {
        return mLookupCubeBytes;
    }

    public boolean isStyleChanged() {
        return mStyleChanged;
    }

    public boolean isVisibleBeforeGalleryEntrance() {
        return mVisibleBeforeGalleryEntrance;
    }

    // Setters
    public void setStyleChanged(boolean styleChanged) {
        this.mStyleChanged = styleChanged;
    }

    public void setVisibleBeforeGalleryEntrance(boolean visibleBeforeGalleryEntrance) {
        this.mVisibleBeforeGalleryEntrance = visibleBeforeGalleryEntrance;
    }
}
