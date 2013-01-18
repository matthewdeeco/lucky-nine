#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>

#define LOG_FILE	"lucky9.log"

void clear_log() {
   FILE *trunc;
   if ((trunc = fopen(LOG_FILE, "w")) != NULL)
   	fclose(trunc);
}

void log_message(const char *filename, const char *message) {
   FILE *logfile;
	if ((logfile = fopen(filename, "a")) == NULL)
      return;
	fprintf(logfile, "%s", message);
   fclose(logfile);
}

void lg(const char *message) {
   log_message(LOG_FILE, message);
}

void log_error(const char *filename, const char *function) {
   int errsv = errno;
   FILE *logfile;
	if ((logfile = fopen(filename, "a")) == NULL)
      return;
	fprintf(logfile, "%s: %s\n", function, strerror(errsv));
   fclose(logfile);
}

#endif
