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
//  ALEngine.cpp
//  626OpenALFFMpeg
//
//  Created by sjhz on 2017/6/26.
//  Copyright © 2017年 sjhz. All rights reserved.
//

//#include "ALEngine.hpp"
/////
//#include "stdafx.h"
#include "ALEngine.hpp"


ALEngine::ALEngine()
{
    InitEngine();
    m_bStop = true;
}


ALEngine::~ALEngine()
{
    DestroyEngine();
    
}

int ALEngine::InitEngine()
{
    ALCdevice* pDevice;
    ALCcontext* pContext;
    
    pDevice = alcOpenDevice(NULL);
    pContext = alcCreateContext(pDevice, NULL);
    alcMakeContextCurrent(pContext);
    
    if (alcGetError(pDevice) != ALC_NO_ERROR)
        return AL_FALSE;
    
    return 0;
}

int ALEngine::DestroyEngine()
{
    ALCcontext* pCurContext;
    ALCdevice* pCurDevice;
    
    pCurContext = alcGetCurrentContext();
    pCurDevice = alcGetContextsDevice(pCurContext);
    
    alcMakeContextCurrent(NULL);
    alcDestroyContext(pCurContext);
    alcCloseDevice(pCurDevice);
    return 0;
}

////

template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

///

int ALEngine::OpenFile(std::string path)
{
    if (!m_bStop)
    {
        Stop();
    }
    m_bStop = false;
    alGenSources(1, &m_source);
    if (alGetError() != AL_NO_ERROR)
    {
        printf("Error generating audio source.");
        return -1;
    }
    ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
    ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
    ALfloat ListenerPos[] = { 0.0, 0, 0 };
    ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
    // first 3 elements are "at", second 3 are "up"
    ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
    alSourcef(m_source, AL_PITCH, 1.0);
    alSourcef(m_source, AL_GAIN, 1.0);
    alSourcefv(m_source, AL_POSITION, SourcePos);
    alSourcefv(m_source, AL_VELOCITY, SourceVel);
    alSourcef(m_source, AL_REFERENCE_DISTANCE, 50.0f);
    alSourcei(m_source, AL_LOOPING, AL_FALSE);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alListener3f(AL_POSITION, 0, 0, 0);
    m_decoder = std::move(make_unique<AudioDecoder>());

//    m_decoder = std::move(std::make_unique<AudioDecoder>());
    //m_decoder.reset(new AudioDecoder()); //for ios
    m_decoder->OpenFile(path);
    m_decoder->StartDecode();
    alGenBuffers(NUMBUFFERS, m_buffers);
    //m_ptPlaying = std::move(std::make_unique<std::thread>(&ALEngine::SoundPlayingThread, this));
    m_ptPlaying.reset(new std::thread(&ALEngine::SoundPlayingThread, this));    //for ios
    return 0;
}

int ALEngine::Play()
{
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state == AL_STOPPED || state == AL_INITIAL)
    {
        alSourcePlay(m_source);
    }
    
    return 0;
}

int ALEngine::PausePlay()
{
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state == AL_PAUSED)
    {
        alSourcePlay(m_source);
    }
    
    return 0;
}

int ALEngine::Pause()
{
    alSourcePause(m_source);
    return 0;
}

int ALEngine::Stop()
{
    if(m_bStop)
        return 0;
    m_bStop = true;
    alSourceStop(m_source);
    alSourcei(m_source, AL_BUFFER, 0);
    m_decoder->StopDecode();//要先把decoder stop,否则可能hang住 播放线程
    if(m_ptPlaying->joinable())
        m_ptPlaying->join();
    //
    alDeleteBuffers(NUMBUFFERS, m_buffers);
    alDeleteSources(1, &m_source);
    
    
    return 0;
}


int ALEngine::SoundPlayingThread()
{
    //get frame
    for (int i = 0; i < NUMBUFFERS; i++)
    {
        SoundCallback(m_buffers[i]);
    }
    Play();
    
    //
    while (true)
    {
        if (m_bStop)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(SERVICE_UPDATE_PERIOD));
        ALint processed = 0;
        alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
        //printf("the processed is:%d\n", processed);
        while (processed > 0)
        {
            ALuint bufferID = 0;
            alSourceUnqueueBuffers(m_source, 1, &bufferID);
            SoundCallback(bufferID);
            processed--;
        }
        Play();
    }
    
    return 0;
}

int ALEngine::SoundCallback(ALuint& bufferID)
{
    PTFRAME frame = m_decoder->GetFrame();
    if (frame == nullptr)
        return -1;
    ALenum fmt;
    if (frame->chs == 1)
    {
        fmt = AL_FORMAT_MONO16;
    }
    else
    {
        fmt = AL_FORMAT_STEREO16;
    }
    alBufferData(bufferID, fmt, frame->data, frame->size, frame->samplerate);
    alSourceQueueBuffers(m_source, 1, &bufferID);
    if (frame)
    {
        av_free(frame->data);
        delete frame;
    }
    return 0;
}
