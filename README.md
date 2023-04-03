# Sound Propagation
## Introduction
Sound Propagation is a C++ program that simulates sound propagation also as a final project for UC Santa Barbara's MAT240B course. The program includes features such as sound absorption, reflection, diffraction(yet to implement), and reverb to create a realistic acoustic environment for user.  

## Reference:
- http://gamma.cs.unc.edu/GSOUND/gsound_aes41st.pdf
- http://gamma.cs.unc.edu/SOUND09/

## Building
This project is based on [allolib](https://github.com/AlloSphere-Research-Group/allolib/) which supports MacOS/Linux/Windows(some extra [steps](https://github.com/AlloSphere-Research-Group/allolib/) for building on Windows).  
To build Sound Propagation, follow these steps:
```
git clone --recurse-submodules https://github.com/jinjinhe2001/MAT201B-physics-allolib.git
./update.sh
./configure.sh
./run.sh
```
## Result

https://user-images.githubusercontent.com/72654824/229414823-158429df-9f83-40ad-8352-50fe9bcf307f.mp4

![sound propagation](https://jinjinhe2001.github.io/images/MAT/sound.png)

