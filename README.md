# FaceBlit
### Real-Time Example-Based Face Stylization on Mobile Devices

This project deals with an implementation of Android mobile application allowing example-based style transfer from a given style exemplar to a human face captured by a built-in camera. The entire process of stylization is computed offline by a mobile device. The application uses an existing face detector (from dlib) for obtaining essential landmarks of a captured face. The project is based on a combination of two papers:

[1] J. Fišer, O. Jamriška, D. Simons, E. Shechtman, J. Lu, P. Asente, M. Lukáč, and D. Sýkora: Example-Based Synthesis of Stylized Facial Animations. ACM Transactions on Graphics 36, 4 (2017). (SIGGRAPH, Los Angeles, USA, July 2017)


[2] D. Sýkora, O. Jamriška, O. Texler, J. Fišer, M. Lukáč, J. Lu, and E. Shechtman: StyleBlit: Fast Example-Based Stylization with Local Guidance. In Computer Graphics Forum. (Eurographics, Genova, Italy, May 2019)


### Build and Run - Desktop (Windows)
* There is a Visual Studio project in directory *FaceBlit/VS*
* OpenCV version: 4.5.0
  * This repository contains necessary .lib files and includes
  * .dll files need to be downloaded (https://sourceforge.net/projects/opencvlibrary/files/4.5.0/opencv-4.5.0-vc14_vc15.exe/download - pre-built for Windows)
    * *opencv_world450d.dll* and *opencv_world450.dll* files are located in *opencv-4.5.0-vc14_vc15/build/x64/vc15/bin* and are expected in the PATH 
* Dlib version: 19.21
  * It is added to the project as a "Header-only library" - file *FaceBlit/app/src/main/cpp/source.cpp*
  
  
### Notes
* All CPP sources (except main.cpp) are located in *FaceBlit/app/src/main/cpp*
* Style exemplars (.png) are located in *FaceBlit/app/src/main/res/drawable*
* Files with detected landmarks (.txt) and lookup tables (.bytes) for each style are located in *FaceBlit/app/src/main/res/raw*

