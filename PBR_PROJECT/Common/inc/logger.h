#ifndef __LOGGER_H
#define __LOGGER_H

#include "custom_macro.h"

#include <iostream>
#include <string>

class Logger {
	
	NoConstructor(Logger)

public:

	enum class Level {
		eMessage, eWarning, eError
	};

	static void Log(const std::string& content, Level level) {
		switch (level) {
		case Level::eMessage:
			Message(content);
			break;
		case Level::eWarning:
			Warning(content);
			break;
		case Level::eError:
			Error(content);
			break;
		}
	}

	static void Message(const std::string& content) {
		time_t now = time(0);
		tm* ltm    = localtime(&now);
		std::fprintf(stdout, "\x1b[32m[%2d:%2d:%2d]:%s\x1b[0m\n", 
			ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
			content.c_str());
	}

	static void Warning(const std::string& content) {
		time_t now = time(0);
		tm* ltm = localtime(&now);
		std::fprintf(stdout, "\x1b[33m[%2d:%2d:%2d]:%s\x1b[0m\n",
			ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
			content.c_str());
	}
	static void Error(const std::string& content) {
		time_t now = time(0);
		tm* ltm = localtime(&now);
		std::fprintf(stdout, "\x1b[31m[%2d:%2d:%2d]:%s\x1b[0m\n",
			ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
			content.c_str());
	}
};

#endif // !__LOGGER_H