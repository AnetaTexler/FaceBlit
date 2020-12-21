
import os

if __name__ == "__main__":

    root_dir = "FFHQ"

    style_dir_names = [f for f in os.listdir(root_dir) if f.lower().startswith("output_") and os.path.isdir(os.path.join(root_dir, f))]
    input_dir_names = [f for f in os.listdir(os.path.join(root_dir, style_dir_names[0]))]

    for style_dir_name in style_dir_names:
        f = open(os.path.join(root_dir, style_dir_name + ".html"), "w+")
        style_name = style_dir_name.replace("output_", "")

        for input_name in input_dir_names:
            input_img = "<img height=512 src=\"" + "input/" + input_name + ".png" + "\">"
            no_voting_img = "<img height=512 src=\"" + style_dir_name + "/" + input_name + "/" + input_name + "_" + style_name + "_0voting_face_ours.png" + "\">"
            voting_3_img = "<img height=512 src=\"" + style_dir_name + "/" + input_name + "/" + input_name + "_" + style_name + "_3voting_face_ours.png" + "\">"
            voting_5_img = "<img height=512 src=\"" + style_dir_name + "/" + input_name + "/" + input_name + "_" + style_name + "_5voting_face_ours.png" + "\">"
            style_img = "<img height=512 src=\"" + "styles/" + style_name + ".png" + "\">"

            line = input_img + no_voting_img + voting_3_img + voting_5_img + style_img + " <br> \n"
            f.write(line)
        f.close()
