package texler.faceblit.utils;

import android.content.Context;
import android.util.Log;

import org.jetbrains.annotations.Contract;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.util.ArrayList;

import texler.faceblit.R;

public class ResourceHelper {

    private static final String TAG = "ResourceHelper";

    @NotNull
    public static ArrayList<String> getRecyclerViewItemsNames() {
        ArrayList<String> names = new ArrayList<>();
        Field[] fields = R.drawable.class.getFields();
        for (Field field : fields) {
            if (field.getName().startsWith("recycler_view"))
                names.add(field.getName());
        }
        return names;
    }


    @NotNull
    @Contract("_ -> new")
    public static int[] getRelatedResources(@NotNull String name) {
        int id_img; // PNG style image
        int id_lm; // TXT style landmarks
        int id_lut; // BYTES lookup table

        switch (name){
            case "bronzestatue":
                id_img = R.drawable.style_bronzestatue_480x640;
                id_lm = R.raw.lm_bronzestatue_480x640;
                id_lut = R.raw.lut_bronzestatue_480x640;
                break;
            case "charcoalman":
                id_img = R.drawable.style_charcoalman_480x640;
                id_lm = R.raw.lm_charcoalman_480x640;
                id_lut = R.raw.lut_charcoalman_480x640;
                break;
            case "expressive":
                id_img = R.drawable.style_expressive_480x640;
                id_lm = R.raw.lm_expressive_480x640;
                id_lut = R.raw.lut_expressive_480x640;
                break;
            case "frank":
                id_img = R.drawable.style_frank_480x640;
                id_lm = R.raw.lm_frank_480x640;
                id_lut = R.raw.lut_frank_480x640;
                break;
            case "girl":
                id_img = R.drawable.style_girl_480x640;
                id_lm = R.raw.lm_girl_480x640;
                id_lut = R.raw.lut_girl_480x640;
                break;
            case "het":
                id_img = R.drawable.style_het_480x640;
                id_lm = R.raw.lm_het_480x640;
                id_lut = R.raw.lut_het_480x640;
                break;
            case "illegalbeauty":
                id_img = R.drawable.style_illegalbeauty_480x640;
                id_lm = R.raw.lm_illegalbeauty_480x640;
                id_lut = R.raw.lut_illegalbeauty_480x640;
                break;
            case "ken":
                id_img = R.drawable.style_ken_480x640;
                id_lm = R.raw.lm_ken_480x640;
                id_lut = R.raw.lut_ken_480x640;
                break;
            case "laurinbust":
                id_img = R.drawable.style_laurinbust_480x640;
                id_lm = R.raw.lm_laurinbust_480x640;
                id_lut = R.raw.lut_laurinbust_480x640;
                break;
            case "lincolnbust":
                id_img = R.drawable.style_lincolnbust_480x640;
                id_lm = R.raw.lm_lincolnbust_480x640;
                id_lut = R.raw.lut_lincolnbust_480x640;
                break;
            case "malevich":
                id_img = R.drawable.style_malevich_480x640;
                id_lm = R.raw.lm_malevich_480x640;
                id_lut = R.raw.lut_malevich_480x640;
                break;
            case "oilman":
                id_img = R.drawable.style_oilman_480x640;
                id_lm = R.raw.lm_oilman_480x640;
                id_lut = R.raw.lut_oilman_480x640;
                break;
            case "oldman":
                id_img = R.drawable.style_oldman_480x640;
                id_lm = R.raw.lm_oldman_480x640;
                id_lut = R.raw.lut_oldman_480x640;
                break;
            case "prisma":
                id_img = R.drawable.style_prisma_480x640;
                id_lm = R.raw.lm_prisma_480x640;
                id_lut = R.raw.lut_prisma_480x640;
                break;
            case "stonebust":
                id_img = R.drawable.style_stonebust_480x640;
                id_lm = R.raw.lm_stonebust_480x640;
                id_lut = R.raw.lut_stonebust_480x640;
                break;
            case "watercolorgirl":
                id_img = R.drawable.style_watercolorgirl_480x640;
                id_lm = R.raw.lm_watercolorgirl_480x640;
                id_lut = R.raw.lut_watercolorgirl_480x640;
                break;
            case "woodenmask":
                id_img = R.drawable.style_woodenmask_480x640;
                id_lm = R.raw.lm_woodenmask_480x640;
                id_lut = R.raw.lut_woodenmask_480x640;
                break;
            default:
                Log.e("ERROR","style: " + name + " is not listed in id-switch-case");
                id_img = R.drawable.style_bronzestatue_480x640;
                id_lm = R.raw.lm_bronzestatue_480x640;
                id_lut = R.raw.lut_bronzestatue_480x640;
                break;
        }

        return new int[] {id_img, id_lm, id_lut};
    }

    @Nullable
    public static String getLandmarksFromResource(int id, @NotNull Context context) {
        InputStream inputStream = context.getResources().openRawResource(id);
        InputStreamReader inputReader = new InputStreamReader(inputStream);
        BufferedReader buffReader = new BufferedReader(inputReader);
        StringBuilder text = new StringBuilder();
        String line;
        try {
            while ((line = buffReader.readLine()) != null) {
                text.append(line);
                text.append('\n');
            }
        } catch (IOException e) {
            Log.e(TAG, "Error while reading landmarks txt file.");
            return null;
        }
        return text.toString();
    }

    @NotNull
    public static byte[] getCubeFromResource(int id, @NotNull Context context) {
        InputStream inputStream = context.getResources().openRawResource(id);
        ByteArrayOutputStream os = new ByteArrayOutputStream();

        byte[] buffer = new byte[1024];
        int len;
        try {
            // read bytes from the input stream and store them in buffer
            while ((len = inputStream.read(buffer)) != -1) {
                os.write(buffer, 0, len); // write bytes from the buffer into output stream
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        return os.toByteArray();
    }
}
