//
// Created by dilin on 8/11/17.
//

#ifndef PEOPLETECKERV2_DETECTOR_H
#define PEOPLETECKERV2_DETECTOR_H

#include <iostream>
#include <opencv2/opencv.hpp>

class Detector
{
public:
    virtual std::vector<cv::Rect> detect(cv::Mat &img) = 0;
    std::vector<cv::MatND> histograms;
    static cv::MatND getHistogram(cv::Mat img,cv::Rect region);
};


#endif //PEOPLETECKERV2_DETECTOR_H
