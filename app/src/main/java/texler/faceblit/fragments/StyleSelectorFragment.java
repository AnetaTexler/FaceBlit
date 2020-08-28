package texler.faceblit.fragments;

import android.graphics.Paint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import texler.faceblit.R;
import texler.faceblit.RecyclerViewAdapter;
import texler.faceblit.utils.ResourceHelper;

public class StyleSelectorFragment extends Fragment {

    private static final String TAG = "StyleSelectorFragment";

    private static StyleSelectorFragment mInstance = null;
    private RecyclerView mRecyclerView;

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

}
