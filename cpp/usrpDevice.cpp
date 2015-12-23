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

#include "usrpDevice.hpp"
#include<iostream>
#include<string.h>
#include <time.h>
#include <assert.h>

static const uhd::stream_args_t stream_args("fc32"); //complex floats


void usrpDevice::openDevice(strg w_ip, bool w_externalClock)
{

	// Find UHD devices
	uhd::device_addr_t args("addr="+w_ip);
	//uhd::device_addr_t args("");
	uhd::device_addrs_t dev_addrs = uhd::device::find(args);
	if (dev_addrs.size() == 0) 
	{
		std::cout<<" usrpDevice: No UHD devices found with IP : "
		         <<w_ip<<std::endl;
		exit(0);
	}


	usrpDevSptr = uhd::usrp::multi_usrp::make(dev_addrs[0]);
	
	std::cout<<dev_addrs[0].to_pp_string()<<std::endl;

	// Initialize tx streamer
	txStreamer = usrpDevSptr->get_tx_stream(stream_args);
	// Initialize rx streamer
	rxStreamer = usrpDevSptr->get_rx_stream(stream_args);

	txBuffs.push_back(txburst);
	rxBuffs.push_back(rxburst);

	if(w_externalClock)
	{
		usrpDevSptr->set_clock_source("external");
	}
	else{
		usrpDevSptr->set_clock_source("internal");
	}

}


void usrpDevice::setTxFrequency(double freq)
{

	usrpDevSptr->set_tx_freq(freq);
	double actualTxFreq = usrpDevSptr->get_tx_freq();
	if(actualTxFreq != freq)
	{
		std::cout<<"usrpDevice: failed to set Tx Freq : "<<freq
		         <<". Actual tx Freq is : "<<actualTxFreq<<std::endl;
		// exit(-1);
	}
	txFreq = freq;

}

void usrpDevice::setRxFrequency(double freq)
{
	usrpDevSptr->set_rx_freq(freq);
	double actualRxFreq = usrpDevSptr->get_rx_freq();
	if(actualRxFreq != freq)
	{
		std::cout<<"usrpDevice: failed to set Rx Freq : "<<freq
		         <<". Actual rx Freq is : "<<actualRxFreq<<std::endl;
		// exit(-1);
	}
	rxFreq = freq;
}

void usrpDevice::setTxGain(int dB)
{
	usrpDevSptr->set_tx_gain(dB);
	int actualTxGain = usrpDevSptr->get_tx_gain();
	if(actualTxGain != dB)
	{
		std::cout<<"usrpDevice: failed to set Tx Gain : "<<dB
		         <<". Actual tx Gain is : "<<actualTxGain<<std::endl;
		// exit(-1);
	}
	txGain = dB;
}

void usrpDevice::setRxGain(int dB)
{
	usrpDevSptr->set_rx_gain(dB);
	int actualRxGain = usrpDevSptr->get_rx_gain();
	if(actualRxGain != dB)
	{
		std::cout<<"usrpDevice: failed to set Rx Gain : "<<dB
		         <<". Actual rx Gain is : "<<actualRxGain<<std::endl;
		// exit(-1);
	}
	rxGain = dB;
}


void usrpDevice::setDecimationFactor(uint32 w_decimation)
{
	if(w_decimation<4 || w_decimation > 512)
	{
		std::cout<<"usrpDevice: Invalid decimation factor : "<<w_decimation
		         <<". Valid values are [4,5,...,512] "<<std::endl;
		exit(-1);
	}

  // FIXME calibrated values may change on new UHD drivers versions and other
  // USRP families.
  
	uint64 lattency = CALIBRATED_LATTENCIES_N200[NUMBER_OF_CALIBRATED_LATTENCIES-1];	
	for(uint32 k=0; k<NUMBER_OF_CALIBRATED_LATTENCIES; k++)
	{
		if(w_decimation <= CALIBRATED_DECIMATIONS[k])
		{
			// Closest calibrated decimation factor
			lattency = CALIBRATED_LATTENCIES_N200[k];
			break;
		}
	}
	
	double smplRate = USRP_ADC_RATE / (double)w_decimation;
	usrpDevSptr->set_tx_rate(smplRate);
	usrpDevSptr->set_rx_rate(smplRate);
	double actualRate = usrpDevSptr->get_tx_rate();
	if(actualRate != smplRate)
	{
		std::cout<<"usrpDevice: failed to set sampling Rate : "<<smplRate
		         <<". Actual rate is : "<<actualRate<<std::endl;
		exit(-1);
	}
	sampleRate = smplRate;

	rxTimeOffset = (uint64)(STARTUP_DELAY*smplRate)+1;
	txTimeOffset = (uint64)(STARTUP_DELAY*smplRate) - lattency;
	assert(txTimeOffset < rxTimeOffset);
	txTimestamp = 0;
}


void usrpDevice::flushReceiveBuffer()
{
	uhd::rx_metadata_t md;
	vectorCFB buffs;
	size_t rx_spp = rxStreamer->get_max_num_samps();
	buffs.push_back(new complexF[rx_spp]);

	// Clear any old data from the ethernet buffer before starting new stream
	while(true) {
		rxStreamer->recv(buffs, rx_spp, md);
		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
		{
			break;
		}
	}
}


void usrpDevice::createFilesystem()
{
	
	// Creating top directory
	strg   dir = topDir;
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (buffer,80,"%F-%T",timeinfo);

	if(mkdir(dir.c_str(), 0777) != 0)
	{
		if(errno != EEXIST)
		{
			std::cout<<"usrpDevice: Failed to create directory : "
			         <<dir<<std::endl;
			exit(-1);
		}
	}
	
	dir += "/" + strg(buffer);
	if(mkdir(dir.c_str(), 0777) != 0)
	{
		std::cout<<"usrpDevice: Failed to create directory : "
		         <<dir<<std::endl;
		exit(-1);
	}
	
	directory = dir;
	
	
	
	// Creating log file
	
	sprintf(fileName, "%s/deviceLog.txt",directory.c_str());
	logFile  = fopen(fileName, "w");

	if(logFile == NULL)
	{
		std::cout<<"usrpDevice: failed to create file : "<<fileName<<std::endl;
		exit(-1);
	}
	
	// Writting configuration to log file
	
	logFileLock.lock();
	fprintf(logFile, "%s\n\n", configurationInfo.c_str());
    
	// Writting FLAGS
	fprintf(logFile,"FLAGS \n \
NBL : NEW BURST LOADED \n \
BNL : BURST NOT LOADED \n \
TBL : TRANSMIT BURST LOAST / LATE \n \
UTE : UNKNOWN TRANSMIT ERROR \n \
URE : UNKNOWN RECEIVE ERROR \n \
RBL : RECEIVE BURST LOST  \n \n ");
	
	fprintf(logFile,"\n################################################## \n\n");
	
	fflush(logFile);

	logFileLock.unlock();
	

}

usrpDevice::usrpDevice(uint32 burstLen, bool recordData_, uint32 filsize, strg dir)
{
	if(burstLen <= 0)
	{
		std::cout<<"usrpDevice: Invalid burstLength : "
		         <<burstLen<<std::endl;
		exit(-1);
	}
	else if(burstLen > MAX_BURST_LENGTH)
	{
		std::cout<<"usrpDevice: Too large burstLength : "
		         <<burstLen<<std::endl;
		exit(-1);
	}
	
	recordData = recordData_;

	if(filsize <= 0)
	{
		std::cout<<"usrpDevice: Invalid file size : "
		         <<filsize<<std::endl;
		exit(-1);
	}
	fileSize = filsize;
	
	if(fileSize < burstLen)
	{
		std::cout<<"usrpDevice: file size should be larger than burst size "
		         <<std::endl;
		exit(-1);
	}


	txburst       = new complexF[burstLen];
	rxburst       = new complexF[burstLen];
	nullburst     = new complexF[burstLen];
	memset(txburst,   0, burstLen * sizeof(complexF) );	
	memset(rxburst,   0, burstLen * sizeof(complexF) );	
	memset(nullburst, 0, burstLen * sizeof(complexF) );	
	burstLength = burstLen;
	
	gReadConfiguration();
	this->openDevice(G_RADAR_PARAMS.deviceIP, G_RADAR_PARAMS.externalClock);
	this->setTxFrequency(G_RADAR_PARAMS.txFreq);
	this->setRxFrequency(G_RADAR_PARAMS.rxFreq);
	this->setTxGain(G_RADAR_PARAMS.txGain);
	this->setRxGain(G_RADAR_PARAMS.rxGain);
	this->setDecimationFactor(G_RADAR_PARAMS.decimationFactor);
	this->flushReceiveBuffer();

	streamingRequested = false;
	restart            = false;
	burstLoaded        = false;
	dataFile           = NULL;
	logFile            = NULL;

	
	char tmp[1000];
	sprintf(tmp, "CONFIGURATION \n \
TX FREQ        :  %E\n \
RX FREQ        :  %E\n \
TX GAIN        :  %d\n \
RX GAIN        :  %d\n \
DECIMATION     :  %u\n \
EXTERNAL CLOCK :  %s\n \
DEVICE IP ADDR :  %s\n \
BURST LENGTH   :  %d", 
	G_RADAR_PARAMS.txFreq, \
	G_RADAR_PARAMS.rxFreq, \
	G_RADAR_PARAMS.txGain, \
	G_RADAR_PARAMS.rxGain, \
	G_RADAR_PARAMS.decimationFactor, \
	(G_RADAR_PARAMS.externalClock? "YES":"NO"), \
	G_RADAR_PARAMS.deviceIP.c_str(),\
	burstLen);
	
	configurationInfo = strg(tmp);
	
	
	topDir = dir;
           
}



usrpDevice::~usrpDevice()
{
	fclose(dataFile);
	logFileLock.lock();
	fclose(logFile);
	logFileLock.unlock();
}


void usrpDevice::start()
{

	if(streamingRequested)
	{
		return;
	}
	
	streamingRequested = true;
	restart = false;
	
	
	createFilesystem();

	// uhd::set_thread_priority_safe();

	uhd::stream_cmd_t cmd = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	usrpDevSptr->issue_stream_cmd(cmd);

	this->flushReceiveBuffer();

	usrpDevSptr->set_time_now(0.0);

	cmd = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
	cmd.stream_now = false;
	cmd.time_spec = STARTUP_DELAY/2;
	usrpDevSptr->issue_stream_cmd(cmd);
	
	ethernalLoop();
}


void usrpDevice::loadBurst(complexF *newBurst)
{
	memcpy(txburst, newBurst, burstLength * sizeof(complexF) );
		
	burstLoaded = true;
	
	if(logFile != NULL)
	{		
		logFileLock.lock();
		// this might fail if log file is not open.
		// log file is created when streaming starts.
		// Hence, any update to the waveform burst before
		// the actual transmission is not logged.
		fprintf(logFile,"NBL  %016llu\n", txTimestamp);
		fflush(logFile);
		logFileLock.unlock();
	}
}


void usrpDevice::ethernalLoop()
{

	while(1)
	{
		transmitThread = new std::thread(&usrpDevice::transmitter, this);
		
		if(recordData)
		{
		        receiveThread  = new std::thread(&usrpDevice::receiver, this);
		}
		asyncMsgThread = new std::thread(&usrpDevice::asyncMsgHandler, this);
		transmitThread->join();
		asyncMsgThread->join();
		
		if(recordData)
		{
		   receiveThread->join();
		}
		
		logFile = NULL;
		
		if(!restart)
		{
			return;
		}
		restart       = false;

		this->start();
	}

}


void usrpDevice::transmitter()
{

	txTimestamp               = 0;
	txMetadata.has_time_spec  = true;
	txMetadata.start_of_burst = false;
	txMetadata.end_of_burst   = false;
	
	
	if(not burstLoaded)
	{
		logFileLock.lock();
		fprintf(logFile,"BNL  %016llu\n", txTimestamp);
		fflush(logFile);
		logFileLock.unlock();
	}
		
	while(streamingRequested)
	{

		txMetadata.time_spec = \
		   uhd::time_spec_t::from_ticks(txTimestamp+txTimeOffset, sampleRate);
		txStreamer->send(txBuffs,burstLength,txMetadata,1);

		txTimestamp += burstLength;

	}
}


void usrpDevice::asyncMsgHandler()
{

	//setup variables and allocate buffer
	uhd::async_metadata_t async_md;
	uint64 timestamp;
	
	while (streamingRequested)
	{
		if (not txStreamer->recv_async_msg(async_md))
		{
			continue;
		}
		
		// packet is late
		if(async_md.has_time_spec)
		{
			timestamp = async_md.time_spec.to_ticks(sampleRate);				
		}
		else
		{
			timestamp = txTimestamp;
		}
			
		//handle the error codes
		switch(async_md.event_code)
		{
			case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
			case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
				break;
				
			
			case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
			case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
			
				// late packet
				logFileLock.lock();
				fprintf(logFile,"TBL  %016llu\n", timestamp);
				fflush(logFile);
				logFileLock.unlock();
			
				break;
				
			case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
			case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
			
				// packet loss
				logFileLock.lock();
				fprintf(logFile,"TBL  %016llu\n", timestamp);
				fflush(logFile);
				logFileLock.unlock();
				break;
			
			case uhd::async_metadata_t::EVENT_CODE_USER_PAYLOAD:
			default:
			
				// Unknown event
				logFileLock.lock();
				fprintf(logFile,"UTE  %016llu\n", timestamp);
				fflush(logFile);
				logFileLock.unlock();
				break;
		}
		
	}
	
}



void usrpDevice::receiver()
{

	int n;
	uint64 rxTimestamp          = 0;
	uint64 sampleCount          = 0;
	uint32 fileCount            = 0;
	uint64 lastdataTimestamp    = 0;
	uint64 currentdataTimestamp = 0;
	uint32 buffindex;
	uint32 samplesTowrite;
	uint32 paddedzeros      = 0;
	

	sprintf(fileName, "%s/%06u.dat",directory.c_str(), fileCount);
	dataFile = fopen(fileName, "w");
	
	if(dataFile == NULL)
	{
		std::cout<<"usrpDevice: failed to create file : "<<fileName<<std::endl;
		exit(-1);
	}
	
	while (streamingRequested)
	{
		n = rxStreamer->recv(rxBuffs, burstLength, rxMetadata, 1);
		rxTimestamp = rxMetadata.time_spec.to_ticks(sampleRate);

		if(  ((rxTimestamp+n) <= rxTimeOffset) || ( n <= 0) )
		{
			continue;
		}
		
		if(rxTimestamp <= rxTimeOffset)
		{
			buffindex = rxTimeOffset-rxTimestamp;
			samplesTowrite = rxTimestamp+n-rxTimeOffset;
			currentdataTimestamp = 1;
			
		}    
		else
		{
			buffindex = 0;
			samplesTowrite = n;
			currentdataTimestamp = rxTimestamp - rxTimeOffset + 1;
		}

		paddedzeros = currentdataTimestamp - lastdataTimestamp - 1;
		// Non-zero value indicate lost data due to overflow or device errors
		// FIXME Log this event
		if(paddedzeros != 0)
		{
		
			logFileLock.lock();
			fprintf(logFile,"RBL  %016llu  %016llu\n",
			        lastdataTimestamp,currentdataTimestamp);
			fflush(logFile);
			logFileLock.unlock();
			
			// padding zeros (for the lost data)
			if( (sampleCount+paddedzeros) <= fileSize)
			{
				fwrite(&nullburst[0], sizeof(complexF), 
				       paddedzeros, dataFile); 
				sampleCount += paddedzeros;
			}
			else
			{
				fwrite(&nullburst[0], sizeof(complexF), 
				       fileSize-sampleCount, dataFile);
				fclose(dataFile);
				
				fileCount++;			
				sprintf(fileName, "%s/%06u.dat",directory.c_str(), fileCount);
				dataFile = fopen(fileName, "w");
				sampleCount = paddedzeros + sampleCount - fileSize;
				fwrite(&nullburst[0], sizeof(complexF), sampleCount, dataFile);
			}
		}

		// Write actual received data
		if( (sampleCount+samplesTowrite) <= fileSize)
		{
			fwrite(&rxburst[buffindex], sizeof(complexF), 
			       samplesTowrite, dataFile);
			sampleCount += samplesTowrite;
		}
		else if (samplesTowrite > 0)
		{
			fwrite(&rxburst[buffindex], sizeof(complexF), 
			       fileSize-sampleCount, dataFile);
			fclose(dataFile);

			fileCount++;			
			sprintf(fileName, "%s/%06u.dat",directory.c_str(), fileCount);
			dataFile = fopen(fileName, "w");
			sampleCount = samplesTowrite + sampleCount - fileSize;
			fwrite(&rxburst[buffindex+fileSize-sampleCount], 
			       sizeof(complexF), sampleCount, dataFile);
		}
		
		
		lastdataTimestamp += paddedzeros + samplesTowrite;
	}
}


