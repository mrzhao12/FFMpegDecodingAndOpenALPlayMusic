/**
*使用OpenAL和FFMPEG解码并播放音乐
*626OpenALFFMpeg
 *赵彤彤 mrzhao12  ttdiOS
*1107214478@qq.com
*http://www.jianshu.com/u/fd9db3b2363b
*本程序是macOS平台下OpenAL和FFMPEG解码并播放音乐
*OpenAL是一个开源的音效库，然而这里只用它来播放音乐。
*FFMPEG负责把音乐解码并转换成指定的编码和采样率，然后送到OpenAL中播放。
*已在windows和iOS平台简单测试通过）
*/
//  main.cpp
//  626OpenALFFMpeg
//
//  Created by sjhz on 2017/6/26.
//  Copyright © 2017年 sjhz. All rights reserved.
//#include <iostream>/Users/zhaotong/Desktop/output的副本.mp3//
//int main(int argc, const char * argv[]) {
//    // insert code here...
//    std::cout << "Hello, World!\n";
//    return 0;
//}
////
#warning 请把openFile处的文件路径修改为你的桌面文件路径(mp3或者aac音频文件)

#include "ALEngine.hpp"
#include <iostream>

template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

int main()
{
//    auto pe = std::make_unique<ALEngine>();/Users/zhaotong/Desktop/11111111.aac
    auto pe = make_unique<ALEngine>();

//    pe->OpenFile("D:\\music\\style.mp3");
//    pe->OpenFile("/Users/zhaotong/Desktop/11111111.aac");
      pe->OpenFile("/Users/zhaotong/Desktop/output的副本.mp3");
//           pe->OpenFile("/Users/zhaotong/Desktop/北京欢迎你.mp3");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //pe->m_decoder->SetSeekPosition(0.5);
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    }
    return 0;
}
