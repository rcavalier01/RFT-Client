//
// Created by Phillip Romig on 7/16/24.
//
//modified by raachel cavalier on 10/19
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <array>
#include <vector>
#include <chrono>

#include "timerC.h"
#include "unreliableTransport.h"
#include "logging.h"


//#define WINDOW_SIZE 10
int main(int argc, char* argv[]) {

    // Defaults
    uint16_t portNum(12345);
    std::string hostname("");
    std::string inputFilename("");
    int requiredArgumentCount(0);
    size_t windowSize = 10;
    int timerDur = 15;

    int opt;
    try {
        while ((opt = getopt(argc, argv, "f:h:p:d:w:t:")) != -1) {
            switch (opt) {
                case 'p':
                    portNum = std::stoi(optarg);
                    break;
                case 'h':
                    hostname = optarg;
		    requiredArgumentCount++;
                    break;
                case 'd':
                    LOG_LEVEL = std::stoi(optarg);
                    break;
                case 'f':
                    inputFilename = optarg;
		    requiredArgumentCount++;
                    break;
                case 'w':
                    windowSize = std::stoi(optarg);
                    break;
                case 't':
                    timerDur = std::stoi(optarg);
                    break;
                case '?':
                default:
                    std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level] [-w window_size] [-t timeout]" << std::endl;
                    break;
            }
        }
    } catch (std::exception &e) {
        std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level] [-w window_size] [-t timeout]" << std::endl;
        FATAL << "Invalid command line arguments: " << e.what() << ENDL;
        return(-1);
    }

    if (requiredArgumentCount != 2) {
        std::cout << "Usage: " << argv[0] << " -f filename -h hostname [-p port] [-d debug_level] [-w window_size] [-t timeout]" << std::endl;
	std::cerr << "hostname and filename are required." << std::endl;
	return(-1);
    }


    TRACE << "Command line arguments parsed." << ENDL;
    TRACE << "\tServername: " << hostname << ENDL;
    TRACE << "\tPort number: " << portNum << ENDL;
    TRACE << "\tDebug Level: " << LOG_LEVEL << ENDL;
    TRACE << "\tOutput file name: " << inputFilename << ENDL;
    TRACE << "\t Timout selected: " << timerDur << ENDL;
    TRACE << "\t Window Size: " << windowSize << ENDL;
    auto start = std::chrono::high_resolution_clock::now();
    int throughput = 0;
    // *********************************
    // * Open the input file
    // *********************************
    //open(inputfilename)
    std::fstream file;
    file.open(inputFilename, std::ios::in);
    if(file.is_open()){
        DEBUG << "File Opened" << ENDL;
    }else{
        DEBUG << "Open Failure" << ENDL;
    }
    try {

        // ***************************************************************
        // * Initialize your timer, window and the unreliableTransport etc.
        // **************************************************************
        unreliableTransportC client(hostname, portNum);
        timerC timer(timerDur);
        std::vector<datagramS> sndpkt(windowSize);
        int startSeq = 1;
        int nextSeq = 1;
        int bytesRead = 0;
        bool zeroRead = false;
        
        // ***************************************************************
        // * Send the file one datagram at a time until they have all been
        // * acknowledged
        // **************************************************************
        bool allSent(false);
        bool allAcked(false);
        while ((!allSent) || (!allAcked)) {

		// Is there space in the window? If so, read some data from the file and send it.
            if((nextSeq < startSeq + windowSize) && !zeroRead){
                file.read(sndpkt[nextSeq%windowSize].data, MAX_PAYLOAD_LENGTH);
                bytesRead = file.gcount();
                //payload len, seq, dont need ack, checksum
                sndpkt[nextSeq % windowSize].payloadLength = bytesRead;
                sndpkt[nextSeq% windowSize].seqNum = nextSeq;
                sndpkt[nextSeq % windowSize].checksum = computeChecksum(sndpkt[nextSeq % windowSize]);
                //fsm
                client.udt_send(sndpkt[nextSeq % windowSize]);
                throughput += sizeof(datagramS);
                if(startSeq == nextSeq){
                    timer.start();
                }
                nextSeq++;
                if(bytesRead == 0){
                    //finished reading file by payload increments
                    DEBUG << "SENDING END SIGNAL" << ENDL;
                    allSent = true;
                    zeroRead = true;
                    //will signal with a zero payload to close
                }
            }
            
            // Call udt_recieve() to see if there is an acknowledgment.  If there is, process it.
            datagramS rcvpkt{};
            ssize_t rcv = client.udt_receive(rcvpkt);
            //received and not corrupt
            if(rcv > 0 && validateChecksum(rcvpkt)){
                startSeq = rcvpkt.ackNum + 1;
                if(startSeq == nextSeq){
                    timer.stop();
                }else{
                    timer.start();
                }
            }
            // Check to see if the timer has expired.
                //if expired restart timer and resend all unacked
            if(timer.timeout()){
                DEBUG << "timeout triggered"<< ENDL;
                timer.start();
                for(int i = startSeq; i < nextSeq; i++){
                    client.udt_send(sndpkt[i % windowSize]);
                    throughput += sizeof(datagramS);
                }
            }
            if((startSeq == nextSeq) && allSent){
                //no unacked packets in window
               allAcked = true;
            }
        }

        // cleanup and close the file and network.
        file.close();
    } catch (std::exception &e) {
        FATAL<< "Error: " << e.what() << ENDL;
        exit(1);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto transferTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto size = static_cast<double>(std::filesystem::file_size(inputFilename));
    auto goodput = size / transferTime.count();
    auto throughput_val = throughput / transferTime.count();
    std::cout << "total transfer time: " << transferTime.count() << " in ms\n";
    std::cout << "file size was: " << size << " bytes\n";
    std::cout << "goodput is: " << goodput << " Bytes/ms\n";
    std::cout << "throughput is: " << throughput_val << " Bytes/ms \n";
    return 0;
}
