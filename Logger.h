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
		Logger();

		~Logger() {
			log.flush();
			log.close();
		}

		static Logger* Instance() {
			if (instance == NULL)
				instance = new Logger();
			return instance;
		}

		static void Release() {
			if (instance != NULL)
				delete instance;
		}

		std::string GetLogName();

		Logger& operator << (const char* s) {
			log << s;
			return *this;
		}
		template<typename T> Logger& operator << (const T& t) {
			log << t;
			return *this;
		}

		template<typename T> Logger& Log(const T& t, LogLevel lvl = LOG_BASIC) {
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

			return *this;
		}

	private:
		static Logger* instance;
		std::string dir;
		std::string name;
		std::ofstream log;
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
		LOG(ds.str() << msg << std::endl;);   \
	} while (0)


#endif
