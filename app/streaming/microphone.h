#pragma once
#include <QWindow>
#include <QSemaphore>
#include <QDebug>

#include <thread>

#include <soundio/soundio.h>
// #include "concurrentqueue.h"
#include "ringbuffer.h"
#include "opus.h"

namespace audio {
    // static const float PCM_16_PARAM=32767;
    struct AudioData{
        float* soundData;
        int count;
        int sampleRate;
        int64_t seq;
        SoundIoFormat format;
        bool encoded=false;
        char* data;
        long build_timestamp;
        long filled_timestamp;
        long encoded_timestamp;
    };
    class AudioDevice;
    class Microphone : public QObject
    {
        Q_OBJECT

    private:
        // 音频参数
        int SAMPLE_RATE = 48000; // 采样率
        int CHANNELS = 1; // 声道数（单声道）

        // 网络参数
//        int DEFAULT_MIC_PORT = 47996; // 默认麦克风端口
        int MAX_QUEUE_SIZE = 50; //为解决盒子上采样问题，暂时先保存2秒的数据，原来是保存200毫秒的数据

//        int HOST_REQUEST_CHECK_INTERVAL_MS = 500; // 检查主机请求状态的间隔
        // 权限请求码
        int PERMISSION_REQUEST_MICROPHONE = 1001;
        // 延迟参数
        double MICROPHONE_LATENCY=0.04f; //麦克风延迟参数 //40毫秒采样一次
//        int PERMISSION_DELAY_MS = 100; // 权限授予后的延迟时间
        // 音频连续性参数
        int FRAME_SIZE_MS = 40; // Opus帧大小 (毫秒)  如果设置20毫秒，ubuntu到windows会有点杂音
        int SAMPLES_PER_FRAME = SAMPLE_RATE * FRAME_SIZE_MS / 1000; // 每帧采样数 (960)
        int BYTES_PER_FRAME = SAMPLES_PER_FRAME * CHANNELS * 2; // 每帧字节数 (1920)
        // 发送线程参数
        int SENDER_THREAD_SLEEP_MS = 2; // 发送线程睡眠时间 (从100减少到5)
        int SENDER_ERROR_RETRY_MS = 1; // 发送错误重试时间 (从10减少到5)
        // 音频捕获优化参数
        int CAPTURE_BUFFER_SIZE_MS = 40; // 捕获缓冲区大小 (毫秒)
        int CAPTURE_BUFFER_SIZE = SAMPLE_RATE * CAPTURE_BUFFER_SIZE_MS / 1000 * CHANNELS * 2;
        int FRAME_INTERVAL_MS = 20; // 帧间隔时间 (毫秒)
        long FRAME_INTERVAL_NS = FRAME_INTERVAL_MS * 1000000L; // 帧间隔纳秒
        // 音频质量参数
        bool ENABLE_AUDIO_SYNC = true; // 启用音频同步
        int MAX_FRAME_DELAY_MS = 50; // 最大帧延迟 (毫秒)

        struct SoundIo *soundio;
        std::vector<int> sampleRates;

        static std::shared_ptr<AudioDevice> audioDevice;

        int currentDeviceIndex;
        char* currentDeviceName;
        int targetSampleRate;
        static std::shared_ptr<AudioData> audioCache;
        static bool isReciveMicrophoneData;//是否开始接受麦克风数据
        std::thread senderThread;
//        std::thread microphoneCheckThread;
        bool isWorking; //是否开始工作，开始工作后，如果没有启动发送数据，从麦克风来的数据将一直被丢弃，直到开始发送
        bool isSendingData;//是否开始发送数据，开始发送数据后，发送线程才会开始工作
        static OpusEncoder* opusEncoder;

        long lastFrameTime; //最后的帧时间
        /**
         * @brief senderThreadRun 对麦克风的流进行编码，采用独立线程设计
         */
        void senderThreadRun();
        /**
         * @brief sendData 发送数据
         * @param data 数据
         * @param length 数据长度
         */
        void sendData(const char* data,int length);

        //static ConcurrentQueue<AudioData,30> samples;
        // static std::shared_ptr<safe::queue_t<std::vector<float>>> samples;
        static RingBuffer<std::shared_ptr<AudioData>>* samples;
        /**
         * @brief processAudioData 处理音频数据
         * @param inStream 音频数据
         * @param frame_count_min 每次最少要处理的帧数，如果我们需要密集处理，则使用最少帧数
         * @param frame_count_max 每次最对可以处理的帧数
         */
        static void processAudioData(struct SoundIoInStream* inStream, int frame_count_min, int frame_count_max);
        /**
         * @brief checkAndCreateDevice 检查并创建麦克风设备
         * @return 是否创建了新的设备
         */
       static bool checkAndCreateDevice(struct SoundIo* soundio);
    signals:

    public:
        explicit Microphone();
        virtual ~Microphone(){
            stop();
        }
        /**
         * @brief start 麦克风开始工作
         * @return
         */
        bool start(bool startImmediately=true);
        /**
         * @brief pause 麦克风暂停工作
         * @return
         */
        bool pause();
        /**
         * @brief pause 麦克风恢复工作
         * @return
         */
        bool resume();
        /**
         * @brief pause 麦克风停止工作
         * @return
         */
        bool stop();
        /**
         * @brief isRunning 获取麦克风是否正在工作
         * @return 麦克风是否正在工作
         */
        bool isRunning(){
            return isSendingData;
        }
        /**
         * @brief flushEvents 刷新事件
         */
        void flushEvents();
        /**
         * @brief updateDevice 更新设备
         */
        void updateDevice();

        QString getMicrophoneFormat(SoundIoFormat format);

        /**
         * @brief createOpusEncoder 创建opus编码器，opus内部不支持重采样，因此我们需要执行重新采样的逻辑
         * @param sample_rate 只支持 8000, 12000, 16000, 24000, or 48000，其他采样率需要进行重采样
         * @param channels
         * @return
         */
        bool createOpusEncoder(int sample_rate,int channels);
        /**
         * @brief resample 音频重采样
         * @param in 输入的数据
         * @param sampleRateIn 输入的采样率
         * @param frameCountIn 输入的样本数
         * @param out 输出的数据
         * @param sampleRateOut 输出的采样率
         * @param frameCountOut 输出的样本数
         */
        static void resample(float* in,int sampleRateIn,int frameCountIn,float* out,int sampleRateOut,int& frameCountOut);
        // /**
        //  * @brief toPcm16 float类型的数据转成pcm16位的数据
        //  * @param in
        //  * @param out
        //  */
        // static void toPcm16(float in,uint16_t& out);

        // /**
        //  * @brief fromPcm16 从pcm16位的数据转成float数据
        //  * @param in
        //  * @param out
        //  */
        // static void fromPcm16(uint16_t in,float& out);

        /**
         * @brief getNearestSampleRate 获取最接近的采样率
         * @param sampleRates 支持的采样率
         * @param targetSampleRate 目标采样率
         * @return 适配的采样率
         */
        static int getNearestSampleRate(std::vector<int> sampleRates,int targetSampleRate);
    };

    struct AudioDevice{
        int currentIndex=0;
        struct SoundIoDevice *device ;
        struct SoundIoInStream *instream;
        Microphone* microphone;
    public:
        void closeStream(){
            if(instream!=nullptr){
                soundio_instream_destroy(instream);
                instream=nullptr;
            }
            if(device!=nullptr){
                soundio_device_unref(device);
                device=nullptr;
            }
        }
    };
}
