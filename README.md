## Wave (Audio Spectrum)
这是关于bass.dll音频库频谱的简单示例.<br>
主要是将其win32库(bass24.zip)中的C示范代码spectrum部分转换为C++Builder版本.<br>

之前使用画线和画点来绘制频谱, 发现比较占用CPU使用率.<br>
后来参考示范代码后, 生成bitmap再绘制的话, 基本就不占用CPU了. 致敬他们的技术.<br>

改写时就会知道C++Builder坑多(当然是自身能力所限导致的, 现在C++Builder已经都忘记得差不多了).<br>
不可否认, C++Builder在快速创建Windows图形界面是非常有优势的, 编译后的执行文件也很小巧.<br>
至于选用什么编程语言, 在编写应用程序之前的思量再三都是不为过的. 不过都会各有各坑的情况.<br>

这里分别使用C++Builder的API和Windows GDI, 效果是差不多. 程序在RAD Studio 11.3编译通过.<br>
<img src="https://github.com/cygstar/Wave/blob/main/assets/Wave-RAD_01.png" height="160">
<img src="https://github.com/cygstar/Wave/blob/main/assets/Wave-RAD_02.png" height="160"><br>
<img src="https://github.com/cygstar/Wave/blob/main/assets/Wave-WIN_01.png" height="160">
<img src="https://github.com/cygstar/Wave/blob/main/assets/Wave-WIN_02.png" height="160"><br>

其中,<br>
在声频频谱(上部): 左键 -> 跳到点击位置并播放; Ctrl+左键 -> 设置循环播放起点; Ctrl+右键 -> 设置循环播放终点; Remove -> 清除循环播放.<br>
在动态频谱(下部): 左键 -> 切换频谱模式<br>

bass.dll必须放在与应用程序相同的目录中(没有此库, 程序将无法使用), 或者放在系统目录的Windows下也是可以的(全局使用).<br>
如果需要播放更多声频类型格式, 必须把音频插件放在当前应用程序在Plugins下.<br>
以上简单示例的代码甚为丑陋且不健壮(比如播放前并没有检查是否正确打开文件, 设定/清除循环起终点时没有判断两点的前后位置等等等, 但程序还能运行...)<br>
此简单示例不足以作为参考, 仅仅提供一点思路的提示吧.<br>

This is a simple example code of the bass.dll audio library's spectrum functionality. It involves converting the spectrum section of its C demo code from the win32 library (bass24.zip) into a version compatible with C++Builder.<br>
Keep on coding, keep on being happy.<br>

关于bass.dll的详细, 请访问以下网站.<br>
For any detailed information, please visit:<br>
https://www.un4seen.com/<br>
