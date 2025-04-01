#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_DAYS_IN_MONTH 30

struct kvpair {
  int key;
  char *value;
};

struct solstice {
  int day;
  int month;
  int year;
};

typedef struct dangun {
  int g_mday;
  int g_month;
  int g_year;

  int d_day;
  int d_wday;
  int d_mday;
  int d_month;
  int d_year;
  char *d_month_str_kr;
  char *d_month_str_ch;
} Dangun;

extern struct kvpair holidays[14];
extern struct kvpair kor_cal[12];
extern struct kvpair ch_cal[12];

void init_dangun(Dangun *, struct tm *);
void find_day(Dangun *, int);
int find_month(Dangun *, int);
void find_year(Dangun *);
struct solstice *get_solstice(struct tm *);
int init_holidays(struct kvpair *, int);
