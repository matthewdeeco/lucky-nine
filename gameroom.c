// functions specific to gamen_rooms (add, remove, is_full)

#ifndef GAMEROOM_H
#define GAMEROOM_H

#include <string.h>
#include <stdlib.h>
#include "network.c"

typedef struct gameroom {
   int *psocks; // sockets of connected players
   int waiting_for; // number of players waiting for
   int n_players; // number of players in room when it is full
} Gameroom;

typedef struct waitlist {
   Gameroom rooms[BACKLOG]; // list of gamen_rooms
   int n_rooms; // number of n_rooms in use
} Waitlist;

Waitlist new_waitlist() {
   Waitlist w;
   w.n_rooms = 0;
   return w;
}

int send_to_all(Gameroom gr, char *msg) {
   int i, success = 1;
   for (i = 0; i < gr.n_players; i++)
      if (send_message(gr.psocks[i], msg, SERVER) == -1)
         success = -1; // there was an error, but continue sending to the others
   return success;
}

int send_to_all_except(Gameroom gr, char *msg, int p_except) {
   int i, success = 1;
   for (i = 0; i < gr.n_players; i++) {
      if (i == p_except)
         continue; // do not send msg to p_except
      if (send_message(gr.psocks[i], msg, SERVER) == -1) 
         success = -1; // there was an error, but continue sending to the others
   }
   return success;
}

void free_room(Gameroom *gr) {
   int i;
   for (i = 0; i < gr->n_players; i++)
      close(gr->psocks[i]);
   free(gr->psocks);
}

int new_room(Waitlist *w, int creator, int player_count) {
   if (w->n_rooms == BACKLOG)
      return -1; // no more space
      
   int n_rooms = w->n_rooms;
   w->rooms[n_rooms].psocks = malloc(player_count * sizeof(int)); // allocate space for player_count players
   w->rooms[n_rooms].psocks[0] = creator; // psocks[0] is the creator
   w->rooms[n_rooms].n_players = player_count;
   // the creator is also a player
   w->rooms[n_rooms].waiting_for = player_count - 1;
   w->n_rooms++; // increase room number
   return n_rooms; // new room is in the last room
}

int is_full(Waitlist w, int room_no) {
   if (room_no < 0 || room_no >= w.n_rooms)
      return -1; // error
   else {
      Gameroom gr = w.rooms[room_no];
      if (gr.waiting_for == 0)
         return 1;
      else
         return 0;
   }
}

Gameroom remove_room(Waitlist *w, int room_no) {
   int i, n_rooms;
   Gameroom gr = w->rooms[room_no];
   n_rooms = --w->n_rooms;
   for (i = room_no; i < n_rooms; i++)
      w->rooms[i] = w->rooms[i + 1];
   return gr;
}

int add_to_room(Waitlist *w, int room_no, int client) {
   if (room_no < 0 || room_no >= w->n_rooms)
      return -1;
   else if (is_full(*w, room_no)) // cannot join a full room
      return -1;
   else {
      Gameroom *gr = &(w->rooms[room_no]);
      gr->psocks[gr->n_players - gr->waiting_for] = client;
      gr->waiting_for--;
      return 1;
   }
}

void print_rooms(const Waitlist w, char *buf) {
   int i, j, n_rooms = w.n_rooms, room_no;
   sprintf(buf, "\nNumber of rooms: %d\n", n_rooms);
   for (i = 0; i < n_rooms; i++) {
      Gameroom gr = w.rooms[i];
      sprintf(buf+strlen(buf), "Room %d: %d-player game, waiting for %d player%s\n", 
         i + 1, gr.n_players, gr.waiting_for, (gr.waiting_for!=1) ? "s":"");
/*      int n_connected = gr.n_players - gr.waiting_for;*/
/*      for (j = 0; j < n_connected; j++) {*/
/*         sprintf(buf+strlen(buf), "P%d: socket %d\n", j+1, gr.psocks[j]);*/
/*      }*/
   }
}
#endif
