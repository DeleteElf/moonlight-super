#include "microphone.h"
#include <Limelight.h>

#if __linux__
#include <pthread.h>
#include <ogg/ogg.h>
#endif

#include <fstream>
#include "opus_defines.h"
#include "opus.h"


#define MAX_PACKET_SIZE 3*1276
namespace audio {
    using namespace std::literals;

    struct WavHeader {
        char chunkId[4];       // RIFF标识符
        uint32_t chunkSize;    // 整个文件的大小（不包括chunkId和chunkSize自身）
        char format[4];        // 格式标识符，通常是"WAVE"
        char subchunk1Id[4];   // 子块标识符，通常是"fmt "
        uint32_t subchunk1Size; // 子块大小，对于PCM格式通常是16或18字节
        uint16_t audioFormat;  // 音频格式，对于PCM是1
        uint16_t numChannels;  // 通道数，1是单声道，2是立体声
        uint32_t sampleRate;   // 采样率，如44100Hz
        uint32_t byteRate;     // 每秒字节数，即sampleRate * numChannels * bitsPerSample/8
        uint16_t blockAlign;   // 数据块对齐单位，即numChannels * bitsPerSample/8
        uint16_t bitsPerSample;// 每个样本的位数，如16位或32位
        char subchunk2Id[4];   // 子块标识符，通常是"data"
        uint32_t subchunk2Size; // 数据块大小，即样本总数 * numChannels * bitsPerSample/8
    };

    unsigned char opus_head[19] = {
        'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',  // Magic signature
        1,                                      // Version (always 1)
        1,                               // 声道数 (1/2)
        0, 0,                                   // 预跳过样本数 (LE short, 通常为0)
        48000 & 0xFF,                     // 采样率 (LE 32-bit, Opus固定为48000)
        (48000 >> 8) & 0xFF,
        (48000 >> 16) & 0xFF,
        (48000 >> 24) & 0xFF,
        0, 0,                                   // 输出增益 (LE short, 通常为0)
        0                                       // 声道映射 (0=默认)
    };


    std::ofstream* out_file=NULL;
    FILE *pcm_data_file=NULL;
    int writeSamples=-1;
    int wroteCount=0;
#if __linux__
    ogg_stream_state ogg_stream;
    void openFile(std::string file,int maxSamples=302){
        ogg_stream_init(&ogg_stream, rand()); // 随机序列号
        out_file=new std::ofstream(file, std::ios::binary);
        if(!out_file->is_open())
        {
            printf("无法创建%s音频文件.\n",file.c_str());
            exit(1);
        }
        writeSamples=maxSamples;
        wroteCount=0;


        const char* vendor = "myTestOggEncoder";
        std::vector<unsigned char> opus_tags;
        opus_tags.insert(opus_tags.end(), "OpusTags", "OpusTags" + 8);  // Magic
        opus_tags.push_back(strlen(vendor) & 0xFF);                     // Vendor长度
        opus_tags.insert(opus_tags.end(), vendor, vendor + strlen(vendor));
        opus_tags.push_back(0);  // 用户评论数 (0)

        ogg_packet header_packet, tags_packet;
        header_packet.b_o_s = 1;  // 标记为流开始
        header_packet.packet = opus_head;
        header_packet.bytes = sizeof(opus_head);
        ogg_stream_packetin(&ogg_stream, &header_packet);

        tags_packet.b_o_s = 0;    // 非流开始包
        tags_packet.packet = opus_tags.data();
        tags_packet.bytes = opus_tags.size();
        ogg_stream_packetin(&ogg_stream, &tags_packet);

    }

    void writeToFile(unsigned char* data,int length,int seq,int frameSize=960,int channels=1){
        if(seq<writeSamples){
            // 封装为 Ogg 包（简化版，实际需处理时间戳等）
            ogg_packet packet;
            packet.packet = data;
            packet.bytes = length;
            packet.b_o_s = 0; // 第一帧标记
            packet.e_o_s = (seq >= writeSamples-1);//
            packet.granulepos = seq * frameSize; // 时间戳（样本数）
            packet.packetno = seq;

            if(seq >= writeSamples-1){
                qDebug() << "last packet:eos:"<< packet.e_o_s;
            }
            ogg_stream_packetin(&ogg_stream, &packet);
            {
                // 写入 Ogg 页
                ogg_page page;
                while (ogg_stream_pageout(&ogg_stream, &page) == 1) {
                    out_file->write((char*)page.header, page.header_len);
                    out_file->write((char*)page.body, page.body_len);
                    wroteCount++;
                    qDebug() << "wrote packet target:"<<writeSamples << " done:"<< wroteCount;
                }
            }
            if(seq >= writeSamples-1){
                qDebug() << "flush last packet target:"<<writeSamples << " done:"<< wroteCount;
                // 写入 Ogg 页
                ogg_page page;
                while (ogg_stream_flush_fill(&ogg_stream, &page,0x00) == 1) {
                    out_file->write((char*)page.header, page.header_len);
                    out_file->write((char*)page.body, page.body_len);
                    wroteCount++;
                    qDebug() << "wrote last packet target:"<<writeSamples << " done:"<< wroteCount;
                }
                qDebug() << "wrote last packet!!!";
            }
        }
    }
#endif
    void openFile(std::string file,int numSamples,int numChannels=1,int sampleRate=48000){
        /*创建一个保存PCM数据的文件*/
        if((pcm_data_file = fopen(file.c_str(), "wb")) == NULL)
        {
            printf("无法创建%s音频文件.\n",file.c_str());
            exit(1);
        }
        writeSamples=numSamples;
        WavHeader header = {};
        std::memcpy(header.chunkId, "RIFF", 4);
        std::memcpy(header.format, "WAVE", 4);
        std::memcpy(header.subchunk1Id, "fmt ", 4);
        header.subchunk1Size = 16; // PCM格式固定为16字节
        header.audioFormat = 1; // PCM格式固定为1
        header.numChannels = numChannels; // 单声道或立体声
        header.sampleRate = sampleRate; // 采样率
        header.bitsPerSample = 16; // 每个样本的位数，这里假设为16位
        header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8; // 每秒字节数
        header.blockAlign = header.numChannels * header.bitsPerSample / 8; // 数据块对齐单位
        header.subchunk2Id[0] = 'd';
        header.subchunk2Id[1] = 'a';
        header.subchunk2Id[2] = 't';
        header.subchunk2Id[3] = 'a'; // 数据块标识符为"data"
        header.subchunk2Size = numSamples * header.numChannels * header.bitsPerSample / 8; // 数据块大小，即样本总数 * numChannels * bitsPerSample/8
        header.chunkSize = 36 + header.subchunk2Size; // 文件总大小（不包括RIFF和WAVE标识符）

         const char* data=reinterpret_cast<const char*>(&header);
         int size=sizeof(header);
         fwrite(data,1,size,pcm_data_file);
    }

    void writeToFile(float data,int seq){
        if(seq<writeSamples){
            int16_t sample = static_cast<int16_t>(32767*data);
            if(sample<-32768)
                sample=-32768;
            if(sample>32767)
                sample=32767;
            fwrite(&sample,1,sizeof(short),pcm_data_file);
        }
    }

    void closeFile(){
        if(pcm_data_file!=NULL){
            /*关闭文件流*/
            fclose(pcm_data_file);
            pcm_data_file=NULL;
        }
#if __linux__
        if(out_file!=NULL&&out_file->is_open()){
            ogg_stream_clear(&ogg_stream);
            out_file->close();
            qDebug() << "close out file!!!";
        }
#endif
    }


    RingBuffer<std::shared_ptr<AudioData>>* Microphone::samples=nullptr;
    OpusEncoder* Microphone::opusEncoder=nullptr;
    std::shared_ptr<AudioData> Microphone::audioCache=nullptr;
    bool Microphone::isReciveMicrophoneData=false;
    std::shared_ptr<AudioDevice> Microphone::audioDevice=std::make_shared<AudioDevice>();

    Microphone::Microphone():currentDeviceIndex(-1),isWorking(false)
        ,isSendingData(false),lastFrameTime(0){
        sampleRates=std::vector<int>{8000, 12000, 16000, 24000, 48000};
    }

    // 线性插值函数
    float linearInterpolate(float x0, float y0, float x1, float y1, float x) {
        return y0 + (y1 - y0) * ((x - x0) / (x1 - x0));
    }

    long getCurrentTime(){
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return duration.count();
    }

    static int64_t seqNumber=0;
    void Microphone::processAudioData(struct SoundIoInStream* inStream, int frame_count_min, int frame_count_max){
//        qDebug() << "正在处理麦克风数据============>当前收到的帧数："<<frame_count_max <<"允许一次处理的最小帧数:" <<frame_count_min;
        struct SoundIoChannelArea *channelArea;
        int err;
        int frames_left = frame_count_max;
        if(!isReciveMicrophoneData){
            if(audioCache!=nullptr&&audioCache->soundData!=nullptr){
                delete[] audioCache->soundData;
                audioCache->soundData=nullptr;
                audioCache=nullptr;
            }
            isReciveMicrophoneData=true;
        }

        for (;;) {
            if(!audioDevice->microphone->isWorking)
                return;
            int frame_count = frames_left;
            if ((err = soundio_instream_begin_read(inStream, &channelArea, &frame_count)))
                qCritical()<<"begin read error: "<<soundio_strerror(err);
            if (!frame_count)
                break;
            int lastCount=frame_count;
//            qDebug() << "正在处理麦克风数据,帧数："<<frame_count;
            float* in= nullptr;
            if(channelArea){//非空洞数据
                in=(float*)channelArea->ptr;
                if(inStream->layout.channel_count>1){//如果是多通道的数据，还需要转成单通道,通过测试
                    for (int i = 0; i < frame_count; ++i) {
                       in[i]=in[i*inStream->layout.channel_count];
                       // writeToFile(in[i],seqNumber-1);
                    }
                }
            }
            int capturePos=0;
            while(lastCount>0){
                //std::shared_ptr<AudioData> data=samples->tryLast();
                if(audioCache==nullptr||audioCache->soundData==nullptr ||audioCache->count==audioDevice->microphone->SAMPLES_PER_FRAME){
                    if(audioCache!=nullptr&&audioCache->count==audioDevice->microphone->SAMPLES_PER_FRAME){//采样数满了就进行编码
                        audioCache->filled_timestamp=getCurrentTime();
                        unsigned char out_data[MAX_PACKET_SIZE] = {0};
                        auto encodedBytes  = opus_encode_float(opusEncoder, audioCache->soundData,
                                                               audioCache->count, out_data, MAX_PACKET_SIZE);
                        if(audioCache->soundData!=nullptr){
                            delete[] audioCache->soundData;
                            audioCache->soundData=nullptr;
                        }
                        if(encodedBytes  < 0) {
                            qCritical()<<"opus编码失败,"<< encodedBytes ;
                            continue;
                        }
                        audioCache->encoded_timestamp=getCurrentTime();
                        audioCache->data=(char*)out_data;
                        audioCache->count=encodedBytes ;//转成byte的长度
                        audioCache->encoded=true;
                        samples->push(audioCache);//数据加入到样本池
#if __linux__
                        // if(audioCache->seq<writeSamples){
                        //     writeToFile(out_data,encodedBytes,audioCache->seq);
                        // }
#endif
//                        qDebug() << "完成音频编码，编码字节长度==>"<< encodedBytes ;
                    }
                    audioCache=std::make_shared<AudioData>();
                    audioCache->soundData=new float[audioDevice->microphone->SAMPLES_PER_FRAME];
                    audioCache->format=inStream->format;
                    audioCache->sampleRate=audioDevice->microphone->SAMPLE_RATE; //直接转成48k
                    audioCache->count=0;
                    audioCache->seq=seqNumber++;
                    audioCache->build_timestamp=getCurrentTime();
                    //samples->push(temp_data);//数据加入到样本池
                    //data=samples->last();

                }
                //直接执行重采样逻辑
                double ratio = static_cast<double>(audioCache->sampleRate) / inStream->sample_rate;
                int newFrameCount=lastCount*ratio;//48k的样本数
                int spaces= audioDevice->microphone->SAMPLES_PER_FRAME-audioCache->count;
                int writeCount=spaces;
                if (!channelArea) { //出现空洞数据，设置这一段数据为空数据即可
                    memset(audioCache->soundData,audioCache->count, writeCount * inStream->bytes_per_sample);
                    qCritical()<<"Dropped "<<frame_count<<" frames due to internal overflow";
                } else {
                    if(newFrameCount<writeCount)
                        writeCount=newFrameCount;
                    for (int i = audioCache->count; i <audioCache->count+ writeCount; i++) {
                        double inIndex = (i-audioCache->count)/ratio+capturePos;//增加偏移量
                        int index1 = static_cast<int>(std::floor(inIndex));
                        //float frac = inIndex - index1;
                        if(inIndex==index1){ //刚好在索引上
                            audioCache->soundData[i] = in[index1];
                        }else if (index1 >= 0 && index1 < frame_count - 1) {//如果不在索引上，则取前后2帧数据的线性插值
                            float last=in[index1];
                            float next=in[index1 + 1];
                            audioCache->soundData[i] = linearInterpolate(index1, last, index1 + 1,next, inIndex);
                        } else { //大采样率转小采样率
                            // 处理边界情况，可以根据需求选择填充方式，如复制边界值
                            audioCache->soundData[i] = in[0];
                        }
                        // writeToFile(audioCache->soundData[i],seqNumber-1);
                    }
                }
                audioCache->count=audioCache->count+writeCount; //修正长度
                int encodedCount= static_cast<int>(writeCount/ratio);//计算已经编码的长度
                capturePos+=encodedCount;//更新下标
                lastCount= lastCount-encodedCount;  //更新剩余数量  (newFrameCount-writeCount)/ratio; //转回采样样本数
            }
            if ((err = soundio_instream_end_read(inStream)))
                qCritical()<<"end read error: "<<soundio_strerror(err);
            frames_left -= frame_count;
            if (frames_left <= 0)
                break;
        }
//        qDebug() << "正在处理麦克风数据============>处理完成的帧数：" << frame_count_max;
    }


    bool Microphone::start(bool startImmediately){
        if(isWorking)
            return false;
        qDebug() << "麦克风开始初始化,soundio version:" << soundio_version_string();
        int err;
        soundio = soundio_create();
        if (!soundio) {
            fprintf(stderr, "out of memory\n");
            return false;
        }
        // 设置实时优先级警告回调，注册了此方法，才会提升soundio的优先级。
        soundio->emit_rtprio_warning = [](void) {
            fprintf(stderr, "警告: 无法设置实时线程优先级。可能需要root权限或调整系统限制。\n");
        };
        if ((err = soundio_connect(soundio))) {
            fprintf(stderr, "error connecting: %s", soundio_strerror(err));
            return false;
        }
        audioDevice->microphone=this;
        soundio->on_devices_change=[](struct SoundIo* sound)->void{
           if(audioDevice!= nullptr&&audioDevice->microphone!= nullptr&&audioDevice->microphone->isWorking) {
               checkAndCreateDevice(sound);
           }
        };
        samples=new RingBuffer<std::shared_ptr<AudioData>>(MAX_QUEUE_SIZE);
        targetSampleRate= getNearestSampleRate(sampleRates,SAMPLE_RATE);//按8000, 12000, 16000, 24000, or 48000 就近更换。
        qInfo() << "编码的目标采样率："<<targetSampleRate;
        if(createOpusEncoder(targetSampleRate,CHANNELS)) { //创建编码器
            seqNumber=0;
#if __linux__
            // openFile("test.opus");
#endif
            // openFile("test.wav",960*50*60,1);
            //samples = std::make_shared<sample_queue_t::element_type>(30);
            // samples.get().
            isSendingData = startImmediately; //立即接收来自麦克风的数据
            isWorking=true;
            senderThread= std::thread([&]() mutable { //使用mutable允许修改捕获的变量
               this->senderThreadRun();
            });

#if __linux__
            // 获取线程的 ID
        pthread_t pthread_id = senderThread.native_handle();
        // 设置调度策略为 SCHED_RR (实时轮转) 或 SCHED_FIFO (实时先进先出)
        // 注意：SCHED_FIFO 和 SCHED_RR 需要超级用户权限才能使用高优先级
        struct sched_param param;
        int policy;
        pthread_getschedparam(pthread_id, &policy, &param);
        param.sched_priority = sched_get_priority_max(policy); // 获取最大优先级
        pthread_setschedparam(pthread_id, policy, &param);
#endif
//            microphoneCheckThread=std::thread([&]() mutable { //使用mutable允许修改捕获的变量
//                qInfo() << "麦克风事件监控程序已启动！！！";
//                while (isWorking){
//                    std::time_t current_time=std::time(nullptr);
//                    if(current_time-lastCheckTime>1){ //每秒检查一次
//                        lastCheckTime=current_time;
//                        soundio_flush_events(soundio);//刷新事件池
//                    }
//                    std::this_thread::sleep_for(100ms);
//                }
//                qInfo() << "麦克风事件监控程序已退出！！！";
//            });
//            soundio_flush_events(soundio);//刷新事件池
            if(isSendingData){
                qInfo() << "麦克风已经就绪，准备开始工作";
            }else{
                qInfo() << "麦克风已经就绪，暂停工作";
            }
            return true;
        }else{
            return false;
        }
    }

    bool Microphone::checkAndCreateDevice(struct SoundIo* soundio){
        int default_in_device_index = soundio_default_input_device_index(soundio);
        if (default_in_device_index < 0) {
            fprintf(stderr, "no output device found");
            return false;
        }
        struct SoundIoDevice* device = soundio_get_input_device(soundio, default_in_device_index);
        if (!device) {
            fprintf(stderr, "out of memory");
            return false;
        }
        if(audioDevice->currentIndex==default_in_device_index&&audioDevice->device!=nullptr&&audioDevice->device->name==device->name){
            return false;
        }else{ //执行更换设备的逻辑
            audioDevice->closeStream();
            samples->empty();
            isReciveMicrophoneData=false;
        }
        qInfo() << "获取到新的麦克风设备["<< default_in_device_index <<"]："<< device->name ;
        audioDevice->device=device;
        int err;
        struct SoundIoInStream* stream = soundio_instream_create(audioDevice->device);
        if (!stream) {
            fprintf(stderr, "Failed to create input stream.\n");
            return false;
        }
        audioDevice->instream=stream;
        qInfo() <<"设备支持的采样率范围:"<< audioDevice->device->sample_rates->min << " - " << audioDevice->device->sample_rates->max;
        audioDevice->instream->error_callback=[](struct SoundIoInStream* instream, int err)->void{
            qCritical()<<  "获取音频流出错！！ " << soundio_strerror(err);
            // audioDevice->closeStream();
            // samples->empty();
            isReciveMicrophoneData=false;
            audioDevice->currentIndex=-1;
        };
        audioDevice->instream->format = SoundIoFormatFloat32NE; // 设置音频格式为32位浮点数，小端字节序
        audioDevice->instream->sample_rate=  soundio_device_nearest_sample_rate(audioDevice->device,audioDevice->microphone->SAMPLE_RATE); // 设置采样率为44100Hz或最接近的采样率 device->sample_rate_current;  // 设置采样率与设备相同
        audioDevice->instream->layout = audioDevice->device->current_layout; //设置声道与设备相同
        audioDevice->instream->software_latency=audioDevice->microphone->MICROPHONE_LATENCY; // 设置软件延迟，单位为秒,设置0.04表示一次采样40ms的数据就callback一次。
        audioDevice->instream->userdata=nullptr;// 设置用户数据（可选）
        audioDevice->instream->read_callback=processAudioData; // 设置读取回调函数
        //instream->overflow_callback = your_overflow_callback; // 设置溢出回调函数（可选）
        qInfo() << "音频格式："<<audioDevice->microphone->getMicrophoneFormat(audioDevice->instream->format);
        qInfo() << "通道数："<<audioDevice->instream->layout.channel_count;
        qInfo() << "采样率："<<audioDevice->instream->sample_rate;
        qInfo() << "软件延迟："<<audioDevice->instream->software_latency;
        if ((err = soundio_instream_open(audioDevice->instream))) {
            qCritical()<<  "无法打开设备: " << soundio_strerror(err);
            return false;
        }
        if (audioDevice->instream->layout_error)
            qCritical()<<  "无法设置频道: " <<  soundio_strerror(audioDevice->instream->layout_error);
        if ((err = soundio_instream_start(audioDevice->instream))) {
            qCritical()<<  "无法启动设备: " << soundio_strerror(err);
            return false;
        }
        audioDevice->currentIndex=default_in_device_index;
        return true;
    }

    void Microphone::senderThreadRun(){
        qInfo() << "发送麦克风数据线程开始";
        while (isWorking) {
            if(isSendingData&&!samples->isEmpty()){
                auto temp=samples->first();
                if(temp!=nullptr&&temp->encoded){ //
                    auto audioData=samples->pop();//从环形缓存中取出第一个
                    long currentTime=getCurrentTime();
                    while (audioDevice->microphone->ENABLE_AUDIO_SYNC&&
                           currentTime-audioDevice->microphone->lastFrameTime<audioDevice->microphone->FRAME_INTERVAL_MS*0.8){
                        std::this_thread::sleep_for(1ms);//开启声音同步后，如果太快就减速
                        currentTime=getCurrentTime();
                    }
                    if(currentTime-audioData->build_timestamp>audioDevice->microphone->MAX_FRAME_DELAY_MS){
                        qDebug() << "数据包存在延迟，创建时间:" <<currentTime-audioData->build_timestamp << "毫秒";
                    }
//                    qDebug() << "正在向远程主机发送数据，数据长度:" <<audioData->count << " 剩余数据包个数："<< samples->Count();
                    sendData(audioData->data,audioData->count);
                    audioDevice->microphone->lastFrameTime=currentTime;
                }
            }else{
                std::this_thread::sleep_for(SENDER_THREAD_SLEEP_MS*1ms);
            }
        }
        qInfo() << "发送麦克风数据线程结束";
    }

    void Microphone::sendData(const char* data,int length){
//        qDebug() << "正在向远程主机发送数据，长度:" <<length;
//        qint64 currentTime=QDateTime::currentMSecsSinceEpoch();
        LiSendAudioStreamEvent(data,length,0x61,0x12345678);
    }

    bool Microphone::pause(){
        qInfo() << "麦克风暂停工作";
        isSendingData=false;
        return true;
    }

    bool Microphone::resume() {
        qInfo() << "麦克风恢复工作";
        isSendingData=true;
        return true;
    }

    bool Microphone::stop(){
        qInfo() << "麦克风尝试停止工作";
        isSendingData=false;
        isWorking=false;
//        if(microphoneCheckThread.joinable())
//            microphoneCheckThread.join();
        if(senderThread.joinable())
            senderThread.join();
        if(soundio!=nullptr){
            audioDevice->closeStream();
            if(soundio->userdata!=nullptr){
                soundio->userdata=nullptr;
            }
            soundio_flush_events(soundio);
            soundio_disconnect(soundio);
        }
        if(audioDevice->microphone!=nullptr)
            audioDevice->microphone=nullptr;
        samples->empty();
//        soundio_destroy(soundio);
        qInfo() << "麦克风停止工作";
        closeFile();
        return true;
    }

    void Microphone::flushEvents(){
        soundio_flush_events(soundio);//刷新事件池
    }

    void Microphone::updateDevice() {
        if(isWorking)
            checkAndCreateDevice(soundio);
    }


    QString Microphone::getMicrophoneFormat(SoundIoFormat format){
        switch ((int)format) {
            case SoundIoFormat::SoundIoFormatS8:
                return "SoundIoFormatS8";
            case SoundIoFormat::SoundIoFormatU8:
                return "SoundIoFormatU8";
            case SoundIoFormat::SoundIoFormatS16LE:
                return "SoundIoFormatS16LE";
            case SoundIoFormat::SoundIoFormatS16BE:
                return "SoundIoFormatS16BE";
            case SoundIoFormat::SoundIoFormatU16LE:
                return "SoundIoFormatU16LE";
            case SoundIoFormat::SoundIoFormatU16BE:
                return "SoundIoFormatU16BE";
            case SoundIoFormat::SoundIoFormatS24LE:
                return "SoundIoFormatS24LE";
            case SoundIoFormat::SoundIoFormatS24BE:
                return "SoundIoFormatS24BE";
            case SoundIoFormat::SoundIoFormatU24LE:
                return "SoundIoFormatU24LE";
            case SoundIoFormat::SoundIoFormatU24BE:
                return "SoundIoFormatU24BE";
            case SoundIoFormat::SoundIoFormatS32LE:
                return "SoundIoFormatS32LE";
            case SoundIoFormat::SoundIoFormatS32BE:
                return "SoundIoFormatS32BE";
            case SoundIoFormat::SoundIoFormatU32LE:
                return "SoundIoFormatU32LE";
            case SoundIoFormat::SoundIoFormatU32BE:
                return "SoundIoFormatU32BE";
            case SoundIoFormat::SoundIoFormatFloat32LE:
                return "SoundIoFormatFloat32LE";
            case SoundIoFormat::SoundIoFormatFloat32BE:
                return "SoundIoFormatFloat32BE";
            case SoundIoFormat::SoundIoFormatFloat64LE:
                return "SoundIoFormatFloat64LE";
            case SoundIoFormat::SoundIoFormatFloat64BE:
                return "SoundIoFormatFloat64BE";
            default:
                return "unknown foramt";
        }
    }

    bool Microphone::createOpusEncoder(int sample_rate,int channels){
    //    opus_encoder_ctl(OpusEncoder *st, int request, ...)
        qInfo()<< "载入opus编码器："<< opus_get_version_string();
        /* OPUS_APPLICATION_VOIP：对语音信号进行处理，适用于voip 业务场景，最适合大多数VoIP/视频会议应用，其中听觉质量和可懂度最为关键
         * OPUS_APPLICATION_AUDIO：这个模式适用于音乐类型等非语音内容，最适合广播/高保真应用，要求解码音频尽可能接近输入信号
         * OPUS_APPLICATION_RESTRICTED_LOWDELAY：低延迟模式，仅在最重要的是可实现的最低延迟时使用。无法使用语音优化模式。
         */
        int err;
        opusEncoder= opus_encoder_create(sample_rate,channels,OPUS_APPLICATION_RESTRICTED_LOWDELAY,&err);
        if(err != OPUS_OK || opusEncoder == nullptr) {
            qCritical() << "打开opus 编码器失败";
            return false;
        }
        opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(SAMPLE_RATE)); //这个与上面的1是一样的效果。
        opus_encoder_ctl(opusEncoder, OPUS_SET_VBR(0));//0:CBR, 1:VBR 开启可变比特率（Variable Bitrate），使编码器能根据语音复杂度动态调整输出大小
//        opus_encoder_ctl(opusEncoder, OPUS_SET_VBR_CONSTRAINT(true));
        //opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(8));//8    0~10
//        opus_encoder_ctl(opusEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); //设置为语音
//        opus_encoder_ctl(opusEncoder, OPUS_SET_LSB_DEPTH(16));//每个采样16个bit，2个byte
//        opus_encoder_ctl(opusEncoder, OPUS_SET_DTX(0));//启用非连续传输（DTX），在无声段不发送完整编码帧，节省带宽。
//        opus_encoder_ctl(opusEncoder, OPUS_SET_INBAND_FEC(1));//启用带内前向纠错，在关键语音帧中嵌入冗余数据；
//        opus_encoder_ctl(opusEncoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));//设置最大带宽   48khz 只需要设置OPUS_BANDWIDTH_WIDEBAND 8k 就够了

        int level=0;
        opus_encoder_ctl(opusEncoder, OPUS_GET_COMPLEXITY(&level));//8    0~10
        qInfo()<< "opus编码器：创建成功！！！ 复杂度：" <<level;
        return true;
    }

    void Microphone::resample(float* in,int sampleRateIn,int frameCountIn,float* out,int sampleRateOut,int& frameCountOut){
       // int newFrameCount=frameCountIn*sampleRateOut/sampleRateIn;
       double ratio = static_cast<double>(sampleRateOut) / sampleRateIn;
       frameCountOut=frameCountIn*ratio;
       for (int i = 0; i < frameCountOut; ++i) {
            double inIndex = i / ratio;
            int index1 = static_cast<int>(std::floor(inIndex));
            //float frac = inIndex - index1;
            if(inIndex==index1){ //刚好在索引上
                out[i] = in[index1];
            }else if (index1 >= 0 && index1 < frameCountIn - 1) {//如果不在索引上，则取前后2帧数据的线性插值
                float last=in[index1];
                float next=in[index1 + 1];
                out[i] = linearInterpolate(index1, last, index1 + 1,next, inIndex);
            } else { //大采样率转小采样率
                // 处理边界情况，可以根据需求选择填充方式，如复制边界值
                out[i] = in[0];
            }
       }
    }

    // void Microphone::toPcm16(float in,uint16_t &out){
    //     out= (uint16_t) (in*PCM_16_PARAM);
    // }

    // void Microphone::fromPcm16(uint16_t in,float& out){
    //     out=(float) in/PCM_16_PARAM;
    // }

    int Microphone::getNearestSampleRate(std::vector<int> sampleRates,int targetSampleRate){
        int best_rate = -1;
        int best_delta = -1;
        for (int i = 0; i < sampleRates.size(); i ++) {
            int candidate_rate = sampleRates[i];
            if (candidate_rate == targetSampleRate)
                return candidate_rate;
            int delta = std::abs(candidate_rate-targetSampleRate);
            bool best_rate_too_small = best_rate < targetSampleRate;
            bool candidate_rate_too_small = candidate_rate < targetSampleRate;
            if (best_rate == -1 ||
                (best_rate_too_small && !candidate_rate_too_small) ||
                ((best_rate_too_small || !candidate_rate_too_small) && delta < best_delta))
            {
                best_rate = candidate_rate;
                best_delta = delta;
            }
        }
        return best_rate;
    }
}
