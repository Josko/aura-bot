/*

   Copyright [2010] [Josko Nikolic]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

 */

#ifndef INCLUDES_H
#define INCLUDES_H

// standard integer sizes for 64 bit compatibility

#ifdef WIN32
#include "ms_stdint.h"
#else
#include <stdint.h>
#endif

// STL

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <vector>
#include <string>

using namespace std;

typedef vector<unsigned char> BYTEARRAY;
typedef pair<unsigned char, string> PIDPlayer;

// time

uint32_t GetTime( ); // seconds
uint32_t GetTicks( ); // milliseconds

#ifdef WIN32
#define MILLISLEEP( x ) Sleep( x )
#else
#define MILLISLEEP( x ) usleep( ( x ) * 1000 )
#endif

// network

#undef FD_SETSIZE
#define FD_SETSIZE 512

// output

void Print( const string &message ); // DCC
void Print2( const string &message ); // DCC and IRC

#endif
