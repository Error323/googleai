#ifndef LOGGER_
#define LOGGER_

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>

enum LogLevel {
	LOG_BASIC,
	LOG_DEBUG,
};

class Logger {
	public:
		Logger(std::string);

		~Logger() {
			log.flush();
			log.close();
		}

		static Logger* Instance() {
			if (instance == NULL)
				instance = new Logger("");
			return instance;
		}

		static void Release() {
			if (instance != NULL)
				delete instance;
		}

		static void SetLogger(Logger* logger) {
			instance = logger;
		}

		std::string GetLogName();

		Logger& operator << (const char* s) {
			if (isGood) {
				log << s;
			}
			return *this;
		}
		template<typename T> Logger& operator << (const T& t) {
			if (isGood) {
				log << t;
			}
			return *this;
		}

		template<typename T> Logger& Log(const T& t, LogLevel lvl = LOG_BASIC) {
			if (isGood) {
				switch (lvl) {
					case LOG_BASIC: {
						log << t; log << std::endl;
					} break;
					case LOG_DEBUG: {
						/* TODO */
					} break;
					default: {
					} break;
				}
			}
			return *this;
		}

	private:
		static Logger* instance;
		std::string dir;
		std::string name;
		std::ofstream log;
		bool isGood;
};

#ifdef DEBUG
#define LOG(msg)                              \
	do {                                      \
		std::stringstream ss;                 \
		ss << msg;                            \
		Logger::Instance()->Log(ss.str());    \
	} while(0)

#define FORMAT_STRING "***ASSERTION FAILED***\n\n\tfile\t%s\n\tline\t%d\n\tfunc\t%s\n\tcond\t%s\n"

#define ASSERT(cond)                                                              \
	do {                                                                          \
		if ( !(cond) ) {                                                          \
			FATAL(FORMAT_STRING, __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
		}                                                                         \
	} while (0)

#define ASSERT_MSG(cond, msg)                                                     \
	do {                                                                          \
		if ( !(cond) ) {                                                          \
			FATAL(FORMAT_STRING, __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
			LOG(msg);                                                             \
		}                                                                         \
	} while (0)

#define BACKTRACE()                                          \
	do {                                                     \
		void* addresses[16];                                 \
		size_t size = backtrace(addresses, 16);              \
		char** symbols = backtrace_symbols(addresses, size); \
		for (size_t i = 0; i < size; i++)                    \
			LOG(symbols[i]);                                 \
		free(symbols);                                       \
	} while (0)

#define FATAL(...)                           \
	do {                                     \
		char buffer[2048];                   \
		snprintf(buffer, 2048, __VA_ARGS__); \
		LOG(buffer);                         \
		BACKTRACE();                         \
	} while (0)

#endif // DEBUG

#ifndef DEBUG // NOT DEBUG
#define LOG(msg)
#define LOGD(msg)
#define ASSERT(cond)
#define ASSERT_MSG(cond, msg)
#define ASSERTD(cond)
#define FATAL(...)
#endif // NOT DEBUG

#endif
