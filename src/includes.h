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

#include <iostream>
#include <string>
#include <cstdint>
#include <chrono>

// time

inline int64_t GetTime()
{
  const std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch()).count();
}

inline int64_t GetTicks()
{
  const std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch()).count();
}

// output

inline void Print(const std::string& message) // outputs to console
{
  std::cout << message << std::endl;
}

inline void Print(const char* message)
{
  std::cout << message << std::endl;
}

void Print2(const std::string& message); // outputs to console and irc

#endif // AURA_INCLUDES_H_
