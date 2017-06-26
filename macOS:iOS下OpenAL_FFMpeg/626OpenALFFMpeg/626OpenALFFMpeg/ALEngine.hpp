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
//  ALEngine.hpp
//  626OpenALFFMpeg
//
//  Created by sjhz on 2017/6/26.
//  Copyright © 2017年 sjhz. All rights reserved.
//

//#ifndef ALEngine_hpp
//#define ALEngine_hpp

#include <stdio.h>

//#endif /* ALEngine_hpp */

///
#ifndef __ALENGINE_H__
#define __ALENGINE_H__
#define NUMBUFFERS              (4)
#define    SERVICE_UPDATE_PERIOD    (20)
// #include <libavcodec/avcodec.h>
//#include <al.h>
//#include <alc.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include  <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "AudioDecoder.hpp"
class ALEngine
{
public:
    ALEngine();
    ~ALEngine();
    
    int OpenFile(std::string path);
    int Play(); //只负责从初始状态和停止状态中播放
    int PausePlay();    //只负责从暂停状态播放
    int Pause();
    int Stop();
private:
    ALuint            m_source;
    std::unique_ptr<AudioDecoder> m_decoder;
    std::unique_ptr<std::thread> m_ptPlaying;
    
    std::atomic_bool m_bStop;
    
    ALuint m_buffers[NUMBUFFERS];
    ALuint m_bufferTemp;
private:
    int SoundPlayingThread();
    int SoundCallback(ALuint& bufferID);
    int InitEngine();
    int DestroyEngine();
};

#endif
