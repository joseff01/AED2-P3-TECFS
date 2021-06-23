#ifndef TECFS_DISK_DISKNODE_H
#define TECFS_DISK_DISKNODE_H

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>

#include "../lib/tinyxml2.h"
#include "../lib/json.hpp"
#include "DataBlock.h"
#include "Metadata.h"

using namespace std;
using namespace tinyxml2;
using json = nlohmann::json;

class DiskNode {

private:
    int portNo;
    int sockfd;
    char buffer[1025];
    string libPath;
    void clientSetup();
    string receiveMsg();
    json receiveJson();
    void sendMsg(string message);
    void saveFile(json jsonMessage);
    void recoverBlock(json jsonMessage);
    void recoverFile(json jsonMessage);
public:
    explicit DiskNode(int diskNum);

};


#endif //TECFS_DISK_DISKNODE_H