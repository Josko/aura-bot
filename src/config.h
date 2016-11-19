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

#ifndef AURA_CONFIG_H_
#define AURA_CONFIG_H_

#include <map>
#include <string>

//
// CConfig
//

class CConfig
{
private:
  std::map<std::string, std::string> m_CFG;

public:
  CConfig();
  ~CConfig();

  void Read(const std::string& file);
  bool Exists(const std::string& key);
  int32_t GetInt(const std::string& key, int32_t x);
  std::string GetString(const std::string& key, const std::string& x);
  void Set(const std::string& key, const std::string& x);
};

#endif // AURA_CONFIG_H_
