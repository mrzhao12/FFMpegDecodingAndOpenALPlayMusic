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
//  AudioDecoder.hpp
//  626OpenALFFMpeg
//
//  Created by sjhz on 2017/6/26.
//  Copyright © 2017年 sjhz. All rights reserved.
//
//
//#ifndef AudioDecoder_hpp
//#define AudioDecoder_hpp

#include <stdio.h>

//#endif /* AudioDecoder_hpp */




/////

#ifndef __AUDIODECODER_H__
#define __AUDIODECODER_H__
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

typedef struct _tFrame
{
    void* data;
    int size;
    int chs;
    int samplerate;
    uint64_t pts;
    uint64_t duration;
}TFRAME, *PTFRAME;

class AudioDecoder
{
public:
    AudioDecoder(int outSamplerate = 44100, int outChannels = 2);
    ~AudioDecoder();
    int OpenFile(std::string path);
    int GetChs() { return m_ch; };
    int GetSampleRate() { return m_sampleRate; };
    uint64_t GetWholeDuration() { return m_wholeDuration; };//单位微秒
    int StartDecode();
    int StopDecode();
    PTFRAME GetFrame();
    void SetSeekPosition(double playProcess);
private:
    int DecodeThread();
    int InternalAudioSeek(uint64_t start_time);
    
    
private:
    int m_ch;
    int m_sampleRate;
    uint64_t m_wholeDuration;
    std::shared_ptr<AVFormatContext> m_pFormatCtx;
    AVCodecContext* m_pAudioCodecCtx;
    AVCodec* m_pAudioCodec;
    int m_nAudioIndex;
    std::string m_strPath;
    std::queue<PTFRAME> m_queueData;
    std::atomic_bool m_bDecoding;
    static const int MAX_BUFF_SIZE = 128;
    std::shared_ptr<std::mutex> m_pmtx;
    std::shared_ptr<std::condition_variable> m_pcond;
    std::shared_ptr<std::thread> m_pDecode;
    
    std::atomic_bool m_bStop;
    std::atomic_bool m_bSeeked;
    
    //
    int m_outSampleRate;
    int m_outChs;
};
#endif
