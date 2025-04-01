#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char **argv) {
  FILE *f;
  struct stat *st = malloc(sizeof(struct stat));
  char buff[20];

  f = fopen("posts/about.html", "r");
  stat("posts/about.html", st);

  strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&st->st_mtim.tv_sec));

  printf("%s", buff);
}
