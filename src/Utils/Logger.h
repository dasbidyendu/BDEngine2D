#pragma once

#include "raylib.h"
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

enum LogLevel {
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_SUCCESS,
  LOG_LEVEL_RAYLIB
};

struct LogEntry {
  LogLevel level;
  std::string message;
  std::string timestamp;
  bool showInConsole;
};

class Logger {
public:
  static void Init() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_logFile.open("session_log.txt", std::ios::out | std::ios::trunc);
    if (m_logFile.is_open()) {
      AddLog(LOG_LEVEL_INFO, "Logger initialized. File: session_log.txt");
    }
  }

  static void Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
      m_logFile.close();
    }
  }

  static void AddLog(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogInternal(level, true, fmt, args);
    va_end(args);
  }

  static void AddLogEx(LogLevel level, bool showInConsole, const char *fmt,
                       ...) {
    va_list args;
    va_start(args, fmt);
    LogInternal(level, showInConsole, fmt, args);
    va_end(args);
  }

  static void RaylibLogCallback(int logLevel, const char *text, va_list args) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), text, args);

    LogLevel level = LOG_LEVEL_RAYLIB;
    if (logLevel == LOG_INFO)
      level = LOG_LEVEL_INFO;
    else if (logLevel == LOG_WARNING)
      level = LOG_LEVEL_WARNING;
    else if (logLevel == LOG_ERROR || logLevel == LOG_FATAL)
      level = LOG_LEVEL_ERROR;

    // By default, hide Raylib debug logs from the in-engine console to reduce
    // clutter. Only show them if they are warnings or errors.
    bool showInConsole =
        (level == LOG_LEVEL_WARNING || level == LOG_LEVEL_ERROR);
    AddLogEx(level, showInConsole, "[Raylib] %s", buf);
  }

  static void Clear() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_logs.clear();
  }

  static const std::vector<LogEntry> &GetLogs() { return m_logs; }

  static const char *GetLevelString(LogLevel level) {
    switch (level) {
    case LOG_LEVEL_INFO:
      return "INFO";
    case LOG_LEVEL_WARNING:
      return "WARNING";
    case LOG_LEVEL_ERROR:
      return "ERROR";
    case LOG_LEVEL_SUCCESS:
      return "SUCCESS";
    case LOG_LEVEL_RAYLIB:
      return "RAYLIB";
    default:
      return "UNKNOWN";
    }
  }

private:
  static inline std::vector<LogEntry> m_logs;
  static inline std::recursive_mutex m_mutex;
  static inline std::ofstream m_logFile;

  static void LogInternal(LogLevel level, bool showInConsole, const char *fmt,
                          va_list args) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    std::string message(buf);
    std::string timestamp = GetTimestamp();

    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_logs.push_back({level, message, timestamp, showInConsole});

    if (m_logs.size() > 5000) {
      m_logs.erase(m_logs.begin());
    }

    if (m_logFile.is_open()) {
      m_logFile << "[" << timestamp << "] [" << GetLevelString(level) << "] "
                << message << std::endl;
      m_logFile.flush();
    }

    // Also print to standard output for debug
    std::cout << "[" << timestamp << "] [" << GetLevelString(level) << "] "
              << message << std::endl;
  }

  static std::string GetTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm *ltm = std::localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", ltm);
    return std::string(buf);
  }
};
