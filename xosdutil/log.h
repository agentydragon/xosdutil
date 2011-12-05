#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

void log_close();
void msg(const char* fmt, ...);
void die(const char* fmt, ...);

#endif
