//
// Created by dilin on 10/23/17.
//

#include "BGSDetector.h"

std::vector<cv::Rect> BGSDetector::detect(cv::Mat &img)
{
    std::vector<cv::Rect> detections,found;
    histograms.clear();
    mask = img.clone();

    cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::Mat structuringElement9x9 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));

    /*
    cv::dilate(imgThresh, imgThresh, structuringElement7x7);
    cv::erode(imgThresh, imgThresh, structuringElement3x3);
    */

    cv::dilate(mask, mask, structuringElement5x5);
    cv::dilate(mask, mask, structuringElement5x5);
    cv::erode(mask, mask, structuringElement5x5);



    std::vector<std::vector<cv::Point> > contours;

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point> > convexHulls(contours.size());

    for (unsigned int i = 0; i < contours.size(); i++)
    {
        cv::convexHull(contours[i], convexHulls[i]);
    }

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
            histograms.push_back(cv::MatND());
        }

    }

    return detections;
}

BGSDetector::BGSDetector(double TH) :
TH(TH)
{
}

Blob::Blob(std::vector<cv::Point> _contour)
{
    currentContour = _contour;

    currentBoundingRect = cv::boundingRect(currentContour);

    cv::Point currentCenter;

    currentCenter.x = (currentBoundingRect.x + currentBoundingRect.x + currentBoundingRect.width) / 2;
    currentCenter.y = (currentBoundingRect.y + currentBoundingRect.y + currentBoundingRect.height) / 2;

    centerPositions.push_back(currentCenter);

    dblCurrentDiagonalSize = sqrt(pow(currentBoundingRect.width, 2) + pow(currentBoundingRect.height, 2));

    dblCurrentAspectRatio = (float)currentBoundingRect.width / (float)currentBoundingRect.height;

    blnStillBeingTracked = true;
    blnCurrentMatchFoundOrNewBlob = true;

    intNumOfConsecutiveFramesWithoutAMatch = 0;
}
