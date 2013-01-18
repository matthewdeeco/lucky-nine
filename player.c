#include <stdlib.h>
#include <stdio.h>
#include "network.c"

enum {MAKE = 1, JOIN};
#define SUIT_LENGTH 9
#define VALUE_LENGTH 6

void send_request(int server, char *oper, int player_count) {
   char msg[SIZE];
   sprintf(msg, "%s %d", oper, player_count);
   if (send_message(server, msg, CLIENT) == -1)
      exit(EXIT_FAILURE);
}

char *input_operation() {
   int i;
   printf("(1) Create a new game room\n(2) Join a gameroom\n");
   while (1) {
      printf("Selection: ");
      scanf("%d", &i);
      if ((i >= 1) && (i <= 2))
         break;
      else
         printf("Invalid selection, try again.\n");
   }
   switch (i) {
      case MAKE: return "MAKE";
      case JOIN: return "JOIN";
   }
}

// Parse string output of display_rooms
int parse_room_count(char *room_list) {
   int rooms; // number of rooms in room_list
   sscanf(room_list, "%*[^:]: %d", &rooms);
   return rooms;
}

// Choose a room to join, then send choice to server
int join_a_room(int server) {
   char msg[SIZE];
   int n_chars;
   int room_no; // room the player wants to join
   int rooms; // number of rooms to choose from
   // display rooms
   if ((n_chars = recv_message(server, &msg, CLIENT)) <= 0)
      return -1;
   msg[n_chars] = 0;
   printf("%s", msg);
   // ensure that there are rooms to choose from
   rooms = parse_room_count(msg);
   if (rooms == 0) { // no rooms to join
      printf("There are no rooms to join, sorry.\n");
      return -1;
   }
   // choose a valid room
   while(1) {
      printf("Join room number: ");
      scanf("%d", &room_no);
      if (room_no <= 0 || room_no > rooms)
         printf("Invalid selection, try again.\n");
      else
         break;
   }
   // send room choice to server
   sprintf(msg, "%d", room_no);
   if (send_message(server, msg, CLIENT) == -1)
      return -1;
   else
      return 1;
}

int get_room_no(int server) {
   char msg[SIZE];
   int n_chars, room_no;
   if ((n_chars = recv_message(server, &msg, CLIENT)) <= 0)
      exit(EXIT_FAILURE);
   msg[n_chars] = 0;
   sscanf(msg, "%d", &room_no);
   return room_no;
}

int wait_until_full(int server, int room_no) {
   char msg[SIZE];
   int n_chars;
   printf("waiting in room #%d...", room_no);
   fflush(stdout); // force printing out
   if ((n_chars = recv_message(server, &msg, CLIENT)) <= 0)
      return 0;
   msg[n_chars] = 0;
   printf("done waiting!\n");
   return 1;
}

// parse top card's value and suit
void parse_top_card(char *top_card_string, char *top_card_value, char *top_card_suit) {
   sscanf(top_card_string, "%s %*s %s", top_card_value, top_card_suit);
}

// separates card list into individual cards
void parse_cards(char *deck, char values[][VALUE_LENGTH], char suits[][SUIT_LENGTH], int n_cards) {
   int i;
   strsep(&deck, "\n"); // go to next line
   for (i = 0; i < n_cards; i++) {
      sscanf(deck, "%*s %s %*s %s\n", values[i], suits[i]);
      strsep(&deck, "\n"); // go to next line
   }
}

int input_suit_request() {
   int choice;
   printf("What suit would you like to set?\n");
   printf(" 1.) Spades\n 2.) Hearts\n 3.) Clovers\n 4.) Diamonds\nSelection: ");
   while (1) {
      scanf("%d", &choice);
      if (choice < 1 || choice > 4)
         printf("Invalid selection.\n");
      else
         return choice;
   }
}

int my_turn(int server, char *top_card_string, char *deck_string) {
   int n_cards, move_index;
   sscanf(deck_string, "%d", &n_cards);
   
   char msg[SIZE];
   char top_card_value[VALUE_LENGTH], top_card_suit[SUIT_LENGTH];
   char values[n_cards][VALUE_LENGTH], suits[n_cards][SUIT_LENGTH];
   sleep(1);
   printf("\nThe top card is '%s'.\n", top_card_string);
   printf("Your deck contains %s\n", deck_string);
   parse_cards(deck_string, values, suits, n_cards);
   parse_top_card(top_card_string, top_card_value, top_card_suit);
   while(1) {
      printf("Enter the number of the card to throw, or 0 to draw a card: ");
      scanf("%d", &move_index);
      if (move_index < 0 || move_index > n_cards) {
         printf("Invalid selection.\n\n");
         continue;
      }
      else
         move_index--;
      if (move_index == -1)
         break; // pass
      else if (!strcmp(values[move_index], "9"))
         break; // nine is always acceptable
      else if (!strcmp(values[move_index], top_card_value))
         break; // value matches the top cards's value
      else if (!strcmp(suits[move_index], top_card_suit))
         break; // suit matches the top card's suit
      else
         printf("'%s of %s' matches neither suit nor value of '%s of %s'.\n\n", 
            values[move_index], suits[move_index], top_card_value, top_card_suit);
   }
   if (move_index == -1) // draw a card
      strcpy(msg, "DRAW");
   else if (!strcmp(values[move_index], "9")) {
      printf("You threw '%s of %s'.\n", values[move_index], suits[move_index]);
      printf("Nine is a lucky number!\n");
      sprintf(msg, "THROW %d %d", move_index, input_suit_request());
   }
   else {
      printf("You threw '%s of %s'.\n", values[move_index], suits[move_index]);
      sprintf(msg, "THROW %d", move_index);
   }
   if (send_message(server, msg, CLIENT) == -1)
      return -1;
}

void start_game(int server) {
   char msg[SIZE], *msgbody, top_card_string[24], oper[8], deck_string[4096];
   int n_chars;
   while(1) {
      if ((n_chars = recv_message(server, &msg, CLIENT)) <= 0)
         return; // error in receiving message from server
      msg[n_chars] = 0;
      if ((msgbody = strstr(msg, "TURN")) != NULL) { // my turn
         sscanf(msg, "%*[^:]:%23[^:]:%4095[^;];", top_card_string, deck_string);
         if (my_turn(server, top_card_string, deck_string) == -1) // error
            return;
      }
      else if ((msgbody = strstr(msg, "ERROR")) != NULL) { // game error
         printf("%sThe game cannot continue.\n", msgbody + 7);
         return;
      }
      else if (strstr(msg, "WIN") != NULL) { // winner, without a tie
         printf("\nGame over! There are no more cards in the deck.\n");
         printf("You had the least number of cards, so you won!\n");
         return;
      }
      else if (strstr(msg, "TIE") != NULL) { // winner, with a tie
         printf("\nGame over! There are no more cards in the deck.\n");
         printf("You won! ..but you tied with others..\n");
         return;
      }
      else if (strstr(msg, "LOSE") != NULL) { // loser
         printf("\nGame over! There are no more cards in the deck.\n");
         printf("You didn't have the least number of cards, so you lost...\n");
         return;
      }
      else { // received turn data
         printf("%s", msg);
      }
   }
}

int ask_for_player_count() {
   int player_count;
   while (1) {
      printf("How many players? (2-4) ");
      scanf("%d", &player_count);
      if (player_count < 2 || player_count > 4)
         printf("Invalid selection.\n");
      else
         return player_count;
   }
}

int ask_for_computer_player(int player_count) {
   int max_n_computers, selection;
   max_n_computers = player_count - 1;
   if (max_n_computers == 0) // cannot add any computers, don't ask
      return 0;
   else while (1) {
      printf("How many computer players do you want? (0-%d) ", max_n_computers);
      fflush(stdin);
      scanf("%d", &selection);
      if (selection < 0 || selection > max_n_computers)
         printf("Invalid selection.\n");
      else
         return selection;
   }
}

// execute new process(es), the computer player/s
void add_computer_players(char *hostname, int computer_player, int room_no) {
   char msg[SIZE];
   pid_t pid;
   int i;
   sprintf(msg, "%d", room_no);
   for (i = 0; i < computer_player; i++) {
      pid = fork();
      if (pid < 0) {
         perror("fork");
         exit(EXIT_FAILURE);
      }
      else if (pid == 0) {
         char *argv[4] = {"./computer", hostname, msg, NULL};
         if (execvp(argv[0], argv) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
         }
      }
      usleep(100);
   }
}


int main(int argc, char *argv[]) {
   char buf[SIZE], *oper, *hostname;
   int server, player_count, computer_player, room_no;
   if (argc < 2) {
      fprintf(stderr, "usage: client hostname\n");
      exit(EXIT_SUCCESS);
	}
	hostname = argv[1];
	if ((server = get_client_socket(hostname)) == -1) {
	   printf("failed to connect!\n");
	   exit(EXIT_FAILURE);
	}
	oper = input_operation();
	player_count = computer_player = 0;
   if (!strcmp(oper, "MAKE")) {
      player_count = ask_for_player_count();
      computer_player = ask_for_computer_player(player_count);
   }
   send_request(server, oper, player_count);
   if (!strcmp(oper, "JOIN")) {
      if (join_a_room(server) == -1) {
         close(server);
         exit(EXIT_FAILURE);
      }
   }
   room_no = get_room_no(server);
   add_computer_players(hostname, computer_player, room_no);
   if (wait_until_full(server, room_no)) // successfully completed a room
      start_game(server);
	close(server);
	printf("\n");
	exit(EXIT_SUCCESS);
}
