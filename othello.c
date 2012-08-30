/* othello.c
 *
 * An Othello/Reversi Game
 * Copyright (c) 1999 Eric Mulvaney
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#define EMPTY 0
#define BLACK 1
#define WHITE 2

#define OPPOSITE(x)     ((~x)&3)
#define VALID_CELL(x,y) ((x > -1) && (x < 8) && (y > -1) && (y < 8))

/* heuristic from Gnothello (Gnome'ish Othello, GPL'd) */ 
int weight[8][8] =
  { {512,   4, 128, 256, 256, 128,   4, 512},
    {  4,   2,   8,  16,  16,   8,   2,   4},
    {128,   8,  64,  32,  32,  64,   8, 128},
    {256,  16,  32,   2,   2,  32,  16, 256},
    {256,  16,  32,   2,   2,  32,  16, 256},
    {128,   8,  64,  32,  32,  64,   8, 128},
    {  4,   2,   8,  16,  16,   8,   2,   4},
    {512,   4, 128, 256, 256, 128,   4, 512}
  };

int board[8][8];

/* stacked moves (gathered not executed) */
int moves_x[1830]; /* the 'x' corrdinate of the [nth] move */
int moves_y[1830]; /* the 'y'... */
int moves_next = 0; /* the number of pairs (moves_x, moves_y) */
int moves_stack_block[60]; /* the number of pairs in current block */
int moves_stack_next = 0; /* the number of blocks (moves_stack_block)  */

/* the changes history, or flips although the first in every block
   is not a flip, but a moved move */
int flips_x[1520];
int flips_y[1520];
int flips_next = 0;
int flips_stack_block[60];
int flips_stack_who[60];
int flips_stack_next = 0;

/* suggest_move returns 1 if a move is possible and below is its suggestion */
int suggested_move_x;
int suggested_move_y;

/* Clear the board, and reset the stacks for another play. */
void clear_board()
{
  int i, j;
  
  for(i = 0; i < 8; i++)
    for(j = 0; j < 8; j++)
      board[i][j] = EMPTY;

  board[3][3] = board[4][4] = WHITE;
  board[3][4] = board[4][3] = BLACK;

  moves_next = 0;
  moves_stack_next = 0;
  flips_next = 0;
  flips_stack_next = 0;
}

/* Count the number of empty spaces on the board. */
inline int number_of_empties()
{
  int i, j, count = 64;

  for(i = 0; i < 8; i++)
    for(j = 0; j < 8; j++)
      if(board[i][j])
        count--;

  return count;
}

/* Determine the validity of a given move. */
int valid_move(int x, int y, int who)
{
  int dx, dy, i, j, opp = OPPOSITE(who);

  if(VALID_CELL(x,y) && board[x][y])
    return 0;
  for(dx = -1; dx < 2; dx++)
    for(dy = -1; dy < 2; dy++)
    {
      i = x + dx;
      j = y + dy;
      /* (dx, dy) forms a vector-direction of travel */
      if((dx || dy) && VALID_CELL(i, j) && (board[i][j] == opp))
      {
        do
        {
          i += dx;
          j += dy;
          /* here we follow the opponents pieces in that direction */
        }
        while(VALID_CELL(i, j) && (board[i][j] == opp));
        /* if they end with one of the players pieces, it's a valid move */
        if(VALID_CELL(i, j) && (board[i][j] == who))
          return 1;
      }
    }
  return 0;
}

/* Go thru all the empty pieces, and push each valid_move onto the
 * stack of moves, and return.
 */
void get_all(int who)
{
  int i, j;

  moves_stack_block[moves_stack_next] = 0;
  for(i = 0; i < 8; i++)
    for(j = 0; j < 8; j++)
      if(valid_move(i, j, who))
      {
        moves_x[moves_next] = i;
        moves_y[moves_next] = j;
        moves_next++;
        moves_stack_block[moves_stack_next]++;
      }
  moves_stack_next++;
}

inline void undo_get_all()
{
  moves_next -= moves_stack_block[--moves_stack_next];
}

/* Assume given move is valid, and perform the addision of the piece
 * to the board, and the flipping of the opponents pieces where
 * applicable.
 */
void make_move(int x, int y, int who)
{
  int dx, dy, i, j, opp = OPPOSITE(who);

  board[x][y] = who;
  flips_x[flips_next] = x;
  flips_y[flips_next] = y;
  flips_next++;
  flips_stack_block[flips_stack_next] = 1;
  flips_stack_who[flips_stack_next] = who;

  for(dx = -1; dx < 2; dx++)
    for(dy = -1; dy < 2; dy++)
    {
      i = x + dx;
      j = y + dy;
      /* branch in each direction */
      if((dx || dy) && VALID_CELL(i, j) && board[i][j] == opp)
      {
        do
        {
          i += dx;
          j += dy;
        }
        while(VALID_CELL(i, j) && (board[i][j] == opp));
        /* if this direction is to be flipped */
        if(VALID_CELL(i, j) && (board[i][j] == who))
        {
          i -= dx;
          j -= dy;
          /* go back and flip each piece */
          while(board[i][j] == opp)
          {
            board[i][j] = who;
            flips_x[flips_next] = i;
            flips_y[flips_next] = j;
            flips_next++;
            flips_stack_block[flips_stack_next]++;
            i -= dx;
            j -= dy;
          };
        }
      }
    }
  flips_stack_next++;
}

/* Undo the last move, removing its piece and all the damage due to
 * flipping that had occured.
 */
void undo_move()
{
  int i, who, top = flips_next;

  flips_next -= flips_stack_block[--flips_stack_next];
  board[flips_x[flips_next]][flips_y[flips_next]] = EMPTY;
  who = OPPOSITE(flips_stack_who[flips_stack_next]);
  for(i = flips_next + 1; i < top; i++)
    board[flips_x[i]][flips_y[i]] = who;
}

/* Calculate the weight of this move in respect to the max_depth of
 * following moves.  Weights are calculated in worst-case, in order to
 * assume the opponent will make the best move possible (at least to
 * the limited perception of max_depth).
 */
int weigh_move(int x, int y, int who, int max_depth, int add)
{
  int bottom, top, i, move_weight = 0, depth_weight = 0, temp;

  /* calculate weight of given move */
  bottom = flips_next;
  make_move(x, y, who);
  top = flips_next;
  if(add)
    for(i = bottom; i < top; i++)
      move_weight += weight[flips_x[i]][flips_y[i]];
  else
    for(i = bottom; i < top; i++)
      move_weight -= weight[flips_x[i]][flips_y[i]];
  /* calculate weight of next move(s) */
  if(max_depth && number_of_empties())
  {
    who = OPPOSITE(who);
    max_depth--;
    add = !add;
    bottom = moves_next;
    get_all(who);
    top = moves_next;
    for(i = bottom; i < top; i++)
    {
      temp = weigh_move(moves_x[i], moves_y[i], who, max_depth, add);
      if(temp < depth_weight)
        depth_weight = temp;
    }
    undo_get_all();
  }
  undo_move();
  return move_weight + depth_weight;
}

/* Here we enumerate all possible moves for who at this time.  The
 * weight of each move is calculated (the worst-case weight) and the
 * best of these is chosen, thereby ensuring among all the worst-cases
 * we choose the best.
 */
int suggest_move(int who, int max_depth)
{
  int bottom, top, i, move_weight, best_weight;

  if(number_of_empties() == 0)
    return 0; /* no moves at all */
  /* ignore difficulty setting at end-game, unless max_depth == 0 */
  if((number_of_empties() < 11) && max_depth)
    max_depth = 10;
  bottom = moves_next;
  get_all(who);
  top = moves_next;
  if(top == bottom)
  {
    undo_get_all();
    return 0; /* no moves for (who) */
  }
  suggested_move_x = moves_x[bottom];
  suggested_move_y = moves_y[bottom];
  best_weight = weigh_move(suggested_move_x, suggested_move_y,
			   who, max_depth, 1);
  for(i = bottom + 1; i < top; i++)
  {
    move_weight = weigh_move(moves_x[i], moves_y[i], who, max_depth, 1);

    if((move_weight > best_weight) ||
       ((move_weight == best_weight) && (rand() % 2)))
    {
      best_weight = move_weight;
      suggested_move_x = moves_x[i];
      suggested_move_y = moves_y[i];
    }
  }
  undo_get_all();
  return 1; /* best move in suggested_move_x/suggested_move_y */
}

void print_board()
{
  int i, j;

  printf("\n   1   2   3   4   5   6   7   8\n");
  for(i = 0; i < 8; i++)
  {
    printf("%c ", 'A' + i);
    for(j = 0; j < 8; j++)
    {
      switch(board[i][j])
      {
        case BLACK: printf("*X*"); break;
        case WHITE: printf(" O "); break;
        default: printf("   ");
      }
      if(j < 7)
        printf("|");
    }
    if(i < 7)
      printf("\n  ---+---+---+---+---+---+---+---\n");
    else
      printf("\n\n");
  }
}

/*************************************************************************/

inline void fclear_eol(FILE* fin)
{
  while(fgetc(fin) != '\n');
}

int main()
{
  struct timeval tv;
  int black_moved=0, white_moved=0, score, i, j;
  char x, y, max_depth;

  gettimeofday(&tv, NULL);
  srand(tv.tv_usec);

  while(1) /* game loop */
  {
beginning:
    clear_board();
    
    do 
    {
      /* Determine how deep the AI should dig for the best move.
         I find 2 a good challange, but I'm getting better.
      */
      printf("AI: Select difficulty: 1-5, or (Q)uit? ");
      scanf("%c", &max_depth);
      fclear_eol(stdin);
      if((max_depth >= '1') && (max_depth <= '5'))
        max_depth -= '0';
      else if((max_depth == 'Q') || (max_depth == 'q'))
      /*
        Q - quit program.
      */
      {
        printf("Quit!\n");
        return 0;
      }
      else
        max_depth = 0;
    } while((max_depth < 1) || (max_depth > 5));

    print_board();

    while(1)
    {
restart_move:    
      printf("Black... ");
      fflush(stdout);
      if(suggest_move(BLACK, 0)) /* hidden move suggestion--heuristic only */
        while(1)
        {
          printf("Specify move (like %c%c; or LIST, UNDO or QUIT): ",
            (char)(suggested_move_x + 'A'),
            (char)(suggested_move_y + '1'));
          scanf("%c%c", &x, &y);
          fclear_eol(stdin);
          if(((x == 'q') || (x == 'Q')) &&
             ((y == 'u') || (y == 'U')))
          /*
            QU - quit out of game-play. 
          */
          {
            printf("Quit!\n\n");
            goto beginning;
          }
          else if(((x == 'l') || (x == 'L')) &&
                  ((y == 'i') || (y == 'I')))
          /*
            LI - list possible moves.
          */
          {
            i = moves_next;
            get_all(BLACK);
            j = moves_next;
            printf("Possible moves: ");
            for(; i < j; i++)
              printf("%c%c ", (char)(moves_x[i] + 'A'),
		              (char)(moves_y[i] + '1'));
            printf("\n");
            undo_get_all();
            continue;
          }
          /*
            UN - undo last move by player, and all AI moves after that.
          */
          else if(((x == 'u') || (x == 'U')) &&
                  ((y == 'n') || (y == 'N')))
          {
            if(flips_stack_next) /* a move has been made */
            {
              do
              {
                undo_move();
                printf("Undid: %s %c%c\n",
                  (flips_stack_who[flips_stack_next] == BLACK)
		       ? "BLACK" : "WHITE",
                  (char)(flips_x[flips_next] + 'A'),
                  (char)(flips_y[flips_next] + '1')
                );
              }
              while(flips_stack_who[flips_stack_next] != BLACK);
              print_board();
              goto restart_move;
            }
            else
              printf("No moves to undo.\n");
            continue;
          }
          if((x >= 'A') && (x <= 'Z'))
            x -= 'A';
          else if((x >= 'a') && (x <= 'z'))
            x -= 'a';
          else 
            x = -1;
          if((y >= '0') && (y <= '9'))
            y -= '1';
          else
            y = -1;
          if(VALID_CELL(x, y) && valid_move(x, y, BLACK))
          {
            make_move(x, y, BLACK);
            black_moved = 1;
            print_board();
            break;
          }
          else
            printf("That is not a valid move.\n");
        }
      else
      {
        printf("Cannot move.  Pass!  [press <enter> to continue]");
        fclear_eol(stdin);
        black_moved = 0;
      }

      if((!black_moved && !white_moved) || !number_of_empties()) break;

      printf("White... ");
      fflush(stdout);
      if(suggest_move(WHITE, max_depth))
      {
        printf("Playing %c%c\n", (char)(suggested_move_x + 'A'),
                                 (char)(suggested_move_y + '1'));
        make_move(suggested_move_x, suggested_move_y, WHITE);
        white_moved = 1;
        print_board();
      }
      else
      {
        printf("Cannot move.  Pass!\n");
        white_moved = 0;
      }

      if((!black_moved && !white_moved) || !number_of_empties()) break;
    };

    /* calculate score */

    score = black_moved = white_moved = 0;

    for(i = 0; i < 8; i++)
      for(j = 0; j < 8; j++)
        switch(board[i][j])
        {
          case BLACK:
            black_moved++;
            score++;
            break;
          case WHITE:
            white_moved++;
            score--;
            break;
        }

    if(score > 0)
      printf("BLACK WINS! %i:%i (%f%%)\n\n", black_moved, white_moved,
	     ((float)black_moved / (black_moved + white_moved)) * 100);
    else if(score < 0)
      printf("WHITE WINS! %i:%i (%f%%)\n\n", white_moved, black_moved,
	     ((float)white_moved / (black_moved + white_moved)) * 100);
    else
      printf("TIE, NOBODY WINS!\n\n");
  };

  return 0;
}
