/* 
 * a computerized player
 * does not output anything to stdout/stderr
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "network.c"

#define SUIT_LENGTH 9
#define VALUE_LENGTH 6

void _send_request(int server, int room_no) {
   char msg[SIZE];
   sprintf(msg, "JOIN-NOW %d", room_no);
   if (send_message(server, msg, SERVER) == -1)
      exit(EXIT_FAILURE);
}

int _wait_until_full(int server) {
   char msg[SIZE];
   int n_chars;
   if ((n_chars = recv_message(server, &msg, SERVER)) <= 0)
      return 0;
   return 1;
}

// parse top card's value and suit
void _parse_top_card(char *top_card_string, char *top_card_value, char *top_card_suit) {
   sscanf(top_card_string, "%s %*s %s", top_card_value, top_card_suit);
}

// separates card list into individual cards
void _parse_cards(char *deck, char values[][VALUE_LENGTH], char suits[][SUIT_LENGTH], int n_cards) {
   int i;
   strsep(&deck, "\n"); // go to next line
   for (i = 0; i < n_cards; i++) {
      sscanf(deck, "%*s %s %*s %s\n", values[i], suits[i]);
      strsep(&deck, "\n"); // go to next line
   }
}

int _input_suit_request() {
   return (1 + rand() % 4);
}

int computer_turn(int server, char *top_card_string, char *deck_string) {
   int n_cards, move_index;
   sscanf(deck_string, "%d", &n_cards);
   
   char msg[SIZE];
   char top_card_value[VALUE_LENGTH], top_card_suit[SUIT_LENGTH];
   char values[n_cards][VALUE_LENGTH], suits[n_cards][SUIT_LENGTH];
   
   sleep(1);
   _parse_cards(deck_string, values, suits, n_cards);
   _parse_top_card(top_card_string, top_card_value, top_card_suit);
   for (move_index = 0; move_index < n_cards; move_index++) {
      if (!strcmp(values[move_index], "9"))
         break; // nine is always acceptable
      else if (!strcmp(values[move_index], top_card_value))
         break; // value matches the top cards's value
      else if (!strcmp(suits[move_index], top_card_suit))
         break; // suit matches the top card's suit
   }
   if (move_index == n_cards) // looped through all cards without a match
      move_index = -1;
   if (move_index == -1) // draw a card
      strcpy(msg, "DRAW");
   else if (!strcmp(values[move_index], "9"))
      sprintf(msg, "THROW %d %d", move_index, _input_suit_request());
   else
      sprintf(msg, "THROW %d", move_index);
   if (send_message(server, msg, SERVER) == -1)
      return -1;
}

void _start_game(int server) {
   char msg[SIZE], *msgbody, top_card_string[24], oper[8], deck_string[4096];
   int n_chars;
   while(1) {
      if ((n_chars = recv_message(server, &msg, SERVER)) <= 0)
         return; // error in receiving message from server
      msg[n_chars] = 0;
      if ((msgbody = strstr(msg, "TURN")) != NULL) { // my turn
         sscanf(msgbody, "%*[^:]:%23[^:]:%4095[^;];", top_card_string, deck_string);
         if (computer_turn(server, top_card_string, deck_string) == -1) // error
            return;
      }
      else if (strstr(msg, "ERROR") != NULL) // game error
         return;
      else if (strstr(msg, "WIN") != NULL) // winner, without a tie
         return;
      else if (strstr(msg, "TIE") != NULL) // winner, with a tie
         return;
      else if (strstr(msg, "LOSE") != NULL) // loser
         return;
   }
}

int main(int argc, char *argv[]) {
   char buf[SIZE];
   int server, room_no;
   srand((unsigned)time(NULL));
   if (argc < 3)
      exit(EXIT_SUCCESS);
	if ((server = get_client_socket(argv[1])) == -1)
	   exit(EXIT_FAILURE);
	sscanf(argv[2], "%d", &room_no);
   _send_request(server, room_no - 1);
   if (_wait_until_full(server)) // successfully completed a room
      _start_game(server);
	close(server);
	exit(EXIT_SUCCESS);
}
