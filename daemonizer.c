#ifndef DAEMONIZER_H
#define DAEMONIZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "logger.c"

void create_lock(char *lock_file) {
   int lfp;
   char str[10];
   if (lock_file == NULL)
      return;
	if ((lfp = open(lock_file, O_RDWR | O_CREAT, 0640)) == -1) {
	   log_error(LOG_FILE, "open");
	   exit(EXIT_FAILURE);
	}
	if (lockf(lfp, F_TLOCK, 0) == -1) {
	   log_error(LOG_FILE, "lockf");
	   exit(EXIT_FAILURE);
	}
	sprintf(str, "%d\n", getpid());
	write(lfp, str, strlen(str)); // write pid to lockfile
}

void daemonize(char *running_dir, char *lock_file) {
   int i;
   pid_t pid;
	pid = fork();
	if (pid < 0) { // fork error
	   log_error(LOG_FILE, "fork");
	   exit(EXIT_FAILURE);
	}
	else if (pid > 0) // parent
	   exit(EXIT_SUCCESS); // exit to return terminal control to user
	
	// child (daemon) continues
	umask(027); // set newly created file permissions
	if (setsid() < 0) { // obtain a new process group
	   log_error(LOG_FILE, "setsid");
	   exit(EXIT_FAILURE);
	}
	chdir(running_dir); // change running directory
	close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);
	create_lock(lock_file);
}

#endif
