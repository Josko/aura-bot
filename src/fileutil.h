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

#ifndef AURA_FILEUTIL_H_
#define AURA_FILEUTIL_H_

#include "includes.h"
#include <vector>

#ifdef WIN32
bool FileExists(std::string file);
#else
bool FileExists(const std::string& file);
#endif

std::vector<std::string> FilesMatch(const std::string& path, const std::string& pattern);
std::string FileRead(const std::string& file, uint32_t start, uint32_t length);
std::string FileRead(const std::string& file);
bool FileWrite(const std::string& file, uint8_t* data, uint32_t length);

#endif // AURA_FILEUTIL_H_
