#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define NUM_DAYS_IN_MONTH 30

struct kvpair {
  int key;
  char *value;
};

struct kvpair ch_cal[12] = {{0, "寅"}, {1, "卯"}, {2, "辰"},
                             {3, "巳"},     {4, "午"},   {5, "未"},
                             {6, "申"}, {7, "酉"},   {8, "戌"},
                             {9, "亥"},   {10, "子"},  {11, "丑"}};

struct kvpair kor_cal[12] = {{0, "호랑이"}, {1, "토끼"}, {2, "용"},
                             {3, "뱀"},     {4, "말"},   {5, "양"},
                             {6, "원숭이"}, {7, "닭"},   {8, "개"},
                             {9, "돼지"},   {10, "쥐"},  {11, "소"}};

struct kvpair holidays[14] = {
    {(0 * 30) + 0, "설날"}, {(0 * 30) + 14, "大보름"}, {(1 * 30) + 0, "머슴날"},  {(2 * 30) + 2, "삼짇날"},
    {-1, "한식"}, {(3 * 30) + 7, "초파일"}, {(4 * 30) + 0, "단오"}, {(5 * 30) + 14, "유두"}, {(6 * 30) + 6, "칠석"}, 
    {(6 * 30) + 14, "백중"}, {(7 * 30), "추석"},  {(8 * 30) + 8, "중양절"}, {-1, "동지"},  {(11 * 30) + 29, "섣달그믐"}
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

void init_dangun(Dangun *d, struct tm *t) {
  // gregorian
  d->g_mday = t->tm_mday;
  d->g_month = t->tm_mon;
  d->g_year = (t->tm_year) + 1900;

  // danggi
  d->d_day = t->tm_yday;
  d->d_wday = t->tm_wday;
  d->d_year = t->tm_year;
  d->d_mday = 0;
  d->d_month = 0;
}


void find_day(Dangun *d, int start) {
  int day = d->d_day;

  // mod by starting day and subtract one since zero indexing to get the day of
  // the korean month
  int kor_day = day % 30;
  d->d_mday = kor_day;
}

int find_month(Dangun *d, int start) {
  int day = d->d_day;

  int mults[12] = { 0 };

  for (int i = 0; i < 12; i++) {
    mults[i] = start + (i * NUM_DAYS_IN_MONTH);
  }
  
  if (day >= start && day <= mults[1]) {
    d->d_month = 0;
  } else if (day > mults[1] && day <= mults[2]) {
    d->d_month = 1;
  } else if (day > mults[2] && day <= mults[3]) {
    d->d_month = 2;
  } else if (day > mults[3] && day <= mults[4]) {
    d->d_month = 3;
  } else if (day > mults[4] && day <= mults[5]) {
    d->d_month = 4;
  } else if (day > mults[5] && day <= mults[6]) {
    d->d_month = 5;
  } else if (day > mults[6] && day <= mults[7]) {
    d->d_month = 6;
  } else if (day > mults[7] && day <= mults[8]) {
    d->d_month = 7;
  } else if (day > mults[8] && day <= mults[9]) {
    d->d_month = 8;
  } else if (day > mults[9] && day <= mults[10]) {
    d->d_month = 9;
  } else if (day > mults[10] && day <= mults[11]) {
    d->d_month = 10;
  } else {
    d->d_month = 11;
  }

  char *kor_month = kor_cal[d->d_month].value;
  char *ch_month = ch_cal[d->d_month].value;
  d->d_month_str_kr = malloc(sizeof(char) * strlen(kor_cal[d->d_month].value) + 1);
  d->d_month_str_ch = malloc(sizeof(char) * strlen(ch_cal[d->d_month].value) + 1);
  strcpy(d->d_month_str_kr, kor_cal[d->d_month].value);
  strcpy(d->d_month_str_ch, ch_cal[d->d_month].value);
  if (!d->d_month_str_kr || !d->d_month_str_ch) {
    fprintf(stderr, "%d: malloc error in %s\n", __LINE__, __FUNCTION__);
    return -1;
  }
  return 0;
}

void find_year(Dangun *d) {
    d->d_year += (1900 + 2333);
}

struct solstice *get_solstice(struct tm *t) {
    int year = 1900 + t->tm_year;
    double y = (year - 2000) / 1000.;

    // from Jean Meeus "Astronomical Algorithms"
    double w_solstice_JD = 2451900.05952 + (365242.74049 * y) -
                           (0.06223 * (y * y)) - (0.00823 * (y * y * y)) +
                           (0.00032 * (y * y * y * y));

    // from Edward Graham Richards; convert from Julian Day Num to Gregorian
    int f = w_solstice_JD + 1401;
    int e = 4 * f + 3;
    int g = (e % 1461) / 4;
    int h = 5 * g + 2;

    int d = ceil((double) ((h % 153) / 5. + 1.)) + 13;
    int m = (((h / 153) + 2) % 12) + 1;

    // above we found gregorian date for the solstice; we wish to translate into
    // the danggi dates which we do very jankily with a dumb "filler" variable
    Dangun *filler = malloc(sizeof(Dangun));
    if (!filler) {
      free(filler);
      fprintf(stderr, "malloc error in find_solstice");
      return NULL;
    }
    filler->d_day = d;
    filler->d_month = m;
    filler->d_year = year;

    find_day(filler, 21);
    find_month(filler, 21);
    find_year(filler);

    struct solstice *ret = malloc(sizeof(struct solstice));
    ret->day = filler->d_mday;
    ret->month = filler->d_month;
    ret->year = filler->d_year;

    free(filler);

    return ret;
}

int init_holidays(struct kvpair *h, int len) {
    for (int i = 0; i < len; i++) {
        if (h[i].key < 0) {
        }
    }
    return 0;
}

/* int main(int argc, char **argv) {
  Dangun *dangun = (Dangun *)malloc(sizeof(Dangun));
  if (!dangun) {
    fprintf(stderr, "%d: malloc error", __LINE__);
    free(dangun);
    exit(1);
  }
  struct tm *a;
  char *c;
  time_t t1;
  time(&t1);
  a = gmtime(&t1);
  struct solstice *s;

  // bounds change yearly. last updated 2024
  // 2024 lunar calendar begins on Jan 21
  int begin = 21;

  init_dangun(dangun, a);

  s = get_solstice(a);

  if (!s) {
      fprintf(stderr, "%d: get_solstice error", __LINE__);
      exit(1);
  }

  if (find_month(dangun, begin) != 0) {
      fprintf(stderr, "%d: find_month error", __LINE__);
      exit(1);
  }

  find_day(dangun, begin);
  find_year(dangun);

  init_holidays(holidays, sizeof(holidays));

  printf("%s(%d)월 %d일 %d년\n", dangun->d_month_str_kr, dangun->d_month+1, dangun->d_mday, dangun->d_year);
  printf("동지: %d년 %d월 %d일\n", s->year, s->month, s->day);

  return 0;
} */
