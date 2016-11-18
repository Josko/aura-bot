/*
Copyright (c) 2001, Mayukh Bose
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  

 * Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

 * Neither the name of Mayukh Bose nor the names of other
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include "csvparser.h"

using namespace std;

CSVParser::CSVParser() : m_nPos(0)
{
}

void CSVParser::SkipSpaces()
{
  while (m_nPos < m_sData.length() && m_sData[m_nPos] == ' ')
    ++m_nPos;
}

const CSVParser& CSVParser::operator<<(const string& sIn)
{
  this->m_sData = sIn;
  this->m_nPos  = 0;
  return *this;
}

const CSVParser& CSVParser::operator<<(const char* sIn)
{
  this->m_sData = sIn;
  this->m_nPos  = 0;
  return *this;
}

CSVParser& CSVParser::operator>>(int& nOut)
{
  string sTmp;
  SkipSpaces();
  while (m_nPos < m_sData.length() && m_sData[m_nPos] != ',')
    sTmp += m_sData[m_nPos++];

  ++m_nPos; // skip past comma
  nOut = atoi(sTmp.c_str());
  return *this;
}

CSVParser& CSVParser::operator>>(double& nOut)
{
  string sTmp = "";
  SkipSpaces();
  while (m_nPos < m_sData.length() && m_sData[m_nPos] != ',')
    sTmp += m_sData[m_nPos++];

  ++m_nPos; // skip past comma
  nOut = atof(sTmp.c_str());
  return *this;
}

CSVParser& CSVParser::operator>>(string& sOut)
{
  bool bQuotes = false;
  sOut         = string();
  SkipSpaces();

  // Jump past first " if necessary
  if (m_nPos < m_sData.length() && m_sData[m_nPos] == '"')
  {
    bQuotes = true;
    ++m_nPos;
  }

  while (m_nPos < m_sData.length())
  {
    if (!bQuotes && m_sData[m_nPos] == ',')
      break;
    if (bQuotes && m_sData[m_nPos] == '"')
    {
      if (m_nPos + 1 >= m_sData.length() - 1)
        break;
      if (m_sData[m_nPos + 1] == ',')
        break;
    }
    sOut += m_sData[m_nPos++];
  }

  // Jump past last " if necessary
  if (bQuotes && m_nPos < m_sData.length() && m_sData[m_nPos] == '"')
    ++m_nPos;

  // Jump past , if necessary
  if (m_nPos < m_sData.length() && m_sData[m_nPos] == ',')
    ++m_nPos;

  return *this;
}
