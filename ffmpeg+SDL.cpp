#include <iostream>
extern "C" {
    #include <SDL.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
}

int main(int argc, char* argv[]) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 创建窗口
    SDL_Window* window = SDL_CreateWindow("SDL2 Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 创建纹理，使用 YUV420P 格式
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 1920, 1080);
    if (texture == nullptr) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 视频文件路径
    std::string filename = "E:\\VS_Code\\C++projects\\ffmpeg+SDL2\\SZUEA.mp4";

    // 打开输入文件
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Failed to retrieve stream information" << std::endl;
        return -1;
    }

    // 找到视频流
    int videoStream = -1;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        std::cerr << "No video stream found" << std::endl;
        return -1;
    }

    // 获取编解码参数
    AVCodecParameters* codecpar = formatCtx->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Failed to find decoder" << std::endl;
        return -1;
    }

    // 分配编解码上下文
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        std::cerr << "Failed to allocate codec context" << std::endl;
        return -1;
    }

    // 拷贝参数到编解码上下文
    if (avcodec_parameters_to_context(codecCtx, codecpar) < 0) {
        std::cerr << "Failed to copy codec parameters to codec context" << std::endl;
        return -1;
    }

    // 打开编解码器
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        std::cerr << "Could not open codec" << std::endl;
        return -1;
    }

    AVPacket packet;
    AVFrame* frame = av_frame_alloc();

    // SDL 事件处理
    SDL_Event event;
    bool running = true;

    while (running) {
        // 读取帧
        if (av_read_frame(formatCtx, &packet) >= 0) {
            if (packet.stream_index == videoStream) {
                avcodec_send_packet(codecCtx, &packet);
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    // 更新纹理
                    SDL_UpdateYUVTexture(texture, nullptr,
                        frame->data[0], frame->linesize[0],
                        frame->data[1], frame->linesize[1],
                        frame->data[2], frame->linesize[2]);

                    // 渲染
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);

                    // 控制帧率
                    SDL_Delay(33.34); // 控制每秒帧数为30
                }
            }
            av_packet_unref(&packet);
        }

        // 处理 SDL 事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }

    // 清理
    avcodec_free_context(&codecCtx);
    av_frame_free(&frame);
    avformat_close_input(&formatCtx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}