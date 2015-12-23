/*

Copyright 2015 Yihenew Beyene

This file is part of SoftRadar.

    SoftRadar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SoftRadar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SoftRadar.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "usrpDevice.hpp"
#include "typeDefs.hpp"
#include <time.h>
#include <cstdlib>
#include <thread>

#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#include <boost/program_options.hpp> 
#include <boost/format.hpp>

namespace po = boost::program_options;

// Globals
bool           STREAM_REQUEST  = false;
bool           TERMINATE       = false;
std::thread *  deviceLoop;
std::thread *  controlLoop;
complexF *     BURST;
uint32         SOURCE_PORT;
uint32         DESTINATION_PORT;
uint32         BURST_LEN = 0;
uint32         FILE_SIZE;
strg           DIRECTORY;


bool readFile(strg fname, strg & flag)
{

	std::ifstream fid;
	fid.open(fname, std::ifstream::in);
	if(not fid)
	{
		return false;
	}

	fid.seekg (0, fid.end);
	if(fid.tellg() != (int)(sizeof(complexF)*BURST_LEN) )
	{
		flag = "Waveform file size doesn't match burst size";
		return false;
	}

	fid.seekg (0, fid.beg);
	fid.read((char *) &BURST[0], sizeof(complexF)*BURST_LEN);
	

	if(fid)
	{
	  flag = "";
		return true;
	}

	flag = "Unknown error while reading from file";
	return false;
}


void startRadio(usrpDevice *dev)
{
	std::cout<<"Starting streamer ...\n";
	dev->start();
	std::cout<<"Device stopped\n";
}


void runController(usrpDevice *dev)
{


	// UDP socket
	
	int    socketFD;
	char   source[256];		// address of received packet
	char   dest[256];		// address of destination socket
	size_t addressSize;	    // size of address string
	
	/* create socket */
	socketFD = socket(AF_INET, SOCK_DGRAM,0);
	if (socketFD < 0) {perror("Socket:  Failed to create socket "); exit(0); }

	/* Set source */
	struct sockaddr_in address;
	addressSize = sizeof (address);
	bzero(&address, addressSize);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(SOURCE_PORT); /* convert host byte order to port byte order */
	if (bind(socketFD, (struct sockaddr*)&address, addressSize) < 0)
	{  
		perror("Socket: bind() failed "); 
		exit(0); 
	}

	/* Set destination */
	struct sockaddr_in *address2 = (struct sockaddr_in *)dest;
	addressSize = sizeof(*address2);
	address2->sin_family = AF_INET;
	address2->sin_addr.s_addr = INADDR_ANY;
	address2->sin_port = htons(DESTINATION_PORT);
		

	int MAX_PACKET_LENGTH = 1024;

	char buffer[MAX_PACKET_LENGTH];
	int msgLen = -1;
	socklen_t tmpLen;
	buffer[0] = '\0';
	char cmdcheck[4];
	char command[MAX_PACKET_LENGTH];
	char response[MAX_PACKET_LENGTH];
	
	while(not TERMINATE)
	{
		memset(buffer,  0, MAX_PACKET_LENGTH);
		
		// Listen for commands
		tmpLen = sizeof(source);
		msgLen = recvfrom(socketFD, (void*)buffer, MAX_PACKET_LENGTH, 0,
		                  (struct sockaddr*)source, &tmpLen);

		if(msgLen <= 0)
		{
			if ((msgLen==-1) && (errno!=EAGAIN)) 
			{
				perror("Socket::read() failed");
				exit(0);
			}
			else
			{
				continue;		
			}
		}
		
		
		// parse message
		sscanf(buffer,"%3s %s",cmdcheck,command);

		if (strcmp(cmdcheck,"CMD")!=0) 
		{
			std::cout<<"Unknown command \n";
			continue;
		}

		if (strcmp(command,"START")==0) 
		{
			STREAM_REQUEST = true;
		}
		else if (strcmp(command,"STOP")==0) 
		{
			STREAM_REQUEST = false;
			dev->stop();
		}
		else if (strcmp(command,"TERMINATE")==0) 
		{
			STREAM_REQUEST = false;
			TERMINATE      = true;
		}
		else if (strcmp(command,"GETSTATE")==0) 
		{
			// tell whether device is running or not
			int n = sprintf(response, "RSP STATE %s", (STREAM_REQUEST? "ACTIVE" : "IDLE"));
			if(n>0)
			{
				sendto(socketFD, response, n, 0,
							(struct sockaddr *)dest, addressSize);
			}
		}
		else if (strcmp(command,"GETCONFIG")==0) 
		{
			// send the device configuration
			strg config = "RSP CONFIG *" + dev->getConfig();
			sendto(socketFD, config.c_str(), config.length(), 0,
							(struct sockaddr *)dest, addressSize);
		}		
		else if (strcmp(command,"LOAD")==0) 
		{
			// load new burst
			strg resp;
			
			// parse file name
			sscanf(buffer,"%3s %*s %s",cmdcheck,command);
			
			strg fname(command);
			strg readflag;
			
			if(readFile(fname, readflag))			
			{
				dev->loadBurst(BURST);
				resp = "RSP BURSTFLAG SUCCESS " + readflag;	
			} 
			else
			{
				resp = "RSP BURSTFLAG FAILURE " + readflag;	
				std::cout<<"Failed to read file \n";	
			}
			sendto(socketFD, resp.c_str(), resp.length(), 0,
							(struct sockaddr *)dest, addressSize);
		}
		
	}
}


int main(int argc, char* argv[])
{
  
  strg filename;
  bool recordData = false;
  
	//setup the program options
	po::options_description desc("options");
	desc.add_options()
	("help", "help message")
	("burstlen", po::value<unsigned>(&BURST_LEN)->default_value(1000), "Burst size (1:32000) samples")
	("record", po::bool_switch(&recordData), "record received data")
	("wmfilename", po::value<std::string>(&filename)->default_value(""), "Waveform file name")
	("logfilesize", po::value<unsigned>(&FILE_SIZE)->default_value(1000000), "Log file size (samples)")
	("dir", po::value<std::string>(&DIRECTORY)->default_value("Data"), "Log data directory")
	("srcport", po::value<unsigned>(&SOURCE_PORT)->default_value(5700), "Port for sending commands")
	("destport", po::value<unsigned>(&DESTINATION_PORT)->default_value(5701), "Port for receiving replies")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
	std::cout << boost::format("softRadar %s") % desc << std::endl;
	return ~0;
	}
	
	if (SOURCE_PORT == DESTINATION_PORT){
	std::cout <<"source and destination ports should be different "<< std::endl;
	return ~0;
	
	}
	// Initialize burst buffer
	BURST  = new complexF[BURST_LEN];
	memset(BURST, 0, BURST_LEN*sizeof(complexF));
	
	
	// Initialize the device
	usrpDevice *dev = new usrpDevice(BURST_LEN, recordData, FILE_SIZE, DIRECTORY);
	
	// Load waveform file if provided
	if(not filename.empty() )
	{
	
	  strg readflag;
		if(readFile(filename, readflag))			
		{
			dev->loadBurst(BURST);
		}
		else
		{
			std::cout <<"Failed to load waveform file '"<< filename <<"'"<< std::endl;
			return ~0;	
		}
	}
	else
	{
		std::cout<<"Make sure to upload waveform file before streaming starts\n";
	}
	// Start the controller (infinite loop)
	controlLoop = new std::thread(runController, dev);
	
	while(not TERMINATE)
	{
	
		// Check if the device is requested to start streaming
		if(not STREAM_REQUEST)
		{
			sleep(1);
			continue;
		}
		
		// Start the streamer
		deviceLoop = new std::thread(startRadio, dev);
		deviceLoop->join();
		STREAM_REQUEST = false;
		
	}
	

  return 0;
}

