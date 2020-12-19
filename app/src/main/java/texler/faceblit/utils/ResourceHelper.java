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

    private static final String TAG = "DataHelper";

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
                id_img = R.drawable.style_bronzestatue;
                id_lm = R.raw.lm_bronzestatue;
                id_lut = R.raw.lut_bronzestatue;
                break;
            case "charcoalman":
                id_img = R.drawable.style_charcoalman;
                id_lm = R.raw.lm_charcoalman;
                id_lut = R.raw.lut_charcoalman;
                break;
            case "expressive":
                id_img = R.drawable.style_expressive;
                id_lm = R.raw.lm_expressive;
                id_lut = R.raw.lut_expressive;
                break;
            case "fonz":
                id_img = R.drawable.style_fonz;
                id_lm = R.raw.lm_fonz;
                id_lut = R.raw.lut_fonz;
                break;
            case "frank":
                id_img = R.drawable.style_frank;
                id_lm = R.raw.lm_frank;
                id_lut = R.raw.lut_frank;
                break;
            case "girl":
                id_img = R.drawable.style_girl;
                id_lm = R.raw.lm_girl;
                id_lut = R.raw.lut_girl;
                break;
            case "het":
                id_img = R.drawable.style_het;
                id_lm = R.raw.lm_het;
                id_lut = R.raw.lut_het;
                break;
            case "illegalbeauty":
                id_img = R.drawable.style_illegalbeauty;
                id_lm = R.raw.lm_illegalbeauty;
                id_lut = R.raw.lut_illegalbeauty;
                break;
            case "japan":
                id_img = R.drawable.style_japan;
                id_lm = R.raw.lm_japan;
                id_lut = R.raw.lut_japan;
                break;
            case "ken":
                id_img = R.drawable.style_ken;
                id_lm = R.raw.lm_ken;
                id_lut = R.raw.lut_ken;
                break;
            case "laurinbust":
                id_img = R.drawable.style_laurinbust;
                id_lm = R.raw.lm_laurinbust;
                id_lut = R.raw.lut_laurinbust;
                break;
            case "lincolnbust":
                id_img = R.drawable.style_lincolnbust;
                id_lm = R.raw.lm_lincolnbust;
                id_lut = R.raw.lut_lincolnbust;
                break;
            case "malevich":
                id_img = R.drawable.style_malevich;
                id_lm = R.raw.lm_malevich;
                id_lut = R.raw.lut_malevich;
                break;
            case "oilman":
                id_img = R.drawable.style_oilman;
                id_lm = R.raw.lm_oilman;
                id_lut = R.raw.lut_oilman;
                break;
            case "oldman":
                id_img = R.drawable.style_oldman;
                id_lm = R.raw.lm_oldman;
                id_lut = R.raw.lut_oldman;
                break;
            case "prisma":
                id_img = R.drawable.style_prisma;
                id_lm = R.raw.lm_prisma;
                id_lut = R.raw.lut_prisma;
                break;
            case "sketch":
                id_img = R.drawable.style_sketch;
                id_lm = R.raw.lm_sketch;
                id_lut = R.raw.lut_sketch;
                break;
            case "stonebust":
                id_img = R.drawable.style_stonebust;
                id_lm = R.raw.lm_stonebust;
                id_lut = R.raw.lut_stonebust;
                break;
            case "watercolorgirl":
                id_img = R.drawable.style_watercolorgirl;
                id_lm = R.raw.lm_watercolorgirl;
                id_lut = R.raw.lut_watercolorgirl;
                break;
            case "woodenmask":
                id_img = R.drawable.style_woodenmask;
                id_lm = R.raw.lm_woodenmask;
                id_lut = R.raw.lut_woodenmask;
                break;
            default:
                Log.e("ERROR","style: " + name + " is not listed in id-switch-case");
                id_img = R.drawable.style_bronzestatue;
                id_lm = R.raw.lm_bronzestatue;
                id_lut = R.raw.lut_bronzestatue;
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
