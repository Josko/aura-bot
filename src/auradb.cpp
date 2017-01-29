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

#include "auradb.h"
#include "aura.h"
#include "util.h"
#include "config.h"
#include "sqlite3.h"

#include <utility>
#include <algorithm>

using namespace std;

//
// CQSLITE3 (wrapper class)
//

CSQLITE3::CSQLITE3(const string& filename)
  : m_Ready(true)
{
  if (sqlite3_open_v2(filename.c_str(), (sqlite3**)&m_DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
    m_Ready = false;
}

CSQLITE3::~CSQLITE3()
{
  sqlite3_close((sqlite3*)m_DB);
}

//
// CAuraDB
//

CAuraDB::CAuraDB(CConfig* CFG)
  : FromAddStmt(nullptr),
    FromCheckStmt(nullptr),
    BanCheckStmt(nullptr),
    AdminCheckStmt(nullptr),
    RootAdminCheckStmt(nullptr),
    m_HasError(false)
{
  Print("[SQLITE3] version " + string(SQLITE_VERSION));
  m_File = CFG->GetString("db_sqlite3_file", "aura.dbs");

  Print("[SQLITE3] opening database [" + m_File + "]");
  m_DB = new CSQLITE3(m_File);

  if (!m_DB->GetReady())
  {
    // setting m_HasError to true indicates there's been a critical error and we want Aura to shutdown
    // this is okay here because we're in the constructor so we're not dropping any games or players

    Print(string("[SQLITE3] error opening database [" + m_File + "] - ") + m_DB->GetError());
    m_HasError = true;
    m_Error    = "error opening database";
    return;
  }

  // find the schema number so we can determine whether we need to upgrade or not

  string        SchemaNumber;
  sqlite3_stmt* Statement;
  m_DB->Prepare(R"(SELECT value FROM config WHERE name="schema_number")", (void**)&Statement);

  if (Statement)
  {
    int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
    {
      if (sqlite3_column_count(Statement) == 1)
        SchemaNumber = string((char*)sqlite3_column_text(Statement, 0));
      else
        Print("[SQLITE3] error getting schema number - row doesn't have 1 column");
    }
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error getting schema number - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error getting schema number - " + m_DB->GetError());

  if (SchemaNumber.empty())
  {
    // couldn't find the schema number

    Print("[SQLITE3] couldn't find schema number, create tables");

    // assume the database is empty
    // note to self: update the SchemaNumber and the database structure when making a new schema

    Print("[SQLITE3] assuming database is empty");

    if (m_DB->Exec(R"(CREATE TABLE admins ( id INTEGER PRIMARY KEY, name TEXT NOT NULL, server TEXT NOT NULL DEFAULT "" ))") != SQLITE_OK)
      Print("[SQLITE3] error creating admins table - " + m_DB->GetError());

    if (m_DB->Exec("CREATE TABLE bans ( id INTEGER PRIMARY KEY, server TEXT NOT NULL, name TEXT NOT NULL, date TEXT NOT NULL, admin TEXT NOT NULL, reason TEXT )") != SQLITE_OK)
      Print("[SQLITE3] error creating bans table - " + m_DB->GetError());

    if (m_DB->Exec("CREATE TABLE players ( id INTEGER PRIMARY KEY, name TEXT NOT NULL, games INTEGER, dotas INTEGER, loadingtime INTEGER, duration INTEGER, left INTEGER, wins INTEGER, losses INTEGER, kills INTEGER, deaths INTEGER, creepkills INTEGER, creepdenies INTEGER, assists INTEGER, neutralkills INTEGER, towerkills INTEGER, raxkills INTEGER, courierkills INTEGER )") != SQLITE_OK)
      Print("[SQLITE3] error creating players table - " + m_DB->GetError());

    if (m_DB->Exec("CREATE TABLE config ( name TEXT NOT NULL PRIMARY KEY, value TEXT NOT NULL )") != SQLITE_OK)
      Print("[SQLITE3] error creating config table - " + m_DB->GetError());

    m_DB->Prepare(R"(INSERT INTO config VALUES ( "schema_number", ? ))", (void**)&Statement);

    if (Statement)
    {
      sqlite3_bind_text(Statement, 1, "1", -1, SQLITE_TRANSIENT);

      const int32_t RC = m_DB->Step(Statement);

      if (RC == SQLITE_ERROR)
        Print("[SQLITE3] error inserting schema number [1] - " + m_DB->GetError());

      m_DB->Finalize(Statement);
    }
    else
      Print("[SQLITE3] prepare error inserting schema number [1] - " + m_DB->GetError());
  }
  else
    Print("[SQLITE3] found schema number [" + SchemaNumber + "]");

  if (m_DB->Exec("CREATE TEMPORARY TABLE iptocountry ( ip1 INTEGER NOT NULL, ip2 INTEGER NOT NULL, country TEXT NOT NULL, PRIMARY KEY ( ip1, ip2 ) )") != SQLITE_OK)
    Print("[SQLITE3] error creating temporary iptocountry table - " + m_DB->GetError());

  if (m_DB->Exec(R"(CREATE TEMPORARY TABLE rootadmins ( id INTEGER PRIMARY KEY, name TEXT NOT NULL, server TEXT NOT NULL DEFAULT "" ))") != SQLITE_OK)
    Print("[SQLITE3] error creating temporary rootadmins table - " + m_DB->GetError());
}

CAuraDB::~CAuraDB()
{
  Print("[SQLITE3] closing database [" + m_File + "]");

  if (FromAddStmt)
    m_DB->Finalize(FromAddStmt);

  if (BanCheckStmt)
    m_DB->Finalize(BanCheckStmt);

  if (FromCheckStmt)
    m_DB->Finalize(FromCheckStmt);

  if (AdminCheckStmt)
    m_DB->Finalize(AdminCheckStmt);

  if (RootAdminCheckStmt)
    m_DB->Finalize(RootAdminCheckStmt);

  delete m_DB;
}

uint32_t CAuraDB::AdminCount(const string& server)
{
  uint32_t      Count = 0;
  sqlite3_stmt* Statement;
  m_DB->Prepare("SELECT COUNT(*) FROM admins WHERE server=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
      Count = sqlite3_column_int(Statement, 0);
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error counting admins [" + server + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error counting admins [" + server + "] - " + m_DB->GetError());

  return Count;
}

bool CAuraDB::AdminCheck(const string& server, string user)
{
  bool IsAdmin = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  if (!AdminCheckStmt)
    m_DB->Prepare("SELECT * FROM admins WHERE server=? AND name=?", (void**)&AdminCheckStmt);

  if (AdminCheckStmt)
  {
    sqlite3_bind_text((sqlite3_stmt*)AdminCheckStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text((sqlite3_stmt*)AdminCheckStmt, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(AdminCheckStmt);

    // we're just checking to see if the query returned a row, we don't need to check the row data itself

    if (RC == SQLITE_ROW)
      IsAdmin = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking admin [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Reset(AdminCheckStmt);
  }
  else
    Print("[SQLITE3] prepare error checking admin [" + server + " : " + user + "] - " + m_DB->GetError());

  return IsAdmin;
}

bool CAuraDB::AdminCheck(string user)
{
  bool IsAdmin = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  sqlite3_stmt* Statement;
  m_DB->Prepare("SELECT * FROM admins WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    // we're just checking to see if the query returned a row, we don't need to check the row data itself

    if (RC == SQLITE_ROW)
      IsAdmin = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking admin [" + user + "] - " + m_DB->GetError());

    m_DB->Reset(Statement);
  }
  else
    Print("[SQLITE3] prepare error checking admin [" + user + "] - " + m_DB->GetError());

  return IsAdmin;
}

bool CAuraDB::RootAdminCheck(const string& server, string user)
{
  bool IsRoot = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  if (!RootAdminCheckStmt)
    m_DB->Prepare("SELECT * FROM rootadmins WHERE server=? AND name=?", (void**)&RootAdminCheckStmt);

  if (RootAdminCheckStmt)
  {
    sqlite3_bind_text((sqlite3_stmt*)RootAdminCheckStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text((sqlite3_stmt*)RootAdminCheckStmt, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step((sqlite3_stmt*)RootAdminCheckStmt);

    // we're just checking to see if the query returned a row, we don't need to check the row data itself

    if (RC == SQLITE_ROW)
      IsRoot = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking admin [" + user + "] - " + m_DB->GetError());

    m_DB->Reset((sqlite3_stmt*)RootAdminCheckStmt);
  }
  else
    Print("[SQLITE3] prepare error checking admin [" + user + "] - " + m_DB->GetError());

  return IsRoot;
}

bool CAuraDB::RootAdminCheck(string user)
{
  bool IsRoot = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  sqlite3_stmt* Statement;
  m_DB->Prepare("SELECT * FROM rootadmins WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    // we're just checking to see if the query returned a row, we don't need to check the row data itself

    if (RC == SQLITE_ROW)
      IsRoot = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking admin [" + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error checking admin [" + user + "] - " + m_DB->GetError());

  return IsRoot;
}

bool CAuraDB::AdminAdd(const string& server, string user)
{
  bool Success = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  sqlite3_stmt* Statement;
  m_DB->Prepare("INSERT INTO admins ( server, name ) VALUES ( ?, ? )", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error adding admin [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error adding admin [" + server + " : " + user + "] - " + m_DB->GetError());

  return Success;
}

bool CAuraDB::RootAdminAdd(const string& server, string user)
{
  bool Success = false;
  transform(begin(user), end(user), begin(user), ::tolower);

  sqlite3_stmt* Statement;
  m_DB->Prepare("INSERT INTO rootadmins ( server, name ) VALUES ( ?, ? )", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error adding root admin [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error adding root admin [" + server + " : " + user + "] - " + m_DB->GetError());

  return Success;
}

bool CAuraDB::AdminRemove(const string& server, string user)
{
  bool          Success = false;
  sqlite3_stmt* Statement;
  transform(begin(user), end(user), begin(user), ::tolower);
  m_DB->Prepare("DELETE FROM admins WHERE server=? AND name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error removing admin [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error removing admin [" + server + " : " + user + "] - " + m_DB->GetError());

  return Success;
}

uint32_t CAuraDB::BanCount(const string& server)
{
  uint32_t      Count = 0;
  sqlite3_stmt* Statement;
  m_DB->Prepare("SELECT COUNT(*) FROM bans WHERE server=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
      Count = sqlite3_column_int(Statement, 0);
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error counting bans [" + server + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error counting bans [" + server + "] - " + m_DB->GetError());

  return Count;
}

CDBBan* CAuraDB::BanCheck(const string& server, string user)
{
  CDBBan* Ban = nullptr;
  transform(begin(user), end(user), begin(user), ::tolower);

  if (!BanCheckStmt)
    m_DB->Prepare("SELECT name, date, admin, reason FROM bans WHERE server=? AND name=?", (void**)&BanCheckStmt);

  if (BanCheckStmt)
  {
    sqlite3_bind_text((sqlite3_stmt*)BanCheckStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text((sqlite3_stmt*)BanCheckStmt, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(BanCheckStmt);

    if (RC == SQLITE_ROW)
    {
      if (sqlite3_column_count((sqlite3_stmt*)BanCheckStmt) == 4)
      {
        string Name   = string((char*)sqlite3_column_text((sqlite3_stmt*)BanCheckStmt, 0));
        string Date   = string((char*)sqlite3_column_text((sqlite3_stmt*)BanCheckStmt, 1));
        string Admin  = string((char*)sqlite3_column_text((sqlite3_stmt*)BanCheckStmt, 2));
        string Reason = string((char*)sqlite3_column_text((sqlite3_stmt*)BanCheckStmt, 3));

        Ban = new CDBBan(server, Name, Date, Admin, Reason);
      }
      else
        Print("[SQLITE3] error checking ban [" + server + " : " + user + "] - row doesn't have 4 columns");
    }
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking ban [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Reset(BanCheckStmt);
  }
  else
    Print("[SQLITE3] prepare error checking ban [" + server + " : " + user + "] - " + m_DB->GetError());

  return Ban;
}

bool CAuraDB::BanAdd(const string& server, string user, const string& admin, const string& reason)
{
  bool          Success = false;
  sqlite3_stmt* Statement;
  transform(begin(user), end(user), begin(user), ::tolower);
  m_DB->Prepare("INSERT INTO bans ( server, name, date, admin, reason ) VALUES ( ?, ?, date('now'), ?, ? )", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 2, user.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 3, admin.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 4, reason.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error adding ban [" + server + " : " + user + " : " + admin + " : " + reason + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error adding ban [" + server + " : " + user + " : " + admin + " : " + reason + "] - " + m_DB->GetError());

  return Success;
}

bool CAuraDB::BanRemove(const string& server, string user)
{
  bool          Success = false;
  sqlite3_stmt* Statement;
  transform(begin(user), end(user), begin(user), ::tolower);
  m_DB->Prepare("DELETE FROM bans WHERE server=? AND name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, server.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Statement, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error removing ban [" + server + " : " + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error removing ban [" + server + " : " + user + "] - " + m_DB->GetError());

  return Success;
}

bool CAuraDB::BanRemove(string user)
{
  bool          Success = false;
  sqlite3_stmt* Statement;
  transform(begin(user), end(user), begin(user), ::tolower);
  m_DB->Prepare("DELETE FROM bans WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, user.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error removing ban [" + user + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error removing ban [" + user + "] - " + m_DB->GetError());

  return Success;
}

void CAuraDB::GamePlayerAdd(string name, uint64_t loadingtime, uint64_t duration, uint64_t left)
{
  sqlite3_stmt* Statement;
  transform(begin(name), end(name), begin(name), ::tolower);

  // check if entry exists

  int32_t  RC;
  uint32_t Games = 0;

  m_DB->Prepare("SELECT games, loadingtime, duration, left FROM players WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
    {
      Games += 1 + sqlite3_column_int(Statement, 0);
      loadingtime += sqlite3_column_int64(Statement, 1);
      duration += sqlite3_column_int64(Statement, 2);
      left += sqlite3_column_int64(Statement, 3);
    }

    m_DB->Finalize(Statement);
  }
  else
  {
    Print("[SQLITE3] prepare error adding gameplayer [" + name + "] - " + m_DB->GetError());
    return;
  }

  if (Games == 0)
  {
    // insert new entry

    m_DB->Prepare("INSERT INTO players ( name, games, loadingtime, duration, left ) VALUES ( ?, ?, ?, ?, ? )", (void**)&Statement);

    if (Statement == nullptr)
    {
      Print("[SQLITE3] prepare error inserting gameplayer [" + name + "] - " + m_DB->GetError());
      return;
    }

    sqlite3_bind_text(Statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(Statement, 2, 1);
    sqlite3_bind_int64(Statement, 3, loadingtime);
    sqlite3_bind_int64(Statement, 4, duration);
    sqlite3_bind_int64(Statement, 5, left);
  }
  else
  {
    // update existing entry

    m_DB->Prepare("UPDATE players SET games=?, loadingtime=?, duration=?, left=? WHERE name=?", (void**)&Statement);

    if (Statement == nullptr)
    {
      Print("[SQLITE3] prepare error updating gameplayer [" + name + "] - " + m_DB->GetError());
      return;
    }

    sqlite3_bind_int(Statement, 1, Games);
    sqlite3_bind_int64(Statement, 2, loadingtime);
    sqlite3_bind_int64(Statement, 3, duration);
    sqlite3_bind_int64(Statement, 4, left);
    sqlite3_bind_text(Statement, 5, name.c_str(), -1, SQLITE_TRANSIENT);
  }

  RC = m_DB->Step(Statement);

  if (RC != SQLITE_DONE)
    Print("[SQLITE3] error adding gameplayer [" + name + "] - " + m_DB->GetError());

  m_DB->Finalize(Statement);
}

CDBGamePlayerSummary* CAuraDB::GamePlayerSummaryCheck(string name)
{
  sqlite3_stmt*         Statement;
  CDBGamePlayerSummary* GamePlayerSummary = nullptr;
  transform(begin(name), end(name), begin(name), ::tolower);
  m_DB->Prepare("SELECT games, loadingtime, duration, left FROM players WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
    {
      if (sqlite3_column_count(Statement) == 4)
      {
        const uint32_t TotalGames  = sqlite3_column_int(Statement, 0);
        const uint64_t LoadingTime = sqlite3_column_int64(Statement, 1);
        const uint64_t Left        = sqlite3_column_int64(Statement, 2);
        const uint64_t Duration    = sqlite3_column_int64(Statement, 3);

        GamePlayerSummary = new CDBGamePlayerSummary(TotalGames, (double)LoadingTime / TotalGames / 1000, (double)Duration / Left * 100);
      }
      else
        Print("[SQLITE3] error checking gameplayersummary [" + name + "] - row doesn't have 4 columns");
    }
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking gameplayersummary [" + name + "] - " + m_DB->GetError());

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error checking gameplayersummary [" + name + "] - " + m_DB->GetError());

  return GamePlayerSummary;
}

void CAuraDB::DotAPlayerAdd(string name, uint32_t winner, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t neutralkills, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills)
{
  bool          Success = false;
  sqlite3_stmt* Statement;
  transform(begin(name), end(name), begin(name), ::tolower);
  m_DB->Prepare("SELECT dotas, wins, losses, kills, deaths, creepkills, creepdenies, assists, neutralkills, towerkills, raxkills, courierkills FROM players WHERE name=?", (void**)&Statement);

  int32_t  RC;
  uint32_t Dotas  = 1;
  uint32_t Wins   = 0;
  uint32_t Losses = 0;

  if (winner == 1)
    ++Wins;
  else if (winner == 2)
    ++Losses;

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
    {
      Dotas += sqlite3_column_int(Statement, 0);
      Wins += sqlite3_column_int(Statement, 1);
      Losses += sqlite3_column_int(Statement, 2);
      kills += sqlite3_column_int(Statement, 3);
      deaths += sqlite3_column_int(Statement, 4);
      creepkills += sqlite3_column_int(Statement, 5);
      creepdenies += sqlite3_column_int(Statement, 6);
      assists += sqlite3_column_int(Statement, 7);
      neutralkills += sqlite3_column_int(Statement, 8);
      towerkills += sqlite3_column_int(Statement, 9);
      raxkills += sqlite3_column_int(Statement, 10);
      courierkills += sqlite3_column_int(Statement, 11);

      Success = true;
    }

    m_DB->Finalize(Statement);
  }
  else
  {
    Print("[SQLITE3] prepare error adding dotaplayer [" + name + "] - " + m_DB->GetError());
    return;
  }

  // there must be a row already because we add one, if not present, in GamePlayerAdd( ) before the call to DotAPlayerAdd( )

  if (Success == false)
  {
    Print("[SQLITE3] error adding dotaplayer [" + name + "] - no existing row");
    return;
  }

  m_DB->Prepare("UPDATE players SET dotas=?, wins=?, losses=?, kills=?, deaths=?, creepkills=?, creepdenies=?, assists=?, neutralkills=?, towerkills=?, raxkills=?, courierkills=? WHERE name=?", (void**)&Statement);

  if (Statement == nullptr)
  {
    Print("[SQLITE3] prepare error updating dotalayer [" + name + "] - " + m_DB->GetError());
    return;
  }

  sqlite3_bind_int(Statement, 1, Dotas);
  sqlite3_bind_int(Statement, 2, Wins);
  sqlite3_bind_int(Statement, 3, Losses);
  sqlite3_bind_int(Statement, 4, kills);
  sqlite3_bind_int(Statement, 5, deaths);
  sqlite3_bind_int(Statement, 6, creepkills);
  sqlite3_bind_int(Statement, 7, creepdenies);
  sqlite3_bind_int(Statement, 8, assists);
  sqlite3_bind_int(Statement, 9, neutralkills);
  sqlite3_bind_int(Statement, 10, towerkills);
  sqlite3_bind_int(Statement, 11, raxkills);
  sqlite3_bind_int(Statement, 12, courierkills);
  sqlite3_bind_text(Statement, 13, name.c_str(), -1, SQLITE_TRANSIENT);

  RC = m_DB->Step(Statement);

  if (RC != SQLITE_DONE)
    Print("[SQLITE3] error adding dotaplayer [" + name + "] - " + m_DB->GetError());

  m_DB->Finalize(Statement);
}

CDBDotAPlayerSummary* CAuraDB::DotAPlayerSummaryCheck(string name)
{
  sqlite3_stmt*         Statement;
  CDBDotAPlayerSummary* DotAPlayerSummary = nullptr;
  transform(begin(name), end(name), begin(name), ::tolower);
  m_DB->Prepare("SELECT dotas, wins, losses, kills, deaths, creepkills, creepdenies, assists, neutralkills, towerkills, raxkills, courierkills FROM players WHERE name=?", (void**)&Statement);

  if (Statement)
  {
    sqlite3_bind_text(Statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    const int32_t RC = m_DB->Step(Statement);

    if (RC == SQLITE_ROW)
    {
      if (sqlite3_column_count(Statement) == 12)
      {
        const uint32_t TotalGames = sqlite3_column_int(Statement, 0);

        if (TotalGames != 0)
        {
          const uint32_t TotalWins         = sqlite3_column_int(Statement, 1);
          const uint32_t TotalLosses       = sqlite3_column_int(Statement, 2);
          const uint32_t TotalKills        = sqlite3_column_int(Statement, 3);
          const uint32_t TotalDeaths       = sqlite3_column_int(Statement, 4);
          const uint32_t TotalCreepKills   = sqlite3_column_int(Statement, 5);
          const uint32_t TotalCreepDenies  = sqlite3_column_int(Statement, 6);
          const uint32_t TotalAssists      = sqlite3_column_int(Statement, 7);
          const uint32_t TotalNeutralKills = sqlite3_column_int(Statement, 8);
          const uint32_t TotalTowerKills   = sqlite3_column_int(Statement, 9);
          const uint32_t TotalRaxKills     = sqlite3_column_int(Statement, 10);
          const uint32_t TotalCourierKills = sqlite3_column_int(Statement, 11);

          DotAPlayerSummary = new CDBDotAPlayerSummary(TotalGames, TotalWins, TotalLosses, TotalKills, TotalDeaths, TotalCreepKills, TotalCreepDenies, TotalAssists, TotalNeutralKills, TotalTowerKills, TotalRaxKills, TotalCourierKills);
        }
      }
      else
        Print("[SQLITE3] error checking dotaplayersummary [" + name + "] - row doesn't have 12 columns");
    }

    m_DB->Finalize(Statement);
  }
  else
    Print("[SQLITE3] prepare error checking dotaplayersummary [" + name + "] - " + m_DB->GetError());

  return DotAPlayerSummary;
}

string CAuraDB::FromCheck(uint32_t ip)
{
  // a big thank you to tjado for help with the iptocountry feature

  string From = "??";

  if (!FromCheckStmt)
    m_DB->Prepare("SELECT country FROM iptocountry WHERE ip1<=? AND ip2>=?", (void**)&FromCheckStmt);

  if (FromCheckStmt)
  {
    // we bind the ip as an int32_t64 because SQLite treats it as signed

    sqlite3_bind_int64((sqlite3_stmt*)FromCheckStmt, 1, ip);
    sqlite3_bind_int64((sqlite3_stmt*)FromCheckStmt, 2, ip);

    const int32_t RC = m_DB->Step(FromCheckStmt);

    if (RC == SQLITE_ROW)
    {
      if (sqlite3_column_count((sqlite3_stmt*)FromCheckStmt) == 1)
        From = string((char*)sqlite3_column_text((sqlite3_stmt*)FromCheckStmt, 0));
      else
        Print("[SQLITE3] error checking iptocountry [" + to_string(ip) + "] - row doesn't have 1 column");
    }
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error checking iptocountry [" + to_string(ip) + "] - " + m_DB->GetError());

    m_DB->Reset(FromCheckStmt);
  }
  else
    Print("[SQLITE3] prepare error checking iptocountry [" + to_string(ip) + "] - " + m_DB->GetError());

  return From;
}

bool CAuraDB::FromAdd(uint32_t ip1, uint32_t ip2, const string& country)
{
  // a big thank you to tjado for help with the iptocountry feature

  bool Success = false;

  if (!FromAddStmt)
    m_DB->Prepare("INSERT INTO iptocountry VALUES ( ?, ?, ? )", (void**)&FromAddStmt);

  if (FromAddStmt)
  {
    // we bind the ip as an int32_t64 because SQLite treats it as signed

    sqlite3_bind_int64((sqlite3_stmt*)FromAddStmt, 1, ip1);
    sqlite3_bind_int64((sqlite3_stmt*)FromAddStmt, 2, ip2);
    sqlite3_bind_text((sqlite3_stmt*)FromAddStmt, 3, country.c_str(), -1, SQLITE_TRANSIENT);

    int32_t RC = m_DB->Step(FromAddStmt);

    if (RC == SQLITE_DONE)
      Success = true;
    else if (RC == SQLITE_ERROR)
      Print("[SQLITE3] error adding iptocountry [" + to_string(ip1) + " : " + to_string(ip2) + " : " + country + "] - " + m_DB->GetError());

    m_DB->Reset(FromAddStmt);
  }
  else
    Print("[SQLITE3] prepare error adding iptocountry [" + to_string(ip1) + " : " + to_string(ip2) + " : " + country + "] - " + m_DB->GetError());

  return Success;
}

//
// CDBBan
//

CDBBan::CDBBan(string nServer, string nName, string nDate, string nAdmin, string nReason)
  : m_Server(std::move(nServer)),
    m_Name(std::move(nName)),
    m_Date(std::move(nDate)),
    m_Admin(std::move(nAdmin)),
    m_Reason(std::move(nReason))
{
}

CDBBan::~CDBBan() = default;

//
// CDBGamePlayer
//

CDBGamePlayer::CDBGamePlayer(string nName, uint64_t nLoadingTime, uint64_t nLeft, uint32_t nColour)
  : m_Name(std::move(nName)),
    m_LoadingTime(nLoadingTime),
    m_Left(nLeft),
    m_Colour(nColour)
{
}

CDBGamePlayer::~CDBGamePlayer() = default;

//
// CDBGamePlayerSummary
//

CDBGamePlayerSummary::CDBGamePlayerSummary(uint32_t nTotalGames, float nAvgLoadingTime, uint32_t nAvgLeftPercent)
  : m_TotalGames(nTotalGames),
    m_AvgLoadingTime(nAvgLoadingTime),
    m_AvgLeftPercent(nAvgLeftPercent)
{
}

CDBGamePlayerSummary::~CDBGamePlayerSummary() = default;

//
// CDBDotAPlayer
//

CDBDotAPlayer::CDBDotAPlayer()
  : m_Colour(0),
    m_NewColour(0),
    m_Kills(0),
    m_Deaths(0),
    m_CreepKills(0),
    m_CreepDenies(0),
    m_Assists(0),
    m_NeutralKills(0),
    m_TowerKills(0),
    m_RaxKills(0),
    m_CourierKills(0)
{
}

CDBDotAPlayer::CDBDotAPlayer(uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nNeutralKills, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills)
  : m_Kills(nKills),
    m_Deaths(nDeaths),
    m_CreepKills(nCreepKills),
    m_CreepDenies(nCreepDenies),
    m_Assists(nAssists),
    m_NeutralKills(nNeutralKills),
    m_TowerKills(nTowerKills),
    m_RaxKills(nRaxKills),
    m_CourierKills(nCourierKills)
{
}

CDBDotAPlayer::~CDBDotAPlayer() = default;

//
// CDBDotAPlayerSummary
//

CDBDotAPlayerSummary::CDBDotAPlayerSummary(uint32_t nTotalGames, uint32_t nTotalWins, uint32_t nTotalLosses, uint32_t nTotalKills, uint32_t nTotalDeaths, uint32_t nTotalCreepKills, uint32_t nTotalCreepDenies, uint32_t nTotalAssists, uint32_t nTotalNeutralKills, uint32_t nTotalTowerKills, uint32_t nTotalRaxKills, uint32_t nTotalCourierKills)
  : m_TotalGames(nTotalGames),
    m_TotalWins(nTotalWins),
    m_TotalLosses(nTotalLosses),
    m_TotalKills(nTotalKills),
    m_TotalDeaths(nTotalDeaths),
    m_TotalCreepKills(nTotalCreepKills),
    m_TotalCreepDenies(nTotalCreepDenies),
    m_TotalAssists(nTotalAssists),
    m_TotalNeutralKills(nTotalNeutralKills),
    m_TotalTowerKills(nTotalTowerKills),
    m_TotalRaxKills(nTotalRaxKills),
    m_TotalCourierKills(nTotalCourierKills)
{
}

CDBDotAPlayerSummary::~CDBDotAPlayerSummary() = default;
