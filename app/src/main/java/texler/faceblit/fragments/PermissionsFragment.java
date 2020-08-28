package texler.faceblit.fragments;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.navigation.Navigation;

import texler.faceblit.R;


/**
 * The sole purpose of this fragment is to request permissions and, once granted, display the
 * camera fragment to the user.
 */
public class PermissionsFragment extends Fragment {

    private final int PERMISSIONS_REQUEST_CODE = 10;
    private static final String[] PERMISSIONS_REQUIRED = new String[]{Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE};

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (allPermissionsGranted(requireContext())) {
            Navigation.findNavController(requireActivity(), R.id.fragment_container).navigate(PermissionsFragmentDirections.actionPermissionsToCamera());
        }
        else {
            requestPermissions(PERMISSIONS_REQUIRED, PERMISSIONS_REQUEST_CODE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == PERMISSIONS_REQUEST_CODE) {
            if (PackageManager.PERMISSION_GRANTED == grantResults[0] && PackageManager.PERMISSION_GRANTED == grantResults[1]) {
                Toast.makeText(getContext(), "Permission request granted", Toast.LENGTH_LONG).show();
                Navigation.findNavController(requireActivity(), R.id.fragment_container).navigate(PermissionsFragmentDirections.actionPermissionsToCamera());
            } else {
                Toast.makeText(getContext(), "Permission request denied", Toast.LENGTH_LONG).show();
            }
        }
    }

    public static boolean allPermissionsGranted (Context context) {
        for (String permission : PERMISSIONS_REQUIRED){
            if (ContextCompat.checkSelfPermission(context, permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }
}
