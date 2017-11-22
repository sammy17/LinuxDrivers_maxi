//
// Created by dilin on 10/23/17.
//

#ifndef TRACK_BGSDETECTOR_H
#define TRACK_BGSDETECTOR_H
#include <opencv2/opencv.hpp>
#include "Detector.h"

class Blob {
public:
    // member variables ///////////////////////////////////////////////////////////////////////////
    Blob(std::vector<cv::Point> _contour);
    std::vector<cv::Point> currentContour;

    cv::Rect currentBoundingRect;

    std::vector<cv::Point> centerPositions;

    double dblCurrentDiagonalSize;
    double dblCurrentAspectRatio;

    bool blnCurrentMatchFoundOrNewBlob;

    bool blnStillBeingTracked;

    int intNumOfConsecutiveFramesWithoutAMatch;

    cv::Point predictedNextPosition;
};

class BGSDetector : public Detector
{
public:
    explicit BGSDetector(double TH=15);
    std::vector<cv::Rect> detect(cv::Mat &img);

private:
    void backgroundSubstraction(cv::Mat &frame0, cv::Mat &frame1, cv::Mat &frame2
            , cv::Mat &bgModel, cv::Mat &mask, double TH=15);
    cv::Mat mask;
    double TH;
};


#endif //TRACK_BGSDETECTOR_H
