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

#include "bncsutilinterface.h"
#include "aura.h"
#include "util.h"
#include "fileutil.h"

#include <bncsutil/bncsutil.h>

#include <bitset>
#include <algorithm>
#include <cmath>

using namespace std;

//
// CBNCSUtilInterface
//

CBNCSUtilInterface::CBNCSUtilInterface(const string& userName, const string& userPassword)
{
  m_NLS = new NLS(userName, userPassword);
}

CBNCSUtilInterface::~CBNCSUtilInterface()
{
  delete static_cast<NLS*>(m_NLS);
}

void CBNCSUtilInterface::Reset(const string& userName, const string& userPassword)
{
  delete static_cast<NLS*>(m_NLS);
  m_NLS = new NLS(userName, userPassword);
}

inline static std::string CaseInsensitiveFileExists(const std::string& path, std::string&& file)
{
  std::string mutated_file = file;
  const size_t NumberOfCombinations = std::pow(2, mutated_file.size());

  for (size_t perm = 0; perm < NumberOfCombinations; ++perm)
  {
    std::bitset<64> bs(perm);
    std::transform(mutated_file.begin(), mutated_file.end(), mutated_file.begin(), ::tolower);

    for (size_t index = 0; index < bs.size() && index < mutated_file.size(); ++index)
    {
      if (bs[index])
        mutated_file[index] = ::toupper(mutated_file[index]);
    }

    if (FileExists(path + mutated_file))
      return path + mutated_file;
  }

  return "";
}

bool CBNCSUtilInterface::HELP_SID_AUTH_CHECK(const string& war3Path, const string& keyROC, const string& keyTFT, const string& valueStringFormula, const string& mpqFileName, const std::vector<uint8_t>& clientToken, const std::vector<uint8_t>& serverToken, const uint8_t war3Version)
{
  const string FileWar3EXE = [&]() {
    if (war3Version >= 28)
      return CaseInsensitiveFileExists(war3Path, "Warcraft III.exe");
    else
      return CaseInsensitiveFileExists(war3Path, "war3.exe");
  }();
  const string FileStormDLL = CaseInsensitiveFileExists(war3Path, "storm.dll");
  const string FileGameDLL  = CaseInsensitiveFileExists(war3Path, "game.dll");

  if (!FileWar3EXE.empty() && (war3Version >= 29 || (!FileStormDLL.empty() && !FileGameDLL.empty())))
  {
    // TODO: check getExeInfo return value to ensure 1024 bytes was enough

    char     buf[1024];
    uint32_t EXEVersion;
    unsigned long EXEVersionHash;

    getExeInfo(FileWar3EXE.c_str(), buf, 1024, &EXEVersion, BNCSUTIL_PLATFORM_X86);

    if (war3Version >= 29)
    {
      const char* filesArray[] = {FileWar3EXE.c_str()};
      checkRevision(valueStringFormula.c_str(), filesArray, 1, extractMPQNumber(mpqFileName.c_str()), &EXEVersionHash);
    }
    else 
      checkRevisionFlat(valueStringFormula.c_str(), FileWar3EXE.c_str(), FileStormDLL.c_str(), FileGameDLL.c_str(), extractMPQNumber(mpqFileName.c_str()), &EXEVersionHash);

    m_EXEInfo        = buf;
    m_EXEVersion     = CreateByteArray(EXEVersion, false);
    m_EXEVersionHash = CreateByteArray(int64_t(EXEVersionHash), false);
    m_KeyInfoROC     = CreateKeyInfo(keyROC, ByteArrayToUInt32(clientToken, false), ByteArrayToUInt32(serverToken, false));
    m_KeyInfoTFT     = CreateKeyInfo(keyTFT, ByteArrayToUInt32(clientToken, false), ByteArrayToUInt32(serverToken, false));

    if (m_KeyInfoROC.size() == 36 && m_KeyInfoTFT.size() == 36)
      return true;
    else
    {
      if (m_KeyInfoROC.size() != 36)
        Print("[BNCSUI] unable to create ROC key info - invalid ROC key");

      if (m_KeyInfoTFT.size() != 36)
        Print("[BNCSUI] unable to create TFT key info - invalid TFT key");
    }
  }
  else
  {
    if (FileWar3EXE.empty())
      Print("[BNCSUI] unable to open War3EXE [" + FileWar3EXE + "]");

    if (FileStormDLL.empty() && war3Version < 29)
      Print("[BNCSUI] unable to open StormDLL [" + FileStormDLL + "]");
    if (FileGameDLL.empty() && war3Version < 29)
      Print("[BNCSUI] unable to open GameDLL [" + FileGameDLL + "]");
  }

  return false;
}

bool CBNCSUtilInterface::HELP_SID_AUTH_ACCOUNTLOGON()
{
  // set m_ClientKey

  char buf[32];
  (static_cast<NLS*>(m_NLS))->getPublicKey(buf);
  m_ClientKey = CreateByteArray(reinterpret_cast<uint8_t*>(buf), 32);
  return true;
}

bool CBNCSUtilInterface::HELP_SID_AUTH_ACCOUNTLOGONPROOF(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& serverKey)
{
  // set m_M1

  char buf[20];
  (static_cast<NLS*>(m_NLS))->getClientSessionKey(buf, string(begin(salt), end(salt)).c_str(), string(begin(serverKey), end(serverKey)).c_str());
  m_M1 = CreateByteArray(reinterpret_cast<uint8_t*>(buf), 20);
  return true;
}

bool CBNCSUtilInterface::HELP_PvPGNPasswordHash(const string& userPassword)
{
  // set m_PvPGNPasswordHash

  char buf[20];
  hashPassword(userPassword.c_str(), buf);
  m_PvPGNPasswordHash = CreateByteArray(reinterpret_cast<uint8_t*>(buf), 20);
  return true;
}

std::vector<uint8_t> CBNCSUtilInterface::CreateKeyInfo(const string& key, uint32_t clientToken, uint32_t serverToken)
{
  std::vector<uint8_t> KeyInfo;
  CDKeyDecoder         Decoder(key.c_str(), key.size());

  if (Decoder.isKeyValid())
  {
    const uint8_t Zeros[] = {0, 0, 0, 0};
    AppendByteArray(KeyInfo, CreateByteArray(static_cast<uint32_t>(key.size()), false));
    AppendByteArray(KeyInfo, CreateByteArray(Decoder.getProduct(), false));
    AppendByteArray(KeyInfo, CreateByteArray(Decoder.getVal1(), false));
    AppendByteArray(KeyInfo, CreateByteArray(Zeros, 4));
    size_t Length = Decoder.calculateHash(clientToken, serverToken);
    auto   buf    = new char[Length];
    Length        = Decoder.getHash(buf);
    AppendByteArray(KeyInfo, CreateByteArray(reinterpret_cast<uint8_t*>(buf), Length));
    delete[] buf;
  }

  return KeyInfo;
}
