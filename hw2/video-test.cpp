#include <opencv2/opencv.hpp>

#include <iostream>
#include <queue>
#include <chrono>
#include <pthread.h>
#include <cstdio>
#include <unistd.h>

using namespace std;
// using namespace cv;

struct cap_resrc {
    cv::VideoCapture* cap_p;
    std::queue<cv::Mat>* frame_buffer_p;
    unsigned int buffer_size;
    bool ended;
    bool finished;
};

void* read_frames(void* arg) {
    cap_resrc* resrc = static_cast<cap_resrc*>(arg);
    cv::Mat img;
    while (!resrc->ended) {
        if (resrc->frame_buffer_p->size() < resrc->buffer_size) {
            resrc->cap_p->read(img);
            // Get the size of a frame in bytes
            int imgSize = img.total() * img.elemSize();
            resrc->frame_buffer_p->push(img.clone());
            if (imgSize == 0) {
                goto read_frames_exit;
            }
        } else {
            usleep(10*1000);
        }
    }
read_frames_exit:
    resrc->finished = true;
    pthread_exit(0);
}

void threadRead() {
    cv::Mat server_img, client_img;
    cv::VideoCapture cap("./dQw4w.mpg");

    // Get the resolution of the video
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double frame_time = 1000/cap.get(cv::CAP_PROP_FPS);
    cout << "Video resolution: " << width << ", " << height << endl;
    cout << "Video frame rate: " << 1000/frame_time << "FPS" << endl;

    std::queue<cv::Mat> frame_buffer;

    client_img = cv::Mat::zeros(height, width, CV_8UC3);

    if (!client_img.isContinuous()) {
        client_img = client_img.clone();
    }

    auto video_st = std::chrono::system_clock::now();
    auto ts_prev = std::chrono::system_clock::now();
    auto ts_intvl = std::chrono::milliseconds((int)frame_time);
    int frame_count = 0;
    
    cap_resrc res;
    
    res.cap_p = &cap;
    res.frame_buffer_p = &frame_buffer;
    res.buffer_size = cap.get(cv::CAP_PROP_FPS)*3;
    res.ended = false;
    res.finished = false;
    pthread_t tid;

    std::chrono::duration<double> video_time;
    cap >> client_img;
    cv::imshow("Video", client_img);
    pthread_create(&tid, NULL, &read_frames, (void*)&res);
    video_st = std::chrono::system_clock::now();
    ts_prev = std::chrono::system_clock::now();

    while (!res.finished || frame_buffer.size()) {
        if (frame_buffer.size()>0 && std::chrono::system_clock::now() - ts_prev > ts_intvl) {
            cv::imshow("Video", frame_buffer.front());
            frame_buffer.pop();
            frame_count++;
            if (frame_count == 1) { // Ignore first frame
                video_st = std::chrono::system_clock::now();
            }
            ts_prev = std::chrono::system_clock::now();
        }
        if (cv::waitKey(1) == 27) {
            break;
        }
    }
    res.ended = true;
    video_time = std::chrono::system_clock::now() - video_st;
    
    fprintf(stderr, "[video] Successfully received %d frames in %.2f s\n", frame_count, video_time.count());
    fprintf(stderr, "[video] Average frame rate: %.2f FPS\n", (frame_count-1)/video_time.count());

    pthread_join(tid, NULL);

    cap.release();
    cv::destroyAllWindows();
}

void normalRead() {
    cv::Mat server_img, client_img;
    cv::VideoCapture cap("./dQw4w.mpg");
    
    // Get the resolution of the video
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double frame_time = 1000/cap.get(cv::CAP_PROP_FPS);
    cout << "Video resolution: " << width << ", " << height << endl;
    cout << "Video frame rate: " << 1000/frame_time << "FPS" << endl;

    // Allocate container to load frames
    server_img = cv::Mat::zeros(height, width, CV_8UC3);
    client_img = cv::Mat::zeros(height, width, CV_8UC3);

    // Ensure the memory is continuous (for efficiency issue.)
    if (!server_img.isContinuous()) {
        server_img = server_img.clone();
    }

    if (!client_img.isContinuous()) {
        client_img = client_img.clone();
    }

    auto video_st = std::chrono::system_clock::now();
    auto ts_prev = std::chrono::system_clock::now();
    auto ts_intvl = std::chrono::milliseconds((int)frame_time);
    int frame_count = 0;
    char key_pressed = -1;
    
    std::chrono::duration<double> video_time;
    cap >> server_img;
    cv::imshow("Video", server_img);
    video_st = std::chrono::system_clock::now();
    ts_prev = std::chrono::system_clock::now();

    while (1) {
        // Get a frame from the video to the container of the server.
        cap >> server_img;
        // // Get the size of a frame in bytes
        int imgSize = server_img.total() * server_img.elemSize();
        
        uchar buffer[imgSize];
        memcpy(buffer, server_img.data, imgSize);
        uchar *iptr = client_img.data;
        memcpy(iptr, buffer, imgSize);

        // Old method
        cv::imshow("Video", client_img);
        if (frame_count == 1) { // Ignore first frame
            video_st = std::chrono::system_clock::now();
        }
        frame_count++;
        ts_prev = std::chrono::system_clock::now();
        while (std::chrono::system_clock::now() - ts_prev < ts_intvl
               && (key_pressed = (char)cv::waitKey(1)) != 27) {   
        }
        if (key_pressed == 27)
            break;
    }
    video_time = std::chrono::system_clock::now() - video_st;
    
    fprintf(stderr, "[video] Successfully received %d frames in %.2f s\n", frame_count, video_time.count());
    fprintf(stderr, "[video] Average frame rate: %.2f FPS\n", (frame_count-1)/video_time.count());

    cap.release();
    cv::destroyAllWindows();
}

int main(int argc, char *argv[]) {
    // cv::Mat server_img, client_img;
    // cv::VideoCapture cap("./dQw4w.mpg");
    

    // // Get the resolution of the video
    // int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    // int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    // double frame_time = 1000/cap.get(cv::CAP_PROP_FPS);
    // cout << "Video resolution: " << width << ", " << height << endl;
    // cout << "Video frame rate: " << 1000/frame_time << "FPS" << endl;

    // std::queue<cv::Mat> frame_buffer;

    // // Allocate container to load frames
    // server_img = cv::Mat::zeros(height, width, CV_8UC3);
    // client_img = cv::Mat::zeros(height, width, CV_8UC3);

    // // Ensure the memory is continuous (for efficiency issue.)
    // if (!server_img.isContinuous()) {
    //     server_img = server_img.clone();
    // }

    // if (!client_img.isContinuous()) {
    //     client_img = client_img.clone();
    // }

    // auto video_st = std::chrono::system_clock::now();
    // auto ts_prev = std::chrono::system_clock::now();
    // auto ts_intvl = std::chrono::milliseconds((int64_t)frame_time);
    // int frame_count = 0;
    
    
    // cap_resrc res;
    
    // res.cap_p = &cap;
    // res.frame_buffer_p = &frame_buffer;
    // res.buffer_size = cap.get(cv::CAP_PROP_FPS)*10;
    // res.ended = false;
    // res.finished = false;
    // pthread_t tid;

    // std::chrono::duration<double> video_time;
    // cap >> client_img;
    // cv::imshow("Video", client_img);
    // pthread_create(&tid, NULL, &read_frames, (void*)&res);
    // video_st = std::chrono::system_clock::now();
    // ts_prev = std::chrono::system_clock::now();

    // while (!res.finished || frame_buffer.size()) {
    //     // Get a frame from the video to the container of the server.
    //     // cap >> server_img;
    //     // // Get the size of a frame in bytes
    //     // int imgSize = server_img.total() * server_img.elemSize();
        
    //     // uchar buffer[imgSize];
    //     // memcpy(buffer, server_img.data, imgSize);
    //     // uchar *iptr = client_img.data;
    //     // memcpy(iptr, buffer, imgSize);

    //     if (frame_buffer.size()>0 && std::chrono::system_clock::now() - ts_prev > ts_intvl) {
    //         cv::imshow("Video", frame_buffer.front());
    //         frame_buffer.pop();
    //         frame_count++;
    //         ts_prev = std::chrono::system_clock::now();
    //     }
    //     if ((char)cv::pollKey() == 27) {
    //         res.ended = true;
    //         break;
    //     }

    //     // // Old method
    //     // cv::imshow("Video", client_img);
    //     // frame_count++;
    //     // char c = -1;
    //     // while (std::chrono::system_clock::now() - ts_prev < ts_intvl
    //     //        && (c = (char)cv::waitKey(1)) != 27) {   
    //     // }
    //     // ts_prev = std::chrono::system_clock::now();
    //     // if (c == 27)
    //     //     break;

    //     // Naive method
    //     // char c = (char)waitKey(frame_time);
    //     // if (c == 27) break;
    // }
    // video_time = std::chrono::system_clock::now() - video_st;
    
    // fprintf(stderr, "[video] Successfully received %d frames in %.2f s\n", frame_count, video_time.count());
    // fprintf(stderr, "[video] Average frame rate: %.2f FPS\n", frame_count/video_time.count());

    // pthread_join(tid, NULL);

    // cap.release();
    // cv::destroyAllWindows();
    // // free(resrc);
    // // free(cap_p);
    // // free(frame_buffer_p);
    threadRead();
    normalRead();
    return 0;
}
