# LTB2FBX

这是一个将LTB模型文件转换为FBX格式文件的命令行工具，同时也支持了将DTX格式图像转换TGA格式的功能。LTB和DTX都是LithTechJupiter游戏引擎的资源文件格式，LTB是二进制3d模型文件（LTA为非二进制版，即ASCII格式），DTX则是纹理资源。

项目中使用了部分LithTech源代码来加载LTB文件和转换DTX文件，主要参考了以下三个项目：

* https://github.com/crskycode/msLTBImporter ：这是一个MikeShape3D导入LTB格式文件的插件，里面包含了部分LithTech源码。本项目正是受到了这个项目的启发。
* https://github.com/jsj2008/lithtech ：项目包含较为完整的LithTechJupiter引擎源码和部分游戏源码。
* https://github.com/giaynhap/LTB2SMD ：越南小伙伴写的，将LTB转换为SMD格式的工具。我参考了其中读取LTB文件的部分。

模型导出使用的是Assimp库。由于部分游戏会对LTB文件使用Lzma压缩，所以还使用了Lzma库来解压缩。本项目主要使用FPS网络游戏《CrossFire》的资源来进行测试，目前还没有试验过其他使用LithTech引擎的游戏的资源。​ 

### Building

    1、git clone --recursive https://github.com/YoungFine0825/LTB2FBX.git

    2、cd LTB2FBX

    3、cmake -S . -B build

    4、cmake --build build --target ltb2fbx

    构建完成后，会将可执行文件拷贝到Test目录下。

### Usage

    将.ltb/.dtx文件或包含.ltb/.dtx文件的文件夹拖拽到可执行文件图标上即可。
    或使用命令行方式：
        ltb2fbx Model.LTB [Model.fbx] [option] [option] [option] ...
    options:
        -singleAnimFile        每个动画导出为单个文件
        -ignoreMeshes          不导出网格
        -ignoreAnimations      不导出动画

### 注意：

目前存在一个关于骨骼动画导出的问题。对于包含骨骼动画的LTB文件，我在提取LTB文件中的骨骼动画数据时，没有做任何修改，直接将数据传递给Assimp的ExporterScene。这种情况下，转换后得到的FBX文件在一些3D浏览器或是Unity、Unreal等之类的游戏引擎中，无法正常的播放动画。具体表现为：只有动画第一帧骨骼的位置、旋转是正确的，后续帧的骨骼位置、旋转参数均为0。也许是这些程序会在导入FBX动画时会做一些优化，譬如：删除位置、旋转没有发生变化的帧。但在Blender中是能正常播放的，经过Blender再次导出FBX后，在3D浏览器或Unity、Unreal游戏引擎中，骨骼动画就可以正常播放。具体原因尚不清楚，欢迎有思路的大佬交流一下。
