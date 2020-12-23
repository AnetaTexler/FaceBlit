package texler.faceblit;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

import texler.faceblit.fragments.StyleSelectorFragment;


public class RecyclerViewAdapter extends RecyclerView.Adapter<RecyclerViewAdapter.ViewHolder> {

    private static final String TAG = "RecyclerViewAdapter";
    private ArrayList<String> mImageNames;
    private Context mContext;
    private IRecyclerViewCallback mCallback;
    private int mSelectedPosition;

    public RecyclerViewAdapter(Context context, ArrayList<String> imageNames) {
        this.mContext = context;
        this.mImageNames = imageNames;
        //this.mCallback = (IRecyclerViewCallback)context;
        this.mSelectedPosition = -1;
    }

    @NonNull
    @Override
    public RecyclerViewAdapter.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.recycler_view_item, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerViewAdapter.ViewHolder holder, int position) {
        final String imageName = mImageNames.get(position);
        int imageId = mContext.getResources().getIdentifier("@drawable/" + imageName, "drawable", mContext.getPackageName());
        holder.mImageView.setImageResource(imageId);
        holder.mImageView.setForeground(null);

        if (mSelectedPosition == position) {
            holder.mImageView.setForeground(ContextCompat.getDrawable(mContext, R.drawable.highlight_frame));
        }

        holder.mImageView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                int prevSelected = mSelectedPosition;
                mSelectedPosition = position;
                notifyItemChanged(prevSelected);
                notifyItemChanged(position);

                //mCallback.onStyleImageClick(imageName.split("_")[2]); // name without "recycler_view_" prefix
                StyleSelectorFragment.getInstance().setStyleData(imageName.split("_")[2]);
            }
        });
    }

    @Override
    public int getItemCount() {
        return mImageNames.size();
    }


    public class ViewHolder extends RecyclerView.ViewHolder {

        ImageView mImageView;

        private ViewHolder(@NonNull View itemView) {
            super(itemView);
            mImageView = itemView.findViewById(R.id.style_image_view);
        }
    }
}
