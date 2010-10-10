#ifndef LOGGER_
#define LOGGER_

#include <string>
#include <fstream>
#include <sstream>

struct LuaParser;

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

#define LOG(msg)                              \
	do {                                      \
		std::stringstream ss;                 \
		ss << msg;                            \
		Logger::Instance()->Log(ss.str());    \
	} while(0)

#define LOGD(msg)                             \
	do {                                      \
		std::stringstream ds;                 \
		for (int i = 0; i < depth; i++)       \
			ds << "\t";                       \
		LOG(ds.str() << msg);                 \
	} while (0)


#endif
