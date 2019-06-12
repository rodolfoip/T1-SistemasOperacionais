#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define EMPTY ' '
#define CURSOR_PAIR 1
#define TOKEN_PAIR 2
#define EMPTY_PAIR 3
#define LINES 11
#define COLS 11
#define TOKENS 5

void init_ncurses();
void init_table();
void init_tokens();
void init_cursor();
void init_countdown();
void *move_token(void *token);
void *move_cursor();
void board_refresh(void);
void *countdown_timer();
void move_tokens();
void match_move();
void menu_game();
void run_game();
pthread_t token[TOKENS];
pthread_t cursor_thread;
pthread_t countdown_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int id[TOKENS] = {0, 1, 2, 3, 4};
int captured_tokens = 0;
int timer = 10;
int speed_token = 1;
bool end_game = false;

typedef struct CoordStruct
{
  int x;
  int y;
  bool running;
} coord_type;

coord_type cursor, coord_tokens[TOKENS];

int main(void)
{
  menu_game();
  endwin();
  exit(0);
}
void init_ncurses()
{
  srand(time(NULL)); /* inicializa gerador de numeros aleatorios */

  /* inicializa curses */
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();

  /* inicializa colors */
  if (has_colors() == FALSE)
  {
    endwin();
    printf("Seu terminal nao suporta cor\n");
    exit(1);
  }

  start_color();
  /* inicializa pares caracter-fundo do caracter */

  init_pair(CURSOR_PAIR, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(TOKEN_PAIR, COLOR_RED, COLOR_RED);
  init_pair(EMPTY_PAIR, COLOR_BLUE, COLOR_BLUE);
}

void menu_game()
{
  init_ncurses();

  clear();
  mvprintw(0, 0, "Selecione um nível:");
  mvprintw(1, 0, "(1) Fácil - 150 segundos para captura");
  mvprintw(2, 0, "(2) Médio - 80 segundos para captura");
  mvprintw(3, 0, "(3) Dificil - 50 segundos para captura");
  mvprintw(4, 0, "(Q/q) Fechar");

  switch (getch())
  {
  case '1':
    speed_token = 2.0;
    timer = 150;
    run_game();
    break;

  case '2':
    speed_token = 1.5;
    timer = 80;
    run_game();
    break;
  case '3':
    speed_token = 1;
    timer = 50;
    run_game();
    break;

  case 'q':
  case 'Q':
    endwin();
    exit(0);
  default:
    menu_game();
    break;
  }
}

void run_game()
{
  clear();
  refresh();

  init_table();     /* inicializa o tabuleiro */
  init_tokens();    /* inicializa os tokens no tabuleiro */
  init_cursor();    /* inicializa o cursor no tabuleiro */
  move_tokens();    /* move os tokens aleatoriamente */
  board_refresh();  /* redesenha tabuleiro */
  init_countdown(); /* porque sou justo XD */

  for (int i = 0; i < TOKENS; i++)
  {
    pthread_join(token[i], NULL);
  }
  pthread_join(cursor_thread, NULL);
  pthread_join(countdown_thread, NULL);
}

void *move_cursor()
{
  int ch;
  do
  {
    ch = getch();
    pthread_mutex_lock(&mutex);

    switch (ch)
    {
    case KEY_UP:
    case 'w':
    case 'W':
      if ((cursor.y > 0))
      {
        cursor.y = cursor.y - 1;
      }
      break;
    case KEY_DOWN:
    case 's':
    case 'S':
      if ((cursor.y < LINES - 1))
      {
        cursor.y = cursor.y + 1;
      }
      break;
    case KEY_LEFT:
    case 'a':
    case 'A':
      if ((cursor.x > 0))
      {
        cursor.x = cursor.x - 1;
      }
      break;
    case KEY_RIGHT:
    case 'd':
    case 'D':
      if ((cursor.x < COLS - 1))
      {
        cursor.x = cursor.x + 1;
      }
      break;
    }

    match_move();
    board_refresh(); /* redesenha tabuleiro */
    pthread_mutex_unlock(&mutex);

  } while ((ch != 'q') && (ch != 'Q'));

  pthread_mutex_unlock(&mutex);
  pthread_exit(0);
}

void move_tokens()
{
  int i;
  for (i = 0; i < TOKENS; i++)
    pthread_create(&token[i], NULL, move_token, (void *)&id[i]);
}

void *move_token(void *token)
{
  int token_id = *((int *)token);
  while (coord_tokens[token_id].running)
  {
    pthread_mutex_lock(&mutex);

    /* code */
    int i = token_id, new_x, new_y;

    /* determina novas posicoes (coordenadas) do token no tabuleiro (matriz) */
    do
    {
      new_x = rand() % (COLS);
      new_y = rand() % (LINES);
    } while ((new_x == cursor.x) && (new_y == cursor.y));

    /* coloca token na nova posicao */
    coord_tokens[i].x = new_x;
    coord_tokens[i].y = new_y;

    board_refresh();
    pthread_mutex_unlock(&mutex);
    sleep(speed_token); /* Velocidade em que os tokens se movem */
  }

  pthread_exit(0);
}

void match_move()
{
  int i;
  for (i = 0; i < TOKENS; i++)
  {
    if (coord_tokens[i].running && coord_tokens[i].x == cursor.x && coord_tokens[i].y == cursor.y)
    {
      coord_tokens[i].running = false;
      captured_tokens = captured_tokens + 1;

      if (captured_tokens == TOKENS)
      {
        pthread_cancel(countdown_thread);
        end_game = TRUE;
        pthread_exit(0);
      }
    }
  }
}

void *countdown_timer()
{
  do
  {
    move(5, 15);
    clrtoeol();

    pthread_mutex_lock(&mutex);

    mvprintw(5, 15, "Time remaining %i seconds", timer--);
    refresh();

    pthread_mutex_unlock(&mutex);
    sleep(1);

  } while (timer >= 0);

  pthread_cancel(cursor_thread);
  refresh();
  end_game = TRUE;
  pthread_exit(0);
}

void board_refresh(void)
{
  clear();

  int x, y, i;

  /* redesenha tabuleiro "limpo" */

  for (x = 0; x < COLS; x++)
    for (y = 0; y < LINES; y++)
    {
      attron(COLOR_PAIR(EMPTY_PAIR));
      mvaddch(y, x, EMPTY);
      attroff(COLOR_PAIR(EMPTY_PAIR));
    }

  /* poe os tokens no tabuleiro */

  for (i = 0; i < TOKENS; i++)
  {
    if (coord_tokens[i].running)
    {
      attron(COLOR_PAIR(TOKEN_PAIR));
      mvaddch(coord_tokens[i].y, coord_tokens[i].x, EMPTY);
      attroff(COLOR_PAIR(TOKEN_PAIR));
    }
  }
  /* poe o cursor no tabuleiro */

  move(y, x);
  attron(COLOR_PAIR(CURSOR_PAIR));
  mvaddch(cursor.y, cursor.x, EMPTY);
  attroff(COLOR_PAIR(CURSOR_PAIR));
  refresh();
}

void init_countdown()
{
  pthread_create(&countdown_thread, NULL, countdown_timer, NULL);
}

void init_table()
{
  clear();
  for (int i = 0; i < LINES; i++)
  {
    for (int j = 0; j < COLS; j++)
    {
      attron(COLOR_PAIR(EMPTY_PAIR));
      mvaddch(j, i, EMPTY);
      attroff(COLOR_PAIR(EMPTY_PAIR));
    }
  }
  refresh();
}

void init_cursor()
{
  clear();

  cursor.x = 6;
  cursor.y = 6;

  refresh();

  pthread_create(&cursor_thread, NULL, move_cursor, NULL);
}

void init_tokens()
{
  for (int i = 0; i < TOKENS; i++)
  {
    clear();

    coord_tokens[i].running = true;
    coord_tokens[i].x = 0;
    coord_tokens[i].y = 0;

    refresh();
  }
}