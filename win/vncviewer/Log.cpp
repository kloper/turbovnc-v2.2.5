//  Copyright (C) 2010, 2015-2017 D. R. Commander. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
//  USA.

#include "stdhdrs.h"
#include "Log.h"


const int Log::ToDebug   =  1;
const int Log::ToFile    =  2;
const int Log::ToConsole =  4;
const int Log::UseStdio  =  8;

const static int LINE_BUFFER_SIZE = 1024;


Log::Log(int mode, int level, LPTSTR filename, bool append)
{
  hlogfile = NULL;
  m_todebug = false;
  m_toconsole = false;
  m_tofile = false;
  m_usestdio = false;
  SetMode(mode);
  if (mode & ToFile)
    SetFile(filename, append);
}


void Log::SetMode(int mode)
{
  if (mode & ToDebug)
    m_todebug = true;
  else
    m_todebug = false;

  if (mode & ToFile) {
    m_tofile = true;
  } else {
    CloseFile();
    m_tofile = false;
  }

  if (mode & UseStdio)
    m_usestdio = true;
  else
    m_usestdio = false;

  if (mode & ToConsole) {
    if (!m_toconsole && !m_usestdio)
      AllocConsole();
    m_toconsole = true;
  } else {
    m_toconsole = false;
  }
}


void Log::SetLevel(int level)
{
  m_level = level % 100;
  m_giiLevel = level / 100;
}


void Log::SetFile(LPTSTR filename, bool append)
{
  // if a log file is open, close it now.
  CloseFile();

  m_tofile  = true;

  CHAR new_file_name[MAX_PATH] = { 0 };
  CHAR drive[MAX_PATH] = { 0 };
  CHAR dir[MAX_PATH] = { 0 };
  CHAR fname[MAX_PATH] = { 0 };
  CHAR ext[MAX_PATH] = { 0 };
  int rc = _splitpath_s(
      filename,
      drive, sizeof(drive),
      dir, sizeof(dir),
      fname, sizeof(fname),
      ext, sizeof(ext));

  if (rc != 0) {
      strcpy_s(new_file_name, sizeof(new_file_name), filename);
  }
  else {
      DWORD pid = GetCurrentProcessId();
      snprintf(new_file_name, sizeof(new_file_name), "%s%s%s-%d%s", drive, dir, fname, pid,  ext);
  }

  // If filename is NULL or invalid we should throw an exception here

  hlogfile = CreateFile(new_file_name, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hlogfile == INVALID_HANDLE_VALUE) {
    // We should throw an exception here
    m_todebug = true;
    m_tofile = false;
    Print(0, "Error opening log file %s\n", filename);
  }
  if (append)
    SetFilePointer(hlogfile, 0, NULL, FILE_END);
  else
    SetEndOfFile(hlogfile);
}


// if a log file is open, close it now.
void Log::CloseFile()
{
  if (hlogfile != NULL) {
    CloseHandle(hlogfile);
    hlogfile = NULL;
  }
}


void Log::ReallyPrint(LPTSTR format, va_list ap)
{
  char line[LINE_BUFFER_SIZE];
  _vsntprintf_s(line, _countof(line), _TRUNCATE, format, ap);
  line[LINE_BUFFER_SIZE - 2] = (char)'\0';
  int len = (int)strlen(line);
  if (len > 0 && len <= LINE_BUFFER_SIZE - 2 && line[len - 1] == (char)'\n') {
    // Replace trailing '\n' with MS-DOS style end-of-line.
    line[len - 1] = (char)'\r';
    line[len]     = (char)'\n';
    line[len + 1] = (char)'\0';
  }

  if (m_todebug) OutputDebugString(line);

  if (m_toconsole) {
    if (m_usestdio)
      vprintf(format, ap);
    else {
      DWORD byteswritten;
      WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), line,
                   (DWORD)(strlen(line) * sizeof(char)), &byteswritten, NULL);
    }
  }

  if (m_tofile && (hlogfile != NULL)) {
    DWORD byteswritten;
    WriteFile(hlogfile, line, (DWORD)(strlen(line) * sizeof(char)),
              &byteswritten, NULL);
  }
}


Log::~Log()
{
  CloseFile();
}


Log theLog;
