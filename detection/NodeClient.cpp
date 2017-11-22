//
// Created by dilin on 10/18/17.
//

#include "NodeClient.h"

NodeClient::NodeClient(string ip, int port):
resolver(io_service),
socket(io_service)
{
    this->ip = ip;
    this->port = to_string(port);
}

void NodeClient::connect()
{
    tcp::resolver::query query(ip, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    boost::system::error_code error = boost::asio::error::host_not_found;

    while (error && endpoint_iterator != end)
    {
        socket.close();
        socket.connect(*endpoint_iterator++, error);
    }
    if (error)
        throw boost::system::system_error(error);
}



void NodeClient::sendBinMask(Mat binMask)
{
    boost::system::error_code error;
    matToBitBuffer(binMask,buffer);
    write(socket,boost::asio::buffer(buffer,N), error);
    if (error)
        throw boost::system::system_error(error); // Error has occured
}

void NodeClient::matToBitBuffer(Mat &binMask, boost::array<uchar, N> &buffer)
{
    assert(binMask.rows==HEIGHT && binMask.cols==WIDTH);

    buffer[0] = (uchar)(binMask.rows & 255);
    buffer[1] = (uchar)(binMask.rows  >> 8);
    buffer[2] = (uchar)(binMask.cols & 255);
    buffer[3] = (uchar)(binMask.cols  >> 8);

    int bytePos = 4;
    uchar bitPos = 0;
    uchar temp;
    uchar bit;

    for(int i=0;i<binMask.rows;i++)
    {
        for(int j=0;j<binMask.cols;j++)
        {
            if(binMask.at<uchar>(i,j)>0)
                bit = 1;
            else
                bit = 0;

            if(bitPos==0)
                temp = bit;
            else
                temp = (temp<<1) | bit;

            bitPos++;
            if(bitPos>=8)
            {
                buffer[bytePos] = temp;
                bitPos = 0;
                bytePos++;
            }
        }
    }

}
