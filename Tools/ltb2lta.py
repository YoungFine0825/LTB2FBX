import os;
import sys;
import subprocess;
import glob

pyFileDir = os.path.dirname(os.path.abspath(__file__))
pathToLzma = pyFileDir + "/lzma.exe"
pathToModekUnpacker = pyFileDir + "/Model_Unpacker.exe"

if not os.path.exists(pathToLzma):
    print("未找到lzma.exe程序，请将它与该Python文件放在同一目录下！")
if not os.path.exists(pathToModekUnpacker):
    print("Model_Unpacker.exe程序，请将它与该Python文件放在同一目录下！")

def collect_files(directory, extensions):
    files = []
    for extension in extensions:
        pattern = os.path.join(directory, f"*.{extension}")
        files.extend(glob.glob(pattern))
    return files


inputLtbFiles = []
if len(sys.argv) > 1:
    arg1 = sys.argv[1]
    if os.path.isdir(arg1):
        inputLtbFiles = collect_files(arg1,["ltb"])
    else:
        inputLtbFiles.append(arg1)
else:
    inputLtbFiles = collect_files(pyFileDir,["ltb"])


if len(inputLtbFiles) <= 0:
    print("未知文件或目录")
    exit()

#将ltb文件解压缩
def decodeLtbFile(pathToLtb):
    fileName,fileExt = os.path.splitext(pathToLtb)
    pathToOutputFile = fileName + "_decoded.ltb"
    cmdArgs = [pathToLzma," d ",pathToLtb," ",pathToOutputFile]
    cmdStr = ""
    # print(cmdStr.join(cmdArgs))
    ret = subprocess.run(cmdStr.join(cmdArgs),shell=True,capture_output=True,text=True)
    if ret.returncode == 0:
        return pathToOutputFile
    else:
        print(ret.stderr)
        return None

#将ltb文件转换为lta文件
def Ltb2Lta(inputLtbFile,outputLtaFile):
    cmdArgs = [pathToModekUnpacker," d3d -input ",inputLtbFile," -output ",outputLtaFile," -verbose"]
    cmdStr = ""
    # print(cmdStr.join(cmdArgs))
    ret = subprocess.run(cmdStr.join(cmdArgs),shell=True,capture_output=True,text=True)
    if ret.returncode != 0:
        print(ret.stderr)
    return ret.returncode

# main
for pathToLtbFile in inputLtbFiles:
    ltbFileName,fileExt = os.path.splitext(pathToLtbFile)
    outputLtaFileName = ltbFileName + ".lta"
    #尝试对文件解压缩
    pathToDecodedFile = decodeLtbFile(pathToLtbFile)
    retcode = 0
    if pathToDecodedFile is None:
        # 解压缩失败，该文件不需要解压缩或不是使用的lzma压缩算法
        retcode = Ltb2Lta(pathToLtbFile,outputLtaFileName)
    else:
        # 解压缩成功
        retcode = Ltb2Lta(pathToDecodedFile,outputLtaFileName)
    #
    if retcode == 0:
        print("转换成功："+pathToLtbFile+" --> "+outputLtaFileName)
        # 删除解压缩中间文件
        if not pathToDecodedFile is None:
            os.remove(pathToDecodedFile)
    else:
        print("转换失败："+pathToLtbFile)
        