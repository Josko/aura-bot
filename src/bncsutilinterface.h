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

#ifndef AURA_BNCSUTILINTERFACE_H_
#define AURA_BNCSUTILINTERFACE_H_

#include <cstdint>
#include <vector>
#include <string>

//
// CBNCSUtilInterface
//

class CBNCSUtilInterface
{
private:
  void*                m_NLS;
  std::vector<uint8_t> m_EXEVersion;        // set in HELP_SID_AUTH_CHECK
  std::vector<uint8_t> m_EXEVersionHash;    // set in HELP_SID_AUTH_CHECK
  std::vector<uint8_t> m_KeyInfoROC;        // set in HELP_SID_AUTH_CHECK
  std::vector<uint8_t> m_KeyInfoTFT;        // set in HELP_SID_AUTH_CHECK
  std::vector<uint8_t> m_ClientKey;         // set in HELP_SID_AUTH_ACCOUNTLOGON
  std::vector<uint8_t> m_M1;                // set in HELP_SID_AUTH_ACCOUNTLOGONPROOF
  std::vector<uint8_t> m_PvPGNPasswordHash; // set in HELP_PvPGNPasswordHash
  std::string          m_EXEInfo;           // set in HELP_SID_AUTH_CHECK

public:
  CBNCSUtilInterface(const std::string& userName, const std::string& userPassword);
  ~CBNCSUtilInterface();
  CBNCSUtilInterface(CBNCSUtilInterface&) = delete;

  inline std::vector<uint8_t> GetEXEVersion() const { return m_EXEVersion; }
  inline std::vector<uint8_t> GetEXEVersionHash() const { return m_EXEVersionHash; }
  inline std::string          GetEXEInfo() const { return m_EXEInfo; }
  inline std::vector<uint8_t> GetKeyInfoROC() const { return m_KeyInfoROC; }
  inline std::vector<uint8_t> GetKeyInfoTFT() const { return m_KeyInfoTFT; }
  inline std::vector<uint8_t> GetClientKey() const { return m_ClientKey; }
  inline std::vector<uint8_t> GetM1() const { return m_M1; }
  inline std::vector<uint8_t> GetPvPGNPasswordHash() const { return m_PvPGNPasswordHash; }

  inline void SetEXEVersion(const std::vector<uint8_t>& nEXEVersion) { m_EXEVersion = nEXEVersion; }
  inline void SetEXEVersionHash(const std::vector<uint8_t>& nEXEVersionHash) { m_EXEVersionHash = nEXEVersionHash; }

  void Reset(const std::string& userName, const std::string& userPassword);

  bool HELP_SID_AUTH_CHECK(const std::string& war3Path, const std::string& keyROC, const std::string& keyTFT, const std::string& valueStringFormula, const std::string& mpqFileName, const std::vector<uint8_t>& clientToken, const std::vector<uint8_t>& serverToken, const uint8_t war3Version);
  bool HELP_SID_AUTH_ACCOUNTLOGON();
  bool HELP_SID_AUTH_ACCOUNTLOGONPROOF(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& serverKey);
  bool HELP_PvPGNPasswordHash(const std::string& userPassword);

private:
  std::vector<uint8_t> CreateKeyInfo(const std::string& key, uint32_t clientToken, uint32_t serverToken);
};

#endif // AURA_BNCSUTILINTERFACE_H_
