/*

Copyright 2015 Yihenew Beyene

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef __SOFT_RADAR_USRP_DEVICE_HPP
#define __SOFT_RADAR_USRP_DEVICE_HPP

#include "typeDefs.hpp"
#include "params.hpp"

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mutex>
#include <thread>
#include <uhd/version.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/msg.hpp>

static const uint32 NUMBER_OF_CALIBRATED_LATTENCIES = 17;
static const uint32 CALIBRATED_DECIMATIONS[] =
             {4,8,12,16,20,24,28,32,36,40,44,48,50,64,128,256,512};
// Because of decimation (Rx) and interpolation (Tx) filters in the USRP's FPGA
// there is a timestamp (which is tagged by the controller in the USRP) offset
// between Tx and Rx samples. The following values were measured for a set of
// decimaiton factors given above
static const uint32 CALIBRATED_LATTENCIES_N200[] =
               {59,41,34,32,29,29,27,27,25,26,25,25,20,24,24,22,22};

static const double       USRP_ADC_RATE    = 100000000.0;  // 100 MHz
static const double       STARTUP_DELAY    = 0.4;  // seconds
static const unsigned int MAX_BURST_LENGTH = 32000;


class usrpDevice
{

public:


	/** constructor */
	usrpDevice(uint32 burstLen, bool recordData_, uint32 filsize, strg dir);

	/** destructor */
	~usrpDevice();

	void start();
	
	void stop() { streamingRequested = false; }
	
	/** set transmit frequency */
	void setTxFrequency(double freq);

	/** set receive frequency */
	void setRxFrequency(double freq);

	/** set transmit gain */
	void setTxGain(int dB);

	/** get transmit gain */
	int getTxGain(void) { return txGain; }

	/** set receive gain */
	void setRxGain(int dB);

	/** get receive gain */
	int getRxGain(void) { return rxGain; }

	/** Load new waveform */
	void loadBurst(complexF *newBurst);
	
	strg getConfig(void) { return configurationInfo; }
	

private:

	uhd::usrp::multi_usrp::sptr usrpDevSptr;
	uhd::tx_streamer::sptr      txStreamer; // transmit streamer
	uhd::rx_streamer::sptr      rxStreamer; // receiver streamer


	uhd::tx_metadata_t          txMetadata;          // Transmit Metadata 
	uhd::rx_metadata_t          rxMetadata;          // Receive Metadata 

	double                      txFreq;
	double                      rxFreq;
	
	int                         txGain;
	int                         rxGain;
	
	double                      sampleRate;

	vectorCFB                   txBuffs;
	vectorCFB                   rxBuffs;

	complexF *                  txburst;
	complexF *                  rxburst;
	complexF *                  nullburst;

	uint32                      burstLength;
	uint32                      fileSize;

	uint64                      txTimeOffset;
	uint64                      rxTimeOffset; 
	uint64                      txTimestamp;

	bool                        streamingRequested;
	bool                        restart;
	
	bool                        recordData;
	strg                        topDir;
	strg                        directory;
	char                        fileName[2048];
	bool                        burstLoaded;
	FILE *                      dataFile;
	FILE *                      logFile;
	std::mutex                  logFileLock;
	strg                        configurationInfo;

	std::thread *               transmitThread;       // transmitter thread 
	std::thread *               asyncMsgThread;       // async message handler
	std::thread *               receiveThread;        // receiver thread 


	void openDevice(strg w_ip, bool w_externalClock);
	void setDecimationFactor(uint32 w_decimation);
	void flushReceiveBuffer();
	void createFilesystem();

	void transmitter();
	void asyncMsgHandler();
	void receiver();
	void ethernalLoop();
	
};		
		
		
		
		
#endif

