// for game-proper functions, concerned only with a single game room

#include <limits.h>
#include "gameroom.c"
#include "network.c"
#include "logger.c"
#include "deck.c"

void delay() {
   int i;
   for (i = 0; i < INT_MAX / 1600; i++);
}

int player_turn(Gameroom gr, int p_index, Deck *dealer_deck, Deck *player_deck, Card *top) {
   char msg[SIZE], top_card_string[24], temp[SIZE];
   int n_chars, psock;
   psock = gr.psocks[p_index];
   delay();
   if (is_empty(*dealer_deck) && is_empty(*player_deck))
      return 0; // nothing to do
   // notify player that it is his turn, and send all relevant data
   print_deck(*player_deck, temp);
   sprintf(msg, "TURN:%s of %ss:%s;", value_string[top->value], suit_string[top->suit], temp);
   if (send_message(psock, msg, SERVER) == -1) {
      sprintf(msg, "ERROR: Error sending data to Player %d.\n", p_index + 1);
      send_to_all_except(gr, msg, p_index);
      return -1;
   }
/*   // tell everyone except current player to wait*/
/*   sprintf(msg, "waiting for Player %d's move...\n", p_index + 1);*/
/*   if (send_to_all_except(gr, msg, p_index) == -1) {*/
/*      sprintf(msg, "ERROR: Error sending data.\n");*/
/*      send_to_all(gr, msg);*/
/*      return -1;*/
/*   }*/
   // recv player's move
   if ((n_chars = recv_message(psock, &temp, SERVER)) == -1) { // error
      sprintf(msg, "ERROR: Error receiving data from Player %d.\n", p_index + 1);
      send_to_all_except(gr, msg, p_index);
      return -1;
   }
   temp[n_chars] = 0;
   if (n_chars == 0) { // player closed the connection
      sprintf(msg, "ERROR: Player %d disconnected.\n", p_index + 1);
      send_to_all_except(gr, msg, p_index);
      return -1;
   }
   else { // player has moved, send his turn data to everyone else
      Card card;
      if (strstr(temp, "THROW") != NULL) {
         int card_index;
         sscanf(temp, "%*s %d", &card_index);
         card = get_card_at(player_deck, card_index);
         *top = card;
         sprintf(msg, "Player %d threw '%s of %ss'.\n", p_index + 1, 
                  value_string[card.value], suit_string[card.suit]);
         send_to_all_except(gr, msg, p_index);
         sprintf(msg, "You have %d card%s remaining.\n", player_deck->size, (player_deck->size!=1)?"s":""); // add s if plural
         if (send_message(psock, msg, SERVER) == -1) {
            sprintf(msg, "ERROR: Error sending data to Player %d.\n", p_index + 1);
            send_to_all_except(gr, msg, p_index);
            return -1;
         }
         if (!strcmp(value_string[card.value], "9")) {
            int suit;
            sscanf(temp, "%*s %*d %d", &suit);
            suit--; // get index
            sprintf(msg, "The suit was set to %ss.\n", suit_string[suit]);
            delay();
            send_to_all(gr, msg);
            top->value = 0; // no specified value
            top->suit = suit; // change suit
         }
      }
      else if (strstr(temp, "DRAW") != NULL) {
         // return immediately if there are no more cards
         if (is_empty(*dealer_deck))
            return 0;
         card = draw_card_from(dealer_deck, player_deck);
         sprintf(msg, "Player %d drew a card.\n", p_index + 1);
         send_to_all_except(gr, msg, p_index);
         sprintf(msg, "You drew '%s of %ss'.\n", value_string[card.value], suit_string[card.suit]);
         if (send_message(psock, msg, SERVER) == -1) {
            sprintf(msg, "ERROR: Error sending data to Player %d.\n", p_index + 1);
            send_to_all_except(gr, msg, p_index);
            return -1;
         }
         // it's the same player's turn, he has to choose another card
         player_turn(gr, p_index, dealer_deck, player_deck, top);
      }
      return 1;
   }
}

// compute the winner, notify players
void end_game(Gameroom gr, Deck *player_deck) {
   int i, j, n_winners, least_card_count, winners[gr.n_players];
   char msg[SIZE];
   least_card_count = INT_MAX;
   n_winners = j = 0;
   log_message(LOG_FILE, "Ending a game..\n");
   for (i = 0; i < gr.n_players; i++) {
      if (player_deck[i].size < least_card_count) { // reset number of ties
         n_winners = 0;
         winners[n_winners++] = i;
         least_card_count = player_deck[i].size;
      }
      else if (player_deck[i].size == least_card_count) // tie
         winners[n_winners++] = i;
   }
   for (i = 0; i < gr.n_players; i++) {
      if (j < n_winners && i == winners[j]) { // winner
         j++;
         if (n_winners == 1) { // single winner
            if (send_message(gr.psocks[i], "WIN!", SERVER) == -1)
               continue;
         }
         else // multiple winners
            if (send_message(gr.psocks[i], "TIE!", SERVER) == -1)
               continue;
      }
      else // loser
         if (send_message(gr.psocks[i], "LOSE!", SERVER) == -1)
            continue;
   }
}

void free_game(Gameroom *gr, Deck dealer_deck, Deck *player_deck) {
   int i;
   free_deck(dealer_deck);
   for (i = 0; i < gr->n_players; i++)
      free_deck(player_deck[i]);
   free_room(gr);
}

void end_of_round(Gameroom gr, Deck dealer_deck, Deck *player_deck, int *round) {
   int i;
   char msg[SIZE];
   (*round)++;
   sprintf(msg, "\n---== ~End of Round %d~ ==---\n  Card counts:\n", *round);
   sprintf(msg + strlen(msg), "    Dealer  : %d card%s\n", 
      dealer_deck.size, (dealer_deck.size != 1) ? "s" : "");
   for (i = 0; i < gr.n_players; i++)
      sprintf(msg + strlen(msg), "    Player %d: %d card%s\n", 
         i + 1, player_deck[i].size, (player_deck[i].size != 1) ? "s" : "");
   strcat(msg, "-----------======-----------\n\n");
   delay();
   send_to_all(gr, msg);
   delay();
}

void start_game(Gameroom gr) {
   Deck dealer_deck, player_deck[gr.n_players];
   Card top;
   char msg[SIZE];
   int i, n_players, round = 0;
   n_players = gr.n_players;
   
   dealer_deck = new_deck();
   for (i = 0; i < n_players; i++)
      player_deck[i] = new_deck();
   // populate master deck
   initialize_dealer_deck(&dealer_deck);
   shuffle_deck(&dealer_deck);
   // draw cards from master deck and distribute them to players
   distribute_cards(&dealer_deck, player_deck, gr.n_players);
   // draw a card from master deck and set it face-up
   top = draw_card(&dealer_deck);
   
   for (i = 0; ; i++) {
      i = i % n_players;
      if (player_turn(gr, i, &dealer_deck, &(player_deck[i]), &top) == -1) {
         free_game(&gr, dealer_deck, player_deck);
         return;
      }
      if (is_empty(dealer_deck)) { // no more cards, game has to end
         end_of_round(gr, dealer_deck, player_deck, &round);
         end_game(gr, player_deck);
         free_game(&gr, dealer_deck, player_deck);
         return;
      }
      if (i + 1 == n_players)
         end_of_round(gr, dealer_deck, player_deck, &round);
   }
}

