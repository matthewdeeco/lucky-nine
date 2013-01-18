// for connecting clients to gamerooms until they can start a game

#include <pthread.h>
#include "daemonizer.c"
#include "network.c"
#include "gameroom.c"
#include "gamemaster.c"

#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"lucky9.lock"

static Waitlist w;

void sigchld_handler(int s) {
   while(waitpid(-1, NULL, WNOHANG) > 0);
}

void kill_zombies() {
   struct sigaction sa;
   sa.sa_handler = sigchld_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;
   if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      log_error(LOG_FILE, "sigaction");
      exit(EXIT_FAILURE);
   }
}

int join_room(int client) {
   char msg[SIZE];
   int room_no, n_chars;
   // send list of rooms to client
   print_rooms(w, msg);
   if (send_message(client, msg, SERVER) == -1)
      return -1;
   // add client to his room of choice
   if ((n_chars = recv_message(client, &msg, SERVER)) <= 0)
      return -1; // error in communicating with client
   msg[n_chars] = 0;
   sscanf(msg, "%d", &room_no);
   room_no--; // index = room_no - 1
   add_to_room(&w, room_no, client);
   return room_no;
}

void *handle_connection(void *arg) {
   char msg[SIZE], oper[SIZE];
   int client, room_no, chars_sent, waiting_for, has_computer;
   client = *(int *)arg;
   if (recv_message(client, &msg, SERVER) == -1)
      return;
   sscanf(msg, "%s %d", oper, &waiting_for);
   log_message(LOG_FILE, "Received message: '");
   log_message(LOG_FILE, msg);
   log_message(LOG_FILE, "'\n");
   
   if (!strcmp(oper, "MAKE")) {
      room_no = new_room(&w, client, waiting_for);
      strcpy(msg, "No more space for new rooms!\n");
      if (room_no == -1) {
         log_message(LOG_FILE, msg);
         return;
      }
   }
   else if (!strcmp(oper, "JOIN")) {
      room_no = join_room(client);
   }
   if (!strcmp(oper, "JOIN-NOW")) { // a computer
      add_to_room(&w, waiting_for, client);
   }
   else { // send room number if not a computer
      sprintf(msg, "%d", room_no + 1);
      if (send_message(client, msg, SERVER) == -1) {
         close(client);
         return; 
      }
   }
   if (is_full(w, room_no)) {
      Gameroom gr = remove_room(&w, room_no);
      sleep(1);
      send_to_all(gr, "START");
      start_game(gr);
   }
}

void wait_for_connections() {
   int listener, client, i;
   pthread_t pth;
   
   listener = get_server_socket(NULL);
   w = new_waitlist();
   log_message(LOG_FILE, "waiting for connections...\n");
   while(1) { // main accept() loop
      if ((client = accept_connection(listener)) == -1) // error
         continue; // skip to next connection
      pthread_create(&pth, NULL, handle_connection, &client);
      pthread_detach(pth); // auto-free resources when thread terminates
      sleep(1);
   }
}

int main() {
	daemonize(RUNNING_DIR, LOCK_FILE);
	clear_log();
   kill_zombies();
	wait_for_connections();
   return 0;
}
