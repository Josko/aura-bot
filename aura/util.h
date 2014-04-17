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

#ifndef AURA_UTIL_H_
#define AURA_UTIL_H_

#include "includes.h"

inline std::string ToHexString(uint32_t i)
{
  std::string result;
  std::stringstream SS;
  SS << std::hex << i;
  SS >> result;
  return result;
}

inline BYTEARRAY CreateByteArray(const uint8_t *a, int32_t size)
{
  if (size < 1)
    return BYTEARRAY();

  return BYTEARRAY { a, a + size };
}

inline BYTEARRAY CreateByteArray(uint8_t c)
{
  return BYTEARRAY { c };
}

inline BYTEARRAY CreateByteArray(uint16_t i, bool reverse)
{
  if (!reverse)
    return BYTEARRAY { (uint8_t) i, (uint8_t)(i >> 8) };
  else
    return BYTEARRAY { (uint8_t)(i >> 8), (uint8_t) i };
}

inline BYTEARRAY CreateByteArray(uint32_t i, bool reverse)
{
  if (!reverse)
    return BYTEARRAY { (uint8_t) i, (uint8_t)(i >> 8), (uint8_t)(i >> 16), (uint8_t)(i >> 24) };
  else
    return BYTEARRAY { (uint8_t)(i >> 24), (uint8_t)(i >> 16), (uint8_t)(i >> 8), (uint8_t) i };
}

inline uint16_t ByteArrayToUInt16(const BYTEARRAY &b, bool reverse, uint32_t start = 0)
{
  if (b.size() < start + 2)
    return 0;

  if (!reverse)
    return (uint16_t)(b[start + 1] << 8 | b[start]);
  else
    return (uint16_t)(b[ start] << 8 | b[start + 1]);
}

inline uint32_t ByteArrayToUInt32(const BYTEARRAY &b, bool reverse, uint32_t start = 0)
{
  if (b.size() < start + 4)
    return 0;

  if (!reverse)
    return (uint32_t)(b[start + 3] << 24 | b[start + 2] << 16 | b[start + 1] << 8 | b[start]);
  else
    return (uint32_t)(b[start] << 24 | b[start + 1] << 16 | b[start + 2] << 8 | b[start + 3]);
}

inline std::string ByteArrayToDecString(const BYTEARRAY &b)
{
  if (b.empty())
    return std::string();

  std::string result = std::to_string(b[0]);

  for (auto i = begin(b) + 1; i != end(b); ++i)
    result += " " + std::to_string(*i);

  return result;
}

inline std::string ByteArrayToHexString(const BYTEARRAY &b)
{
  if (b.empty())
    return std::string();

  std::string result = ToHexString(b[0]);

  for (auto i = begin(b) + 1; i != end(b); ++i)
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
  b.insert(end(b), begin(append), end(append));
}

inline void AppendByteArrayFast(BYTEARRAY &b, const BYTEARRAY &append)
{
  b.insert(end(b), begin(append), end(append));
}

inline void AppendByteArray(BYTEARRAY &b, const uint8_t *a, int32_t size)
{
  AppendByteArray(b, CreateByteArray(a, size));
}

inline void AppendByteArray(BYTEARRAY &b, const std::string &append, bool terminator = true)
{
  // append the std::string plus a null terminator

  b.insert(end(b), begin(append), end(append));

  if (terminator)
    b.push_back(0);
}

inline void AppendByteArrayFast(BYTEARRAY &b, const std::string &append, bool terminator = true)
{
  // append the std::string plus a null terminator

  b.insert(end(b), begin(append), end(append));

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

inline BYTEARRAY ExtractCString(const BYTEARRAY &b, uint32_t start)
{
  // start searching the byte array at position 'start' for the first null value
  // if found, return the subarray from 'start' to the null value but not including the null value

  if (start < b.size())
  {
    for (uint32_t i = start; i < b.size(); ++i)
    {
      if (b[i] == 0)
        return BYTEARRAY(begin(b) + start, begin(b) + i);
    }

    // no null value found, return the rest of the byte array

    return BYTEARRAY(begin(b) + start, end(b));
  }

  return BYTEARRAY();
}

inline uint8_t ExtractHex(const BYTEARRAY &b, uint32_t start, bool reverse)
{
  // consider the byte array to contain a 2 character ASCII encoded hex value at b[start] and b[start + 1] e.g. "FF"
  // extract it as a single decoded byte

  if (start + 1 < b.size())
  {
    uint32_t c;
    std::string temp = std::string(begin(b) + start, begin(b) + start + 2);

    if (reverse)
      temp = std::string(temp.rend(), temp.rbegin());

    std::stringstream SS;
    SS << temp;
    SS >> std::hex >> c;
    return c;
  }

  return 0;
}

inline BYTEARRAY ExtractNumbers(const std::string &s, uint32_t count)
{
  // consider the std::string to contain a bytearray in dec-text form, e.g. "52 99 128 1"

  BYTEARRAY result;
  uint32_t c;
  std::stringstream SS;
  SS << s;

  for (uint32_t i = 0; i < count; ++i)
  {
    if (SS.eof())
      break;

    SS >> c;

    // TODO: if c > 255 handle the error instead of truncating

    result.push_back((uint8_t) c);
  }

  return result;
}

inline BYTEARRAY ExtractHexNumbers(std::string &s)
{
  // consider the std::string to contain a bytearray in hex-text form, e.g. "4e 17 b7 e6"

  BYTEARRAY result;
  uint32_t c;
  std::stringstream SS;
  SS << s;

  while (!SS.eof())
  {
    SS >> std::hex >> c;

    // TODO: if c > 255 handle the error instead of truncating

    result.push_back((uint8_t) c);
  }

  return result;
}

inline void AssignLength(BYTEARRAY &content)
{
  // insert the actual length of the content array int32_to bytes 3 and 4 (indices 2 and 3)

  const uint16_t Size = (uint16_t) content.size();

  content[2] = (uint8_t) Size;
  content[3] = (uint8_t)(Size >> 8);
}

inline std::string AddPathSeparator(const std::string &path)
{
  if (path.empty())
    return std::string();

#ifdef WIN32
  const char Separator = '\\';
#else
  const char Separator = '/';
#endif

  if (*(end(path) - 1) == Separator)
    return path;
  else
    return path + std::string(1, Separator);
}

inline BYTEARRAY EncodeStatString(BYTEARRAY &data)
{
  BYTEARRAY Result;
  uint8_t Mask = 1;

  for (uint32_t i = 0; i < data.size(); ++i)
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
      Result.insert(end(Result) - 1 - (i % 7), Mask);
      Mask = 1;
    }
  }

  return Result;
}

inline BYTEARRAY DecodeStatString(const BYTEARRAY &data)
{
  uint8_t Mask = 1;
  BYTEARRAY Result;

  for (uint32_t i = 0; i < data.size(); ++i)
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

inline std::vector<std::string> Tokenize(const std::string &s, const char delim)
{
  std::vector<std::string> Tokens;
  std::string Token;

  for (auto i = begin(s); i != end(s); ++i)
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

#endif  // AURA_UTIL_H_
