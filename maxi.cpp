// #include "node/MyTypes.h"
// #include "node/NodeClient.h"

#include "drivers/xbacksub.h"
#include "drivers/xfeature.h"

#include "include/xparameters.h"
#include <chrono>
#include <string.h>
#include <fstream>
#include <iostream>

#include "detection/MyTypes.h"
#include "detection/NodeClient.h"
#include "detection/BGSDetector.h"

//V4L2 Includes

#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <string>
//#include <termios.h>
//#include<opencv2/opencv.hpp>



/***************** Macros (Inline Functions) Definitions *********************/

#define TX_BASE_ADDR 0x01000000
#define DDR_RANGE 0x01000000
#define RX_BASE_ADDR 0x02000000

#define AXILITES_BASEADDR 0x43C00000
#define CRTL_BUS_BASEADDR 0x43C10000
#define AXILITE_RANGE 0xFFFF

#define M_AXI_BOUNDING 0x21000000
#define M_AXI_FEATUREH 0x29000000

using namespace cv;
using namespace std;


/***************** Global Variables *********************/


XBacksub backsub;
XFeature feature;

int fdIP;
uint8_t * ybuffer = new uint8_t[N];


int feature_init(XFeature * ptr){
    ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_AXILITES_BASEADDR);
    ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_FEATURE_0_S_AXI_CRTL_BUS_BASEADDR);
    ptr->IsReady = XIL_COMPONENT_IS_READY;
    return 0;
}

void feature_rel(XFeature * ptr){
    munmap((void*)ptr->Crtl_bus_BaseAddress, AXILITE_RANGE);
    munmap((void*)ptr->Axilites_BaseAddress, AXILITE_RANGE);
}

void feature_config() {
    printf("config\n");
    XFeature_Set_frame_in(&feature,(u32)TX_BASE_ADDR);
    XFeature_Set_bounding(&feature,(u32)M_AXI_BOUNDING);
    XFeature_Set_featureh(&feature,(u32)M_AXI_FEATUREH);
}



int backsub_init(XBacksub * backsub_ptr){
    backsub_ptr->Axilites_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_BACKSUB_0_S_AXI_AXILITES_BASEADDR);
    backsub_ptr->Crtl_bus_BaseAddress = (u32)mmap(NULL, AXILITE_RANGE, PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, XPAR_XBACKSUB_0_S_AXI_CRTL_BUS_BASEADDR);
    backsub_ptr->IsReady = XIL_COMPONENT_IS_READY;
    return 0;
}

void backsub_rel(XBacksub * backsub_ptr){
    munmap((void*)backsub_ptr->Axilites_BaseAddress, AXILITE_RANGE);
    munmap((void*)backsub_ptr->Crtl_bus_BaseAddress, AXILITE_RANGE);
}

void backsub_config(bool ini) {
    printf("config\n");
    XBacksub_Set_frame_in(&backsub,(u32)TX_BASE_ADDR);
    printf("config1\n");
    XBacksub_Set_frame_out(&backsub,(u32)RX_BASE_ADDR);
    printf("config2\n");
    XBacksub_Set_init(&backsub, ini);
}

void print_config() {
    printf("Is Ready = %d \n", XBacksub_IsReady(&backsub));
    printf("Frame in = %X \n", XBacksub_Get_frame_in(&backsub));
    printf("Frame out = %X \n", XBacksub_Get_frame_out(&backsub));
    printf("Init = %d \n", XBacksub_Get_init(&backsub));
}


int main(int argc, char *argv[]) {

    // Initialization communication link
    // NodeClient client("10.8.145.198",8080);
    // client.connect();

    // Initializing IP Core Starts here .........................
    fdIP = open ("/dev/mem", O_RDWR);
    if (fdIP < 1) {
        perror(argv[0]);
        return -1;
    }


    uint32_t * src = (uint32_t*)mmap(NULL, DDR_RANGE,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, TX_BASE_ADDR); 
    uint8_t * dst = (uint8_t*)mmap(NULL, DDR_RANGE,PROT_EXEC|PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, RX_BASE_ADDR); 


    uint16_t * m_axi_bound = (uint16_t*)mmap(NULL, 80,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_BOUNDING);
    uint16_t * m_axi_feature = (uint16_t*)mmap(NULL, 5120*2,PROT_READ|PROT_WRITE, MAP_SHARED, fdIP, M_AXI_FEATUREH);


    if(backsub_init(&backsub)==0) {
        printf("Backsub IP Core Initialized\n");
    }

    if(feature_init(&feature)==0) {
        printf("Feature IP Core Initialized\n");
    }
    // Initializing IP Core Ends here .........................

    
    /******************Initializing V4L2 Driver Starts Here**********************/
    // 1.  Open the device
    int fd; // A file descriptor to the video device
    fd = open("/dev/video0",O_RDWR);
    if(fd < 0){
        perror("Failed to open device, OPEN");
        return 1;
    }

    // 2. Ask the device if it can capture frames
    v4l2_capability capability;
    if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0){
        // something went wrong... exit
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        return 1;
    }

    // 3. Set Image format
    v4l2_format imageFormat;
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = 320;
    imageFormat.fmt.pix.height = 240;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    // tell the device you are using this format
    if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
        perror("Device could not set format, VIDIOC_S_FMT");
        return 1;
    }

    // 4. Request Buffers from the device
    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 1; // one request buffer
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer wich we an use for capturing frames
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0){
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        return 1;
    }


    // 5. Quety the buffer to get raw data ie. ask for the you requested buffer
    // and allocate memory for it
    v4l2_buffer queryBuffer = {0};
    queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuffer.memory = V4L2_MEMORY_MMAP;
    queryBuffer.index = 0;
    if(ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0){
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        return 1;
    }
    // use a pointer to point to the newly created buffer
    // mmap() will map the memory address of the device to
    // an address in memory
    char* buffer = (char*)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, queryBuffer.m.offset);
    memset(buffer, 0, queryBuffer.length);


    // 6. Get a frame
    // Create a new buffer type so the device knows whichbuffer we are talking about
    v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    // Activate streaming
    int type = bufferinfo.type;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("Could not start streaming, VIDIOC_STREAMON");
        return 1;
    }
    /******************Initializing V4L2 Driver Ends Here**********************/


    /***************************** Begin looping here *********************/
    auto begin = std::chrono::high_resolution_clock::now();
    bool isFirst = true;
    for (int it=0;it<1;it++){
        // Queue the buffer
       // auto begin = std::chrono::high_resolution_clock::now();
        if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0){
            perror("Could not queue buffer, VIDIOC_QBUF");
            return 1;
        }

        // Dequeue the buffer
        if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0){
            perror("Could not dequeue the buffer, VIDIOC_DQBUF");
            return 1;
        }


        int outFileMemBlockSize = bufferinfo.bytesused;
        int remainingBufferSize = bufferinfo.bytesused;


        printf("t1\n");
        for(int j=0;j<N;j++)
        {
            ybuffer[j] = buffer[2*j];
        }
        printf("t2\n");

        memcpy(src,buffer,sizeof(uint32_t)*N/2);
        printf("t3\n");
        print_config();
        if (isFirst){
            backsub_config(true);
            isFirst = false;
        }
        else{
            backsub_config(false);
        }
        printf("t4\n");


        XBacksub_Start(&backsub);

        while(!XBacksub_IsDone(&backsub));
        printf("backsub finished\n");


        // Contour detection using opencv
        printf("test1\n");
        Mat mask = Mat(240, 320, CV_8UC1, dst); 

        std::vector<cv::Rect> detections,found;
        
        // cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        // cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
        // cv::Mat structuringElement9x9 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
        
            cv::dilate(mask, mask, structuringElement5x5);
            cv::dilate(mask, mask, structuringElement5x5);
            cv::erode(mask, mask, structuringElement5x5);
            
            std::vector<std::vector<cv::Point> > contours;
        
            // contour detection
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
            std::vector<std::vector<cv::Point> > convexHulls(contours.size());
        
            for (unsigned int i = 0; i < contours.size(); i++)
            {
                cv::convexHull(contours[i], convexHulls[i]);
            }
            printf("test10\n");
            // convex hulls
            for (auto &convexHull : convexHulls) {
                Blob possibleBlob(convexHull);
        
                if (possibleBlob.currentBoundingRect.area() > 100 &&
                    possibleBlob.dblCurrentAspectRatio >= 0.2 &&
                    possibleBlob.dblCurrentAspectRatio <= 1.25 &&
                    possibleBlob.currentBoundingRect.width > 20 &&
                    possibleBlob.currentBoundingRect.height > 20 &&
                    possibleBlob.dblCurrentDiagonalSize > 30.0 &&
                    (cv::contourArea(possibleBlob.currentContour) /
                     (double)possibleBlob.currentBoundingRect.area()) > 0.40)
                {
                    found.push_back(possibleBlob.currentBoundingRect);
                }
            }
            printf("test11\n");
            size_t i, j;
        
            for (i=0; i<found.size(); i++)
            {
                cv::Rect r = found[i];
                for (j=0; j<found.size(); j++)
                    if (j!=i && (r & found[j])==r)
                        break;
                if (j==found.size())
                {
                    r.x += cvRound(r.width*0.1);
                    r.width = cvRound(r.width*0.8);
                    r.y += cvRound(r.height*0.07);
                    r.height = cvRound(r.height*0.8);
                    detections.push_back(r);
                }
        
            }
            printf("test12\n");
            for (int k=0;k<10;k++){
                m_axi_bound[k*4+0] = 64;//detections.at(k).x;
                m_axi_bound[k*4+1] = 64;//detections.at(k).y;
                m_axi_bound[k*4+2] = 128;//detections.at(k).x + detections.at(k).width;
                m_axi_bound[k*4+3] = 128;//detections.at(k).y + detections.at(k).height;
                printf("testloop %d \n",k);
            }
        printf("test2\n");
        feature_config();
        printf("test3\n");
        XFeature_Start(&feature);
        
        while(!XFeature_IsDone(&feature));
        printf("feature finished\nPrinting first histogram :\n");

        for (int h=0;h<512;h++){
            printf("%d, ",m_axi_feature[h]);
        }
        printf("\n");
        //client.sendBinMask(dst);

        // outFile.close();
        //auto end = std::chrono::high_resolution_clock::now();
        //printf("Elapsed time : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count());

        // char c=getch();
        // if (c=='q')
        //   break;

    }

    auto end = std::chrono::high_resolution_clock::now();
    /***************************** End looping here *********************/
//printf("Elapsed time : %lld us\n",std::chrono::duration_cast<std::chrd::chrono::microseconds>(end-begin).count());
    // end streaming
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        return 1;
    }

    for (int i=0;i<100;i++){
        printf("src : %d , dst : %d \n",ybuffer[i],dst[i]);
    }

    close(fd);

    //Release IP Core
    backsub_rel(&backsub);
    feature_rel(&feature);

    munmap((void*)src, DDR_RANGE);
    munmap((void*)dst, DDR_RANGE);

    close(fdIP);
     
    printf("Elapsed time : %lld us\n",std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count());

    printf("Device unmapped\n");

    return 0;
}

