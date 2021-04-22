# FaceBlit: Instant Real-Time Example-Based Style Transfer to Facial Videos

The official implementation of

> **FaceBlit: Instant Real-Time Example-Based Style Transfer to Facial Videos** <br>
_A. Texler, [O. Texler](https://ondrejtexler.github.io/), [M. Kučera](https://www.linkedin.com/in/kuceram/), [M. Chai](http://www.mlchai.com), and [D. Sýkora](https://dcgi.fel.cvut.cz/home/sykorad/)_ <br>
[:globe_with_meridians: Project Page](https://ondrejtexler.github.io/faceblit/index.html), 
[:page_facing_up: Paper](https://dcgi.fel.cvut.cz/home/sykorad/Texler21-I3D.pdf), 
[:books: BibTeX](https://dcgi.fel.cvut.cz/home/sykorad/Texler21-I3D.bib)



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

