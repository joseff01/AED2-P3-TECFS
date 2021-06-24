#include "ControllerNode.h"
#include <stdexcept>

ControllerNode::ControllerNode()
{
    storeFile("as", "hola mundo\nhi");
    serverSetup();
}

void ControllerNode::serverSetup()
{
    int option = 1;
    struct sockaddr_in serv_addr;

    // Initialize all sockets
    for (int &i : clientSocket)
    {
        i = 0;
    }

    std::cout << "Opening Socket..." << std::endl;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNo);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    std::cout << "Binding to port " << portNo << "..." << std::endl;
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Binding Failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 6) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int addrLen = sizeof(serv_addr);
    std::cout << "Waiting for DiskNodes..." << std::endl;

    int connectionCounter = 0;
    while (connectionCounter < 5)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        maxsd = sockfd;
        for (int i = 0; i < 7; ++i)
        {
            socketDescriptor = clientSocket[i];

            if (socketDescriptor > 0)
            {
                FD_SET(socketDescriptor, &readfds);
            }

            if (socketDescriptor > maxsd)
            {
                maxsd = socketDescriptor;
            }
        }

        activity = select(maxsd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            std::cout << "select error" << std::endl;
        }

        if (FD_ISSET(sockfd, &readfds))
        {
            if ((newsockfd = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t *)&addrLen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        }

        printf("New connection , socket fd is %d , ip is : %s , port : %d \n", newsockfd, inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

        if (send(newsockfd, "Server Message Received. Connection established!", strlen("Server Message Received. Connection established!"), 0) != strlen("Server Message Received. Connection established!"))
        {
            perror("send");
        }

        puts("Welcome message sent successfully");

        std::cout << receiveMsg(newsockfd) << std::endl;

        //add new client to list
        for (int i = 0; i < 7; i++)
        {
            //if position is empty
            if (clientSocket[i] == 0)
            {
                clientSocket[i] = newsockfd;
                printf("Adding to list of sockets as %d\n", i);
                connectionCounter++;
                break;
            }
        }

        for (int i = 0; i < 7; i++)
        {
            socketDescriptor = clientSocket[i];

            if (FD_ISSET(socketDescriptor, &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read(socketDescriptor, buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(socketDescriptor, (struct sockaddr *)&serv_addr, (socklen_t *)&addrLen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(socketDescriptor);
                    clientSocket[i] = 0;
                }
                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    send(socketDescriptor, buffer, strlen(buffer), 0);
                }
            }
        }
    }
}

std::string ControllerNode::receiveMsg(int receiveSockfd)
{
    memset(buffer, 0, 1025);
    int n = read(receiveSockfd, buffer, 1025);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }
    std::string receivedMsg = std::string(buffer);
    return receivedMsg;
}

void ControllerNode::sendMsg(int sendSockfd, std::string Msg)
{
    if (send(sendSockfd, Msg.c_str(), strlen(Msg.c_str()), 0) != strlen(Msg.c_str()))
    {
        perror("send");
    }
}

void ControllerNode::storeFile(std::string fileName, std::string fileContents)
{
    // Convert contents to binary
    std::string binContents = letters2bin(fileContents);

    List<std::string> stringDivs;

    // Dividir en 4
    int lengthFourth = binContents.length() / 4;
    for (int i = 0; i < 4; i++)
    {
        stringDivs.push_back(binContents.substr(i * lengthFourth, lengthFourth));
    }

    // Calcular string de paridad
    std::string parityStr;
    for (int i = 0; i < lengthFourth; i++)
    {
        int p = 0;
        for (int j = 0; j < 4; j++)
        {
            p = p ^ (stringDivs[j][i] - '0');
        }
        parityStr.append(std::to_string(p));
    }
    stringDivs.push_back(parityStr);

    // stringDivs.print();

    // Saber dónde va la paridad

    // mapa
    //  0  1  2  3   4
    // [A1 A2 A3 A4 Ap] 0
    // [B1 B2 B3 Bp B4] 1
    // [C1 C2 Cp C3 C4] 2
    // [D1 Dp D2 D3 D4] 3
    // [Ep E1 E2 E3 E4] 4
    // equation to determine p disk: 4 - block#

    // Pedir cantidad de archivos
    sendMsg(clientSocket[0], json({{"Case", FILE_AMOUNT}}).dump());
    json amountResult = json::parse(receiveMsg(clientSocket[0]));

    int fileAmount = amountResult["Amount"].get<int>(); // TODO: change to actual name of entry

    // Revisar si va a haber overflow y si sí, mix n' match los strings
    sendMsg(clientSocket[0], json({{"Case", METADATA_FROM_NUM},
                                   {"Num", fileAmount}})
                                 .dump());
    json metadataResult = json::parse(receiveMsg(clientSocket[0]));

    int lastFileSize = metadataResult["File size"].get<int>();
    int firstAvailableBit = metadataResult["Start bit"].get<int>() + lastFileSize; // TODO: change to actual names of entries

    int starterBlock = (int)firstAvailableBit / 512;
    int bitsRemainingInBlock = 512 - (firstAvailableBit % 512);
    int bitsRemainingForFile = lengthFourth;
    int currentBlock = starterBlock;
    List<std::string> finalStrings = {"", "", "", "", ""};

    while (bitsRemainingForFile > bitsRemainingInBlock)
    {
        int parityDisk = 4 - currentBlock;
        for (int disk = 0; disk < 5; disk++)
        {
            int listIndex;
            if (disk == parityDisk)
            {
                listIndex = 4;
            }
            else if (disk < parityDisk)
            {
                listIndex = disk;
            }
            else if (disk > parityDisk)
            {
                listIndex = disk - 1;
            }
            finalStrings[disk].append(stringDivs[listIndex].substr(lengthFourth - bitsRemainingForFile, bitsRemainingInBlock));
        }
        bitsRemainingForFile -= bitsRemainingInBlock;
        currentBlock++;
        bitsRemainingInBlock = 512;
    }

    // manejar el error de que se salga de la cantidad de memoria máxima, revisando si currentBlock se sale del rango
    if (currentBlock > 4)
    {
        std::cout << "ERROR: archivo intentando ser guardado excede la memoria máxima disponible en el sistema";
        throw std::overflow_error("Archivo intentando ser guardado excede la memoria máxima disponible en el sistema");
    }

    int parityDisk = 4 - currentBlock;
    for (int disk = 0; disk < 5; disk++)
    {
        int listIndex;
        if (disk == parityDisk)
        {
            listIndex = 4;
        }
        else if (disk < parityDisk)
        {
            listIndex = disk;
        }
        else if (disk > parityDisk)
        {
            listIndex = disk - 1;
        }
        finalStrings[disk].append(stringDivs[listIndex].substr(lengthFourth - bitsRemainingForFile, bitsRemainingForFile));
    }

    // TODO huffman encoding
    List<std::string> huffmanStrings = finalStrings;

    // mandar request final a los discos
    for (int disk = 0; disk < 5; disk++)
    {
        sendMsg(clientSocket[disk], json({{"Name", fileName},
                                          {"Contents", huffmanStrings[disk]}})
                                        .dump());
    }
}

std::string ControllerNode::letters2bin(std::string str)
{
    std::string binaryStr;

    for (int i = 0; i < str.length(); ++i)
    {
        std::bitset<8> charBits(str[i]);
        binaryStr.append(charBits.to_string());
    }

    return binaryStr;
}

std::string ControllerNode::bin2letters(std::string str)
{
    std::string charStr;

    for (int i = 0; i < str.length(); i += 8)
    {
        char c = static_cast<char>(std::stoi(str.substr(i, 8), nullptr, 2));
        charStr.append(1, c);
    }

    return charStr;
}