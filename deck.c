#include <time.h>

#define N_SUITS  4
#define N_VALUES 13
#define DECKSIZE N_SUITS * N_VALUES

#define SUIT_LENGTH 9
#define VALUE_LENGTH 7
#define INITIAL_CARD_COUNT 9

static const char *suit_string[N_SUITS] = {"Spade", "Heart", "Clover", "Diamond"};
static const char *value_string[N_VALUES + 1] = {"Card", "Ace", "2", "3", "4", 
   "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};

typedef struct card {
   int suit;
   int value;
} Card;

typedef struct deck {
   Card *cards;
   int size;
} Deck;

// removes the last card in the deck and returns it
Card draw_card(Deck *d) {
   return d->cards[--(d->size)];
}

int is_empty(Deck d) {
   return (d.size <= 0);
}

// removes the card at index and returns it
Card get_card_at(Deck *d, int index) {
   int i;
   Card drawn_card;
   if (index < 0 || index > d->size)
      exit(EXIT_FAILURE); // invalid index
   drawn_card = d->cards[index];
   for (i = index; i < d->size; i++) {
      d->cards[i] = d->cards[i + 1];
   }
   d->size--;
   return drawn_card;
}

// draws a card from src and adds it to dest, then returns the card
Card draw_card_from(Deck *src, Deck *dest) {
   dest->cards[(dest->size)++] = draw_card(src);
   return dest->cards[dest->size - 1];
}

// distribute INITIAL_CARD_COUNT cards from m to each player in p
void distribute_cards(Deck *m, Deck *p, int n_players) {
   int i, j;
   for (i = 0; i < n_players; i++) {
      for (j = 0 ; j < INITIAL_CARD_COUNT; j++) {
         p[i].cards[(p[i].size)++] = draw_card(m);
      }
   }
}

Deck new_deck() {
   Deck d;
   d.size = 0;
   d.cards = malloc(DECKSIZE * sizeof(Card));
   return d;
}

void initialize_dealer_deck(Deck *d) {
   int value, suit, i = 0;
   d->size = DECKSIZE;
   for (value = 1; value <= N_VALUES; value++) {
      for (suit = 0; suit < N_SUITS; suit++) {
         d->cards[i].suit = suit;
         d->cards[i].value = value;
         i++;
      }
   }
}

void free_deck(Deck d) {
   free(d.cards);
}

// shuffling algorithm from http://benpfaff.org/writings/clc/shuffle.html
void shuffle_deck(Deck *d) {
   srand((unsigned)time(NULL));
   int i, j;
	for (i = 0; i < d->size; i++) {
	  j = i + rand() / (RAND_MAX / (DECKSIZE - i) + 1);
	  Card temp = d->cards[j];
	  d->cards[j] = d->cards[i];
	  d->cards[i] = temp;
	}
}

void print_deck(Deck d, char *buf) {
   int i, j;
   sprintf(buf, "%d card%s:\n", d.size, (d.size != 1) ? "s" : "");
   for (i = 0; i < d.size; i++) {
      Card c = d.cards[i];
      sprintf(buf + strlen(buf), "%2d.) %s of %ss\n", i+1, value_string[c.value], suit_string[c.suit]);
   }
}
