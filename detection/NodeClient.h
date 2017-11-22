//
// Created by dilin on 10/18/17.
//

#ifndef NODE_NODECLIENT_H
#define NODE_NODECLIENT_H

#include "MyTypes.h"

class NodeClient
{
public:
    NodeClient(string ip,int port);
    void connect();
    void sendBinMask(Mat binMask);

private:
    boost::asio::io_service io_service;
    tcp::resolver resolver;
    tcp::socket socket;
    string ip;
    string port;
    boost::array<uchar,N> buffer;
    void matToBitBuffer(Mat &binMask,boost::array<uchar,N> &buffer);
};


#endif //NODE_NODECLIENT_H
