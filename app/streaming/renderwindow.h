#pragma once

#include <QSemaphore>
#include <QWindow>

#include <Limelight.h>
#include <opus_multistream.h>
#include "settings/streamingpreferences.h"
#include "input/input.h"
#include "video/decoder.h"
#include "video/overlaymanager.h"
#include "backend/richpresencemanager.h"
#include "session.h"

class Session;

class RenderWindow : public QObject
{
    Q_OBJECT

    friend class SdlInputHandler;
    friend class Session;
private:
    Session* session;

    StreamingPreferences* m_Preferences;
    bool m_IsFullScreen;
    Uint32 m_FullScreenFlag;
    STREAM_CONFIGURATION m_StreamConfig;

    SDL_Window* m_Window;
    SdlInputHandler* m_InputHandler;
    QWindow* m_QtWindow;

    IVideoDecoder* m_VideoDecoder;
    SDL_SpinLock m_DecoderLock;

    Overlay::OverlayManager m_OverlayManager;
    //默认的显示索引，用于接收服务器对应的流
    int trackIndex;
    //客户端可能会调整窗口位置，实际的显示索引，用于sdl窗口计算
    int currentDisplayIndex;
    bool needsFirstEnterCapture;
    bool needsPostDecoderCreationCapture;

    int m_MouseEmulationRefCount;

    SDL_Surface* iconSurface;
    void updateOptimalWindowDisplayMode();
signals:

public:
    virtual ~RenderWindow() {};
    explicit RenderWindow(Session* _session);

    Overlay::OverlayManager& getOverlayManager()
    {
        return m_OverlayManager;
    }
    int getWindowId(){
        return SDL_GetWindowID(m_Window);
    }

    int getTrackIndex(){
        return trackIndex;
    }
    IVideoDecoder* getVideoDecoder(){
        return m_VideoDecoder;
    }

    bool createWindow(int index,std::string windowName,Uint32 defaultWindowFlags);
    bool handleEvents(SDL_Event event,RichPresenceManager presence);
    void toggleFullscreen();
    void notifyMouseEmulationMode(bool enabled);

    void setHdrMode(bool enabled);
    
    int submitDecodeUnit(PDECODE_UNIT du);

    /**
     * @brief getWindowDimensions 获取窗口的维度信息
     * @param displayIndex 显示索引，如果传入-1，则根据父窗口或已经创建的窗口自动计算
     * @param x 返回的x坐标
     * @param y 返回的y坐标
     * @param width 返回的宽度
     * @param height 返回的高度
     */
    int getWindowDimensions(int displayIndex,int& x, int& y,
                            int& width, int& height);

    void close();
};



