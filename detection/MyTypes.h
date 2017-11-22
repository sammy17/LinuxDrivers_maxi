//
// Created by dilin on 10/18/17.
//

#ifndef NODE_MYTYPES_H
#define NODE_MYTYPES_H

#include <iostream>
#include<string>
#include<opencv2/opencv.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace std;
using namespace cv;
using boost::asio::ip::tcp;

typedef unsigned char uchar;

#define WIDTH 640
#define HEIGHT 480
#define BUF_SIZE(W,H) (W*H)/8+5
#define N BUF_SIZE(WIDTH,HEIGHT)
//#define FPS 30

#endif //NODE_MYTYPES_H
