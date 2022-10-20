#include <opencv2/opencv.hpp>

#include <iostream>
#include <queue>
#include <chrono>
#include <pthread.h>
#include <cstdio>

using namespace std;
// using namespace cv;

// struct capture_resource {
//     cv::VideoCapture* cap_p;
//     std::queue<cv::Mat>* frame_buffer_p;
//     unsigned int buffer_size;
//     bool end_stream;
// };

// void* read_frames(void* arg) {
//     capture_resource* resrc = (capture_resource*) arg;
//     cv::Mat img;
//     while (!resrc->end_stream) {
//         if (resrc->frame_buffer_p->size() < resrc->buffer_size) {
//             resrc->cap_p->read(img);
//             // Get the size of a frame in bytes
//             int imgSize = img.total() * img.elemSize();
//             resrc->frame_buffer_p->push(img.clone());
//             if (imgSize == 0) {
//                 goto read_frames_exit;
//             }
//         }
//     }
// read_frames_exit:
//     pthread_exit(0);
// }

int main(int argc, char *argv[]) {
    cv::Mat server_img, client_img;
    cv::VideoCapture cap("./dQw4w.mpg");
    // cv::VideoCapture* cap_p = (cv::VideoCapture*)malloc(sizeof(cv::VideoCapture));
    // *cap_p = cv::VideoCapture("./dQw4w.mpg");

    // Get the resolution of the video
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double frame_time = 1000/cap.get(cv::CAP_PROP_FPS);
    cout << "Video resolution: " << width << ", " << height << endl;
    cout << "Video frame rate: " << 1000/frame_time << "FPS" << endl;

    // std::queue<cv::Mat> frame_buffer;
    // queue<cv::Mat>* frame_buffer_p = (queue<cv::Mat>*)malloc(sizeof(queue<cv::Mat>));
    // *frame_buffer_p = queue<cv::Mat>();

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
    auto ts_intvl = std::chrono::milliseconds((int64_t)frame_time);
    int frame_count = 0;
    
    cap >> server_img;
    imshow("Video", server_img);
    video_st = std::chrono::system_clock::now();
    ts_prev = std::chrono::system_clock::now();
    
    // capture_resource* resrc = (capture_resource*)malloc(sizeof(capture_resource));
    // resrc->cap_p = cap_p;
    // resrc->frame_buffer_p = frame_buffer_p;
    // resrc->buffer_size = 100;
    // resrc->end_stream = false;
    // pthread_t tid;
    // pthread_create(&tid, NULL, &read_frames, (void*)resrc);
    // struct capture_resource resrc{cap, frame_buffer, 100};

    while (1) {
        // Get a frame from the video to the container of the server.
        cap >> server_img;
        // // Get the size of a frame in bytes
        int imgSize = server_img.total() * server_img.elemSize();
        // frame_buffer.push(server_img.clone());
        // frame_buffer.pop();

        uchar buffer[imgSize];

        memcpy(buffer, server_img.data, imgSize);

        uchar *iptr = client_img.data;
        memcpy(iptr, buffer, imgSize);

        // if (std::chrono::system_clock::now() - ts_prev > ts_intvl && frame_buffer_p->size()>0) {
        //     cv::imshow("Video", frame_buffer_p->front());
        //     frame_buffer_p->pop();
        //     frame_count++;
        //     ts_prev = std::chrono::system_clock::now();
        // }
        // char c = (char)cv::waitKey(1);
        // if (c == 27) {
        //     resrc->end_stream = true;
        //     break;
        // }

        // Old method
        cv::imshow("Video", client_img);
        frame_count++;
        char c = -1;
        while (std::chrono::system_clock::now() - ts_prev < ts_intvl
               && (c = (char)cv::waitKey(1)) != 27) {   
        }
        ts_prev = std::chrono::system_clock::now();
        if (c == 27)
            break;

        // Naive method
        // char c = (char)waitKey(frame_time);
        // if (c == 27) break;
    }


    std::chrono::duration<double> video_time = std::chrono::system_clock::now() - video_st;
    fprintf(stderr, "[video] Successfully received %d frames in %.2f s\n", frame_count, video_time.count());
    fprintf(stderr, "[video] Average frame rate: %.2f FPS\n", frame_count/video_time.count());

    // pthread_join(tid, NULL);

    cap.release();
    cv::destroyAllWindows();
    // free(resrc);
    // free(cap_p);
    // free(frame_buffer_p);
    return 0;
}
