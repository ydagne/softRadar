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

#ifndef __SOFT_RADAR_TYPE_DEFS_HPP
#define __SOFT_RADAR_TYPE_DEFS_HPP


#include<math.h>
#include<complex>
#include<vector>
#include<string.h>

static const float             PI = 3.141592653589793;

typedef unsigned long long int uint64;
typedef unsigned int            uint32;
typedef unsigned char           uint8;
typedef std::complex<float>     complexF;
typedef std::vector<bool>       boolV;
typedef std::vector<complexF *> vectorCFB; 
typedef std::string             strg;

#endif
