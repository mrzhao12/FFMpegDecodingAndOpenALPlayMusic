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
//  AudioDecoder.cpp
//  626OpenALFFMpeg
//
//  Created by sjhz on 2017/6/26.
//  Copyright © 2017年 sjhz. All rights reserved.
//

#include "AudioDecoder.hpp"



////
//#include "stdafx.h"
#include "AudioDecoder.hpp"

#define CPP_TIME_BASE_Q (AVRational{1, AV_TIME_BASE})

AudioDecoder::AudioDecoder(int outSamplerate, int outChannels)
{
    m_pmtx = nullptr;
    m_pAudioCodec = nullptr;
    m_pFormatCtx = nullptr;
    m_pAudioCodecCtx = nullptr;
    static bool bFFMPEGInit = false;
    if (!bFFMPEGInit)
    {
        av_register_all();
        avcodec_register_all();
        bFFMPEGInit = true;
    }
    m_outSampleRate = outSamplerate;
    m_outChs = outChannels;
    m_wholeDuration = -1;
    m_ch = -1;
    m_strPath = "";
    m_bStop = true;
    m_bSeeked = false;
}


AudioDecoder::~AudioDecoder()
{
    
}



int AudioDecoder::OpenFile(std::string path)
{
    if (!m_bStop)
    {
        StopDecode();
    }
    m_strPath = path;
    AVFormatContext* pFormatCtx = nullptr;
    if (0 != avformat_open_input( &pFormatCtx, path.c_str(), NULL, NULL))
    {
        return -1;
    }
    m_pFormatCtx = std::shared_ptr<AVFormatContext>(pFormatCtx, [](AVFormatContext* pf) { if (pf) avformat_close_input(&pf); });
    
    if (avformat_find_stream_info(m_pFormatCtx.get(), NULL) < 0)
    {
        return -1;
    }
    
    for (int i = 0; i < m_pFormatCtx->nb_streams; i++)
    {
//        if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
         if (m_pFormatCtx->streams[i]/*视音频流*/->codec->codec_type == AVMEDIA_TYPE_VIDEO)//查找音频
        {
            m_nAudioIndex = i;
            break;
        }
    }
    
    if (m_nAudioIndex == -1)
    {
        return -1;
    }
    m_pAudioCodecCtx = m_pFormatCtx->streams[m_nAudioIndex]->codec;
    m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecCtx->codec_id); //
    if (m_pAudioCodec < 0)
    {
        return -1;
    }
    if (avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, NULL) < 0)
    {
        return -1;
    }
    m_ch = m_pAudioCodecCtx->channels;
    m_wholeDuration = av_rescale_q(m_pFormatCtx->streams[m_nAudioIndex]->duration,
                                   m_pFormatCtx->streams[m_nAudioIndex]->time_base,
                                   CPP_TIME_BASE_Q);
    
    return 0;
}

int AudioDecoder::StartDecode()
{
    m_bStop = false;
    m_pmtx = std::move(std::make_shared<std::mutex>());
    m_pcond = std::move(std::make_shared<std::condition_variable>());
    m_pDecode = std::move(std::make_shared<std::thread>(&AudioDecoder::DecodeThread, this));
    return 0;
}

int AudioDecoder::StopDecode()
{
    m_bStop = true;
    m_pcond->notify_all();
    if(m_pDecode->joinable())
        m_pDecode->join();
    std::unique_lock<std::mutex> lck(*m_pmtx);
    int queueSize = m_queueData.size();
    for (int i = queueSize - 1; i >= 0; i--)
    {
        PTFRAME f = m_queueData.front();
        m_queueData.pop();
        if (f)
        {
            if (f->data)
                av_free(f->data);
            delete f;
        }
    }
    return 0;
}

PTFRAME AudioDecoder::GetFrame()
{
    PTFRAME frame = nullptr;
    std::unique_lock<std::mutex> lck(*m_pmtx);
    m_pcond->wait(lck,
                  [this]() {return m_bStop || m_queueData.size() > 0; });
    if (!m_bStop)
    {
        frame = m_queueData.front();
        m_queueData.pop();
    }
    lck.unlock();
    m_pcond->notify_one();
    return frame;
}

int AudioDecoder::DecodeThread()
{
    if (m_strPath == "")
        return -1;
    m_bDecoding = true;
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    std::shared_ptr<AVFrame> pAudioFrame(av_frame_alloc(), [](AVFrame* pFrame) {av_frame_free(&pFrame);});
    int64_t in_channel_layout = av_get_default_channel_layout(m_pAudioCodecCtx->channels);
    struct SwrContext* au_convert_ctx = swr_alloc();
    int64_t outLayout;
    if (m_outChs == 1)
    {
        outLayout = AV_CH_LAYOUT_MONO;
    }
    else
    {
        outLayout = AV_CH_LAYOUT_STEREO;
    }
    
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
                                        outLayout, AV_SAMPLE_FMT_S16,
                                        m_outSampleRate, in_channel_layout,
                                        m_pAudioCodecCtx->sample_fmt, m_pAudioCodecCtx->sample_rate, 0,
                                        NULL);
    swr_init(au_convert_ctx);
    
    while (true)
    {
        int ret = av_read_frame(m_pFormatCtx.get(), packet);
        if (ret < 0)
            break;
        if (packet->stream_index == m_nAudioIndex)
        {
            int nAudioFinished = 0;
            int nRet = avcodec_decode_audio4(m_pAudioCodecCtx, pAudioFrame.get(),
                                             &nAudioFinished, packet);
            if (nRet > 0 && nAudioFinished != 0)
            {
                PTFRAME frame = new TFRAME;
                frame->chs = m_outChs;
                frame->samplerate = m_outSampleRate;
                frame->duration = av_rescale_q(packet->duration, m_pFormatCtx->streams[m_nAudioIndex]->time_base, CPP_TIME_BASE_Q);
                frame->pts = av_rescale_q(packet->pts, m_pFormatCtx->streams[m_nAudioIndex]->time_base, CPP_TIME_BASE_Q);
                
                //resample
                int outSizeCandidate = m_outSampleRate * 8 *
                double(frame->duration) / 1000000.0;
                uint8_t* convertData = (uint8_t*)av_malloc(sizeof(uint8_t) * outSizeCandidate);
                int out_samples = swr_convert(au_convert_ctx,
                                              &convertData, outSizeCandidate,
                                              (const uint8_t**)&pAudioFrame->data[0], pAudioFrame->nb_samples);
                int Audiobuffer_size = av_samples_get_buffer_size(NULL,
                                                                  m_outChs, out_samples,AV_SAMPLE_FMT_S16,1);
                frame->data = convertData;
                frame->size = Audiobuffer_size;
                std::unique_lock<std::mutex> lck(*m_pmtx);
                m_pcond->wait(lck,
                              [this]() {return m_bStop || m_queueData.size() < MAX_BUFF_SIZE; });
                if (m_bStop)
                {
                    av_free_packet(packet);
                    break;
                }
                if (m_bSeeked)
                {
                    m_bSeeked = false;
                    av_free_packet(packet);
                    continue;
                }
                m_queueData.push(frame);
                lck.unlock();
                m_pcond->notify_one();
            }
        }
        av_free_packet(packet);
    }
    swr_free(&au_convert_ctx);
    return 0;
}


//for seek
int AudioDecoder::InternalAudioSeek(uint64_t start_time_in_us)
{
    if (start_time_in_us < 0)
    {
        return -1;
    }
    int64_t seek_pos = start_time_in_us;
    if (m_pFormatCtx->start_time != AV_NOPTS_VALUE)
        seek_pos += m_pFormatCtx->start_time;
    if (av_seek_frame(m_pFormatCtx.get(), -1, seek_pos, AVSEEK_FLAG_BACKWARD) < 0)
    {
        return -2;
    }
    avcodec_flush_buffers(m_pFormatCtx->streams[m_nAudioIndex]->codec);
    return 0;
}

//设置seek
void AudioDecoder::SetSeekPosition(double playProcess)
{
    uint64_t pos = (uint64_t)(playProcess * (double)m_wholeDuration);
    std::unique_lock<std::mutex> lck(*m_pmtx);
    m_bSeeked = true;
    InternalAudioSeek(pos);
    int queueSize = m_queueData.size();
    for (int i = queueSize - 1; i >= 0; i--)
    {
        PTFRAME f = m_queueData.front();
        m_queueData.pop();
        if (f)
        {
            if (f->data)
                av_free(f->data);
            delete f;
        }
    }
    m_pcond->notify_all();//如果是从wait里面拿的锁，就要手动去激活他们
    return;
}
