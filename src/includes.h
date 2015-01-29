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

#ifndef AURA_INCLUDES_H_
#define AURA_INCLUDES_H_

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
#include <cstdint>

typedef std::vector<uint8_t> BYTEARRAY;
typedef std::pair<uint8_t, std::string> PIDPlayer;

// time

uint32_t GetTime();   // seconds
uint32_t GetTicks();  // milliseconds

#ifdef WIN32
#define MILLISLEEP( x ) Sleep( x )
#else
#define MILLISLEEP( x ) usleep( ( x ) * 1000 )
#endif

// network

#undef FD_SETSIZE
#define FD_SETSIZE 512

// output

void Print(const std::string &message);    // outputs to console
void Print2(const std::string &message);   // outputs to console and irc

#endif  // AURA_INCLUDES_H_
