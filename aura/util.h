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

#ifndef UTIL_H
#define UTIL_H

#include "includes.h"
#include <sys/stat.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <string.h>
#endif

#ifdef __APPLE__
inline string ToString(size_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}
#endif

inline string ToString(uint64_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(uint32_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(uint16_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(int64_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(int32_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(int16_t i)
{
  string result;
  stringstream SS;
  SS << i;
  SS >> result;
  return result;
}

inline string ToString(float f, int digits)
{
  string result;
  stringstream SS;
  SS << std::fixed << std::setprecision(digits) << f;
  SS >> result;
  return result;
}

inline string ToString(const double d, int digits)
{
  string result;
  stringstream SS;
  SS << std::fixed << std::setprecision(digits) << d;
  SS >> result;
  return result;
}

inline string ToHexString(uint32_t i)
{
  string result;
  stringstream SS;
  SS << std::hex << i;
  SS >> result;
  return result;
}

// TODO: these ToXXX functions don't fail gracefully, they just return garbage (in the uint case usually just -1 casted to an unsigned type it looks like)

inline uint16_t ToUInt16(const string &s)
{
  uint16_t result;
  stringstream SS;
  SS << s;
  SS >> result;
  return result;
}

inline uint32_t ToUInt32(const string &s)
{
  uint32_t result;
  stringstream SS;
  SS << s;
  SS >> result;
  return result;
}

inline int16_t ToInt16(const string &s)
{
  int16_t result;
  stringstream SS;
  SS << s;
  SS >> result;
  return result;
}

inline int32_t ToInt32(const string &s)
{
  int32_t result;
  stringstream SS;
  SS << s;
  SS >> result;
  return result;
}

inline double ToDouble(const string &s)
{
  double result;
  stringstream SS;
  SS << s;
  SS >> result;
  return result;
}

inline BYTEARRAY CreateByteArray(const unsigned char *a, int size)
{
  if (size < 1)
    return BYTEARRAY();

  return BYTEARRAY { a, a + size };
}

inline BYTEARRAY CreateByteArray(unsigned char c)
{
  return BYTEARRAY { c };
}

inline BYTEARRAY CreateByteArray(uint16_t i, bool reverse)
{
  if (!reverse)
    return BYTEARRAY { (unsigned char) i, (unsigned char)(i >> 8) };
  else
    return BYTEARRAY { (unsigned char)(i >> 8), (unsigned char) i };
}

inline BYTEARRAY CreateByteArray(uint32_t i, bool reverse)
{
  if (!reverse)
    return BYTEARRAY { (unsigned char) i, (unsigned char)(i >> 8), (unsigned char)(i >> 16), (unsigned char)(i >> 24) };
  else
    return BYTEARRAY { (unsigned char)(i >> 24), (unsigned char)(i >> 16), (unsigned char)(i >> 8), (unsigned char) i };
}

inline uint16_t ByteArrayToUInt16(const BYTEARRAY &b, bool reverse, unsigned int start = 0)
{
  if (b.size() < start + 2)
    return 0;

  if (!reverse)
    return (uint16_t)(b[start + 1] << 8 | b[start]);
  else
    return (uint16_t)(b[ start] << 8 | b[start + 1]);
}

inline uint32_t ByteArrayToUInt32(const BYTEARRAY &b, bool reverse, unsigned int start = 0)
{
  if (b.size() < start + 4)
    return 0;

  if (!reverse)
    return (uint32_t)(b[start + 3] << 24 | b[start + 2] << 16 | b[start + 1] << 8 | b[start]);
  else
    return (uint32_t)(b[start] << 24 | b[start + 1] << 16 | b[start + 2] << 8 | b[start + 3]);
}

inline string ByteArrayToDecString(const BYTEARRAY &b)
{
  if (b.empty())
    return string();

  string result = ToString(b[0]);

  for (auto i = b.begin() + 1; i != b.end(); ++i)
    result += " " + ToString(*i);

  return result;
}

inline string ByteArrayToHexString(const BYTEARRAY &b)
{
  if (b.empty())
    return string();

  string result = ToHexString(b[0]);

  for (auto i = b.begin() + 1; i != b.end(); ++i)
  {
    if (*i < 16)
      result += " 0" + ToHexString(*i);
    else
      result += " " + ToHexString(*i);
  }

  return result;
}

inline void AppendByteArray(BYTEARRAY &b, const BYTEARRAY &append)
{
  b.insert(b.end(), append.begin(), append.end());
}

inline void AppendByteArrayFast(BYTEARRAY &b, const BYTEARRAY &append)
{
  b.insert(b.end(), append.begin(), append.end());
}

inline void AppendByteArray(BYTEARRAY &b, const unsigned char *a, int size)
{
  AppendByteArray(b, CreateByteArray(a, size));
}

inline void AppendByteArray(BYTEARRAY &b, const string &append, bool terminator = true)
{
  // append the string plus a null terminator

  b.insert(b.end(), append.begin(), append.end());

  if (terminator)
    b.push_back(0);
}

inline void AppendByteArrayFast(BYTEARRAY &b, const string &append, bool terminator = true)
{
  // append the string plus a null terminator

  b.insert(b.end(), append.begin(), append.end());

  if (terminator)
    b.push_back(0);
}

inline void AppendByteArray(BYTEARRAY &b, uint16_t i, bool reverse)
{
  AppendByteArray(b, CreateByteArray(i, reverse));
}

inline void AppendByteArray(BYTEARRAY &b, uint32_t i, bool reverse)
{
  AppendByteArray(b, CreateByteArray(i, reverse));
}

inline BYTEARRAY ExtractCString(const BYTEARRAY &b, unsigned int start)
{
  // start searching the byte array at position 'start' for the first null value
  // if found, return the subarray from 'start' to the null value but not including the null value

  if (start < b.size())
  {
    for (unsigned int i = start; i < b.size(); ++i)
    {
      if (b[i] == 0)
        return BYTEARRAY(b.begin() + start, b.begin() + i);
    }

    // no null value found, return the rest of the byte array

    return BYTEARRAY(b.begin() + start, b.end());
  }

  return BYTEARRAY();
}

inline unsigned char ExtractHex(const BYTEARRAY &b, unsigned int start, bool reverse)
{
  // consider the byte array to contain a 2 character ASCII encoded hex value at b[start] and b[start + 1] e.g. "FF"
  // extract it as a single decoded byte

  if (start + 1 < b.size())
  {
    unsigned int c;
    string temp = string(b.begin() + start, b.begin() + start + 2);

    if (reverse)
      temp = string(temp.rend(), temp.rbegin());

    stringstream SS;
    SS << temp;
    SS >> hex >> c;
    return c;
  }

  return 0;
}

inline BYTEARRAY ExtractNumbers(const string &s, unsigned int count)
{
  // consider the string to contain a bytearray in dec-text form, e.g. "52 99 128 1"

  BYTEARRAY result;
  unsigned int c;
  stringstream SS;
  SS << s;

  for (unsigned int i = 0; i < count; ++i)
  {
    if (SS.eof())
      break;

    SS >> c;

    // TODO: if c > 255 handle the error instead of truncating

    result.push_back((unsigned char) c);
  }

  return result;
}

inline BYTEARRAY ExtractHexNumbers(string &s)
{
  // consider the string to contain a bytearray in hex-text form, e.g. "4e 17 b7 e6"

  BYTEARRAY result;
  unsigned int c;
  stringstream SS;
  SS << s;

  while (!SS.eof())
  {
    SS >> hex >> c;

    // TODO: if c > 255 handle the error instead of truncating

    result.push_back((unsigned char) c);
  }

  return result;
}

inline void AssignLength(BYTEARRAY &content)
{
  // insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

  const uint16_t Size = (uint16_t) content.size();

  content[2] = (unsigned char) Size;
  content[3] = (unsigned char)(Size >> 8);
}

#ifdef WIN32
inline bool FileExists(string file)
{	
  if (*(file.end() - 1) == '\\')
	  file = file.substr(0, file.size() - 1);

  struct stat fileinfo;
  return (stat(file.c_str(), &fileinfo) == 0);
}
#else
inline bool FileExists(const string &file)
{
	struct stat fileinfo;
	return (stat(file.c_str(), &fileinfo) == 0);
}
#endif

inline vector<string> FilesMatch(const string &path, const string &pattern)
{
  vector<string> Files;

#ifdef WIN32
  WIN32_FIND_DATAA data;
  HANDLE handle = FindFirstFileA((path + "\\*").c_str(), &data);
  memset(&data, 0, sizeof(WIN32_FIND_DATAA));

  while (handle != INVALID_HANDLE_VALUE)
  {
	string Name = string(data.cFileName);
	transform(Name.begin(), Name.end(), Name.begin(), ::tolower);

	if (Name.find(pattern) != string::npos)
		Files.push_back(string(data.cFileName));		  

	if(FindNextFileA(handle, &data) == FALSE)
		break;
  }

  FindClose(handle);
#else
  DIR *dir = opendir(path.c_str());

  if (dir == nullptr)
    return Files;

  struct dirent *dp = nullptr;

  while ((dp = readdir(dir)) != nullptr)
  {
    string Name = string(dp->d_name);
    transform(Name.begin(), Name.end(), Name.begin(), ::tolower);

    if (Name.find(pattern) != string::npos && Name != "." && Name != "..")
      Files.push_back(string(dp->d_name));
  }

  closedir(dir);
#endif

  return Files;
}

inline string FileRead(const string &file, uint32_t start, uint32_t length)
{
  ifstream IS;
  IS.open(file.c_str(), ios::binary);

  if (IS.fail())
  {
    Print("[UTIL] warning - unable to read file part [" + file + "]");
    return string();
  }

  // get length of file

  IS.seekg(0, ios::end);
  uint32_t FileLength = IS.tellg();

  if (start > FileLength)
  {
    IS.close();
    return string();
  }

  IS.seekg(start, ios::beg);

  // read data

  char *Buffer = new char[length];
  IS.read(Buffer, length);
  string BufferString = string(Buffer, IS.gcount());
  IS.close();
  delete [] Buffer;
  return BufferString;
}

inline string FileRead(const string &file)
{
  ifstream IS;
  IS.open(file.c_str(), ios::binary);

  if (IS.fail())
  {
    Print("[UTIL] warning - unable to read file [" + file + "]");
    return string();
  }

  // get length of file

  IS.seekg(0, ios::end);
  uint32_t FileLength = IS.tellg();
  IS.seekg(0, ios::beg);

  // read data

  char *Buffer = new char[FileLength];
  IS.read(Buffer, FileLength);
  string BufferString = string(Buffer, IS.gcount());
  IS.close();
  delete [] Buffer;

  if (BufferString.size() == FileLength)
    return BufferString;
  else
    return string();
}

inline bool FileWrite(const string &file, unsigned char *data, uint32_t length)
{
  ofstream OS;
  OS.open(file.c_str(), ios::binary);

  if (OS.fail())
  {
    Print("[UTIL] warning - unable to write file [" + file + "]");
    return false;
  }

  // write data

  OS.write((const char *) data, length);
  OS.close();
  return true;
}

inline string AddPathSeperator(const string &path)
{
  if (path.empty())
    return string();

#ifdef WIN32
  char Seperator = '\\';
#else
  char Seperator = '/';
#endif

  if (*(path.end() - 1) == Seperator)
    return path;
  else
    return path + string(1, Seperator);
}

inline BYTEARRAY EncodeStatString(BYTEARRAY &data)
{
  BYTEARRAY Result;
  unsigned char Mask = 1;

  for (unsigned int i = 0; i < data.size(); ++i)
  {
    if ((data[i] % 2) == 0)
      Result.push_back(data[i] + 1);
    else
    {
      Result.push_back(data[i]);
      Mask |= 1 << ((i % 7) + 1);
    }

    if (i % 7 == 6 || i == data.size() - 1)
    {
      Result.insert(Result.end() - 1 - (i % 7), Mask);
      Mask = 1;
    }
  }

  return Result;
}

inline BYTEARRAY DecodeStatString(const BYTEARRAY &data)
{
  unsigned char Mask = 1;
  BYTEARRAY Result;

  for (unsigned int i = 0; i < data.size(); ++i)
  {
    if ((i % 8) == 0)
      Mask = data[i];
    else
    {
      if ((Mask & (1 << (i % 8))) == 0)
        Result.push_back(data[i] - 1);
      else
        Result.push_back(data[i]);
    }
  }

  return Result;
}

inline void Replace(string &Text, const string &Key, const string &Value)
{
  // don't allow any infinite loops

  if (Value.find(Key) != string::npos)
    return;

  string::size_type KeyStart = Text.find(Key);

  while (KeyStart != string::npos)
  {
    Text.replace(KeyStart, Key.size(), Value);
    KeyStart = Text.find(Key);
  }
}

inline vector<string> Tokenize(const string &s, const char delim)
{
  vector<string> Tokens;
  string Token;

  for (auto i = s.begin(); i != s.end(); ++i)
  {
    if (*i == delim)
    {
      if (Token.empty())
        continue;

      Tokens.push_back(Token);
      Token.clear();
    }
    else
      Token += *i;
  }

  if (!Token.empty())
    Tokens.push_back(Token);

  return Tokens;
}

#endif
