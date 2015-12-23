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

#ifndef __SOFT_RADAR_PARAMS_HPP
#define __SOFT_RADAR_PARAMS_HPP

#include "typeDefs.hpp"
#include "rapidxml.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

static void gReadConfiguration();


typedef struct RADAR_PARAMS_
{
	RADAR_PARAMS_()
		: txFreq(0.0),
		  rxFreq(0.0),
		  txGain(1),
		  rxGain(5),
		  decimationFactor(100),
		  externalClock(false),
		  deviceIP("192.168.10.2")
		{
			gReadConfiguration();
		}

	double  txFreq;
	double  rxFreq;
	int     txGain;
	int     rxGain;
	uint32  decimationFactor;
	bool    externalClock;
	strg    deviceIP;

} RADAR_PARAMS;


static RADAR_PARAMS G_RADAR_PARAMS;


static void gReadConfiguration()
{


	// Read configuration file. If there is an error, report it and exit.
	std::ifstream fin("../config/radarConfig.xml");
	if (fin.fail())
	{
		std::cerr << "I/O error while reading configuration file." << std::endl;
	}

	fin.seekg(0, fin.end);
	size_t length = fin.tellg();
	fin.seekg(0, fin.beg);
	char* buffer = new char[length + 1];
	fin.read(buffer, length);
	buffer[length] = '\0';
	fin.close();

	rapidxml::xml_document<> doc;
	doc.parse<0>(buffer);

	rapidxml::xml_node<>*cur_node = doc.first_node("radarConfiguration")->first_node("txFreq");
	strg txFreq = cur_node->first_attribute("val")->value();
	
	cur_node = doc.first_node("radarConfiguration")->first_node("rxFreq");
	strg rxFreq = cur_node->first_attribute("val")->value();
	
	cur_node = doc.first_node("radarConfiguration")->first_node("txGain");
	strg txGain = cur_node->first_attribute("val")->value();
	
	cur_node = doc.first_node("radarConfiguration")->first_node("rxGain");
	strg rxGain = cur_node->first_attribute("val")->value();
	
	cur_node = doc.first_node("radarConfiguration")->first_node("decimation");
	strg decimation = cur_node->first_attribute("val")->value();
	
	cur_node = doc.first_node("radarConfiguration")->first_node("externalClock");
	strg externalClock = cur_node->first_attribute("val")->value();
	  
	cur_node = doc.first_node("radarConfiguration")->first_node("deviceIP");
	strg deviceIP = cur_node->first_attribute("val")->value();
	  
	std::istringstream param1(txFreq);
	param1 >> G_RADAR_PARAMS.txFreq; 
	
	std::istringstream param2(rxFreq);
	param2 >> G_RADAR_PARAMS.rxFreq; 

	std::istringstream param3(txGain);
	param3 >> G_RADAR_PARAMS.txGain; 
	
	std::istringstream param4(rxGain);
	param4 >> G_RADAR_PARAMS.rxGain; 
	
	std::istringstream param5(decimation);
	param5 >> G_RADAR_PARAMS.decimationFactor; 
	
	if( externalClock.compare("YES")  == 0 ||
	    externalClock.compare("yes")  == 0 ||
	    externalClock.compare("TRUE") == 0 ||
	    externalClock.compare("true") == 0 )
	{
		G_RADAR_PARAMS.externalClock = true;
	}
	else{
		G_RADAR_PARAMS.externalClock = false;
	}
	
	G_RADAR_PARAMS.deviceIP = deviceIP; 
	
}


#endif
