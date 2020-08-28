package texler.faceblit.fragments;

import android.os.Bundle;
import android.provider.ContactsContract;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import texler.faceblit.R;
import com.bumptech.glide.Glide;
import java.io.File;


/** Fragment used for each individual page showing a photo inside of [GalleryFragment] */
public class PhotoFragment extends Fragment {

    private static final String FILE_NAME_KEY = "file_name";

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        //return super.onCreateView(inflater, container, savedInstanceState);
        return new ImageView(getContext());
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Bundle args = this.getArguments();
        if (args == null) return;
        String fileName = args.getString(FILE_NAME_KEY);
        if (fileName != null) {
            Glide.with(view).load(new File(fileName)).placeholder(R.drawable.ic_photo).into((ImageView)view);
        }
        else {
            Glide.with(view).load(R.drawable.ic_photo).into((ImageView)view);
        }
    }

    public static Fragment create(File file) {
        Bundle bundle = new Bundle();
        PhotoFragment photoFragment = new PhotoFragment();
        bundle.putString(FILE_NAME_KEY, file.getAbsolutePath());
        photoFragment.setArguments(bundle);
        return photoFragment;
    }
}
