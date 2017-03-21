# mxnet-ssd.cpp
C++ object detection module for [mxnet-ssd](https://github.com/zhreshold/mxnet-ssd).

Note that python detection module is already included in mxnet-ssd.
The goal of this repository is to create a test end for mxnet-ssd with no external
dependency.

![demo](https://cloud.githubusercontent.com/assets/3307514/19162698/900ec9ec-8bbe-11e6-9f24-505906c96371.jpg)
![demo1](https://cloud.githubusercontent.com/assets/3307514/19162697/8f14062e-8bbe-11e6-8f5f-1a4cefb6b428.jpg)

### Installation

#### Ubuntu/Debian
* Install prerequisites, via `sudo apt-get install cmake git build-essentials`
and make sure your c++ compiler support C++11 (g++4.8 or above): `g++ --version`
* Clone this repo recursively:
`git clone --recursive https://github.com/zhreshold/mxnet-ssd.cpp`
* Build all using script:
```
cd $REPO_ROOT/build
bash ./install.sh
```

#### Windows
* Install prerequisite: 
    - `cmake` and `git`(git is optional, highly recommended), they both provide installer online. 
    - Install Visual Studio 2015(verified version).
* Clone this repo:
```
git clone https://github.com/zhreshold/mxnet-ssd.cpp
git checkout win
# or you can download from github as zip file if git was not installed
# be sure to switch to branch win before download
```
* Generate VS solution:
```
cd /path/to/repo/build
./create_sln_vs2015.bat
# the solution will be created in build/vc14/
# open this solution and build, the executable and dlls will be copied to /repo/build if success
```

#### OSX
Installation guide to be added.

### Download the model
Download the pretrained [model](https://github.com/zhreshold/mxnet-ssd.cpp/releases/download/v0.1/deploy_ssd_300_voc0712.zip) and unzip to the same folder as the executable.
By default it's the `build` directory.

You can put the model any where you like, as long as you run the program with `--model /path/to/model_prefix`

### Usage
```
cd $REPO_ROOT/build
./ssd ../demo/000001.jpg
# save detection image
./ssd ../demo/000004.jpg -o out.jpg
# save detection results to text file
./ssd ../demo/000002.jpg --save-result result.txt
```
Full usage info: `./ssd -h`

```
Usage: ssd  [-hv] [-o <FILE>] [-m <FILE>] [-e <INT>] [--class-map <FILE>] [--width <INT>] [--height <INT>] [-r <FLOAT>] [-g <FLOAT>] [-b <FLOAT>] [-t <FLOAT>] [--gpu <INT>] [--disp-size <INT>] [--save-result <FILE>] <FILE>

  Required options:
  <FILE>                    input image

  Optional options:
  -h, --help                print this help and exit
  -v, --version             print version and exit
  -o, --out=FILE            output detection result to image
  -m, --model=FILE          load model prefix(default: deploy_ssd_300)
  -e, --epoch=INT           load model epoch(default: 1)
  --class-map=FILE          load classes from text file
  --width=INT               resize width(default: 300)
  --height=INT              resize height(default: 300)
  -r, --red=FLOAT           red mean pixel value(default: 123)
  -g, --green=FLOAT         green mean pixel value(default: 117)
  -b, --blue=FLOAT          blue mean pixel value(default: 104)
  -t, --thresh=FLOAT        visualize threshold(default: 0.5)
  --gpu=INT                 gpu id to detect with, default use cpu(default: -1)
  --disp-size=INT           display size, -1 to disable display(default: 640)
  --save-result=FILE        save result in text file


```

### Credits
* [CImg](https://github.com/dtschump/CImg)
* [MXNet](https://github.com/dmlc/mxnet)
* [zupply](https://github.com/zhreshold/zupply)
* [OpenBLAS](https://github.com/xianyi/OpenBLAS)
