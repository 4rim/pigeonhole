#include "kr_time.h"
#include <ctype.h>
#include <stdbool.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXFILENUM 100
#define MAXTITLELEN 256
#define CONTENTDIR "posts"

char *perens[MAXFILENUM] = {
    "index.html", "journal.html", "nest.html", "about.html",
    "firstreformed.html", "chouette.html", "chouette-ebola.html",
    "so-you-want-to-learn-russian.html", "russian.html", "types-in-lean.html",
    "pgp.html", "florilegium.html", "harmony-lessons.html", "senseless.html",
    "tiger.html", "moving-on.html"
};

typedef struct post {
    char* title;
    struct stat *stat_info;
    bool ephem; // temporal or ephemeral?
} Post;

typedef struct Dict {
  int len;
  Post post_list[MAXFILENUM];
  // char files[MAXFILENUM][64];
} Dict;

typedef struct pair {
  int num;
  const char *name;
} Pair;

Pair weekdays[7] = {
    {0, "日"}, {1, "月"}, {2, "火"}, {3, "水"}, {4, "木"}, {5, "金"}, {6, "土"},
};

int
comp(const void * e1, const void * e2) 
{
    Post a = *((Post *)e1);
    Post b = *((Post *)e2);

    if (!e1 || !e2)
        return 0; // don't do anything

    long s = a.stat_info->st_mtime;
    long t = b.stat_info->st_mtime;

    // printf("s = %ld, t = %ld\n", s, t); // debug
    if (s > t) return -1; // more recent
    if (s < t) return 1;
    return 0;
}

const char *
get_filename_ext(const char *filename)
{
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return "";
  return dot + 1;
}

char *
path_join(char *a, char *b)
{
  int lena, lenb;
  if (!a)
    lena = 0;
  else
    lena = strlen(a);
  if (!b)
    lenb = 0;
  else
    lenb = strlen(b);
  char *str = malloc(lena + lenb + 2);
  if (str == NULL) {
    free(str);
    exit(1);
  }

  memcpy(str, a, lena);
  memcpy(str + lena, "/", sizeof(char));
  memcpy(str + (lena + 1), b, lenb);

  str[lena + lenb + 1] = '\0';
  return str;
}

void
upper_to_lower(char *in)
{
  while (*in != '\0') {
    *in = tolower(*in);
    in++;
  }
}

void spaces_to_dashes(char *in) {
  while (*in != '\0') {
    if (*in == 32) { // 32 = space
      *in = '-';
    }
    ++in;
  }
}

char *
surround(char *c, char *tag)
{
    // TODO: Refactor this
    char *left_open = "<";
    char *right_open = "</";
    char *close = ">";
    char *ret = malloc(MAXTITLELEN);
    if (!ret) {
      free(ret);
      fprintf(stderr, "%d: malloc() fail", __LINE__);
    }
    strcpy(ret, left_open);
    strcpy(ret + 1, tag);
    strcpy(ret + 1 + strlen(tag), close);
    strcpy(ret + 1 + strlen(tag) + 1, c);
    strcpy(ret + 1 + strlen(tag) + 1 + strlen(c), right_open);
    strcpy(ret + 1 + strlen(tag) + 1 + strlen(c) + 2, tag);
    strcpy(ret + 1 + strlen(tag) + 1 + strlen(c) + 2 + strlen(tag), close);
    return ret;
}

char *
get_korean_date(void)
{
    Dangun *dangun = (Dangun *)malloc(sizeof(Dangun));
    if (!dangun) {
        fprintf(stderr, "%d: malloc error", __LINE__);
        free(dangun);
        return NULL;
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
        return NULL;
    }
    
    if (find_month(dangun, begin) != 0) {
        fprintf(stderr, "%d: find_month error", __LINE__);
        return NULL;
    }
    
    find_day(dangun, begin);
    find_year(dangun);
    
    init_holidays(holidays, sizeof(holidays));
    
    const char *ch_month = dangun->d_month_str_ch;
    int kr_day = dangun->d_mday;
    int kr_year = dangun->d_year;
    char str_day[10];
    char str_year[10];
    
    sprintf(str_day, "%d", kr_day);
    sprintf(str_year, "%d", kr_year);
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int wday = tm.tm_wday;
    const char *ohaeng;
    
    for (int i = 0; i < 7; i++) {
        if (wday == weekdays[i].num) {
            ohaeng = weekdays[i].name;
        }
    }
    
    
   char *date = malloc(sizeof(char) * 126);
   if (date == NULL) {
       free(date);
       fprintf(stderr, "date malloc fail");
       return NULL;
   }

    strcpy(date, str_year);
    strcat(date, "/");
    strcat(date, ch_month);
    strcat(date, "/");
    strcat(date, str_day);
    strcat(date, " ");
    strcat(date, ohaeng);
    strcat(date, "\0");
    char *subheader = surround(date, "h2");
    
     // fwrite(subheader, strlen(subheader) + 1, 1, f);
    return subheader;
}

int
inject(FILE *dest, FILE *src)
{
    int c;
    if (src) {
        while ((c = fgetc(src)) != EOF) {
        fputc(c, dest);
        }
    } else {
        fprintf(stderr, "%d: fopen error\n", __LINE__);
        return 1;
    }
    return 0;
}

FILE *
create_site_file(char *title)
{
  // this takes in barebones HTML content from posts/ dir, and spits it out
  // formatted nicely into site/ dir (with header, nav, css, etc)

  char *site_path = malloc(sizeof(char) * strlen("site") + 2 + strlen(title) +
                           strlen(".html") + 2);
  char *posts_path = malloc(sizeof(char) * strlen(CONTENTDIR) + 2 + strlen(title) +
                            strlen(".html") + 2);
  if (site_path == NULL || posts_path == NULL) {
    free(site_path);
    free(posts_path);
    exit(1);
  }

  char *temp1 = path_join("site", title);
  char *temp2 = path_join(CONTENTDIR, title);

  strcpy(site_path, temp1);
  strcpy(posts_path, temp2);
  strcat(site_path, ".html");
  strcat(posts_path, ".html");

  FILE *f = fopen(site_path, "w");
  if (!f) {
    fprintf(stderr, "fopen failed.\n");
  }

  fputs("<!DOCTYPE html><html lang='en'>\n", f);
  fputs("<head>", f);
  fprintf(f,
          "<meta charset='utf-8'>"
          "<meta name='viewport' content='width=device-width,initial-scale=1'>"
          "<link rel='stylesheet' type='text/css' href='../css/posts.css'>");
  fputs("</head>\n", f);
  fputs("<body>\n", f);

  fputs("<header>\n", f);
  fputs("<nav>\n\
          <b><a href=\"../site/index.html\">home</a></b><br />\n\
          <b><a href=\"../site/about.html\">about</a></b><br />\n\
          <b><a href=\"../site/journal.html\">journal</a></b><br />\n\
          <b><a href=\"../site/nest.html\">nest</a></b><br />\n\
          </nav>",
        f);
  fputs("</header>\n", f);


  fputs("<main>\n", f);
  char *header = surround(title, "h1");
  fwrite(header, strlen(header), 1, f);

  //  char *kr_date = get_korean_date();
  // kr_date = surround(kr_date, "h1");
  // fwrite(kr_date, strlen(kr_date) + 1, 1, f);

  FILE *src = fopen(posts_path, "r");

  if (inject(f, src) != 0) {
      fprintf(stderr, "%d: inject error\n", __LINE__);
  }

  fputs("</main>\n", f);

  fputs("<footer>\n", f);
  fputs("</footer>\n", f);

  fputs("</body></html>", f);
  free(site_path);
  free(posts_path);
  free(temp1);
  free(temp2);
  return f;
  // }
  return NULL;
}

int
build(Dict *d)
{
  for (int i = 0; i < MAXFILENUM; i++) {
    char *fname = d->post_list[i].title;
    if (fname == NULL) {
      return 0;
    }
    strtok(fname, ".");
    if (!create_site_file(fname)) {
      fprintf(stderr, "create_site_file error\n");
    }
  }
  return 0;
}

int
init_dict(Dict *dict, DIR *dir)
{
  Dict *curr = dict;
  int j = 0;
  struct dirent *ep;

  memset(dict->post_list, 0, sizeof(Post) * MAXFILENUM);

  while ((ep = readdir(dir)) != NULL) {
    const char *ext = get_filename_ext(ep->d_name);
    if (strcmp(ext, "html") == 0) {
        curr->post_list[j].title = malloc(strlen(ep->d_name) + 1);
        curr->post_list[j].stat_info = malloc(sizeof(struct stat));
        if (curr->post_list[j].title == NULL || curr->post_list[j].stat_info == NULL) {
            free(curr->post_list[j].title);
            free(curr->post_list[j].stat_info);
            fprintf(stderr, "malloc error in init_dict");
            exit(1);
        }
        ++curr->len;
        strncpy(curr->post_list[j].title, ep->d_name, strlen(ep->d_name) + 1);
        strncpy(curr->post_list[j].title, ep->d_name, strlen(ep->d_name) + 1);

        stat(path_join(CONTENTDIR, curr->post_list[j].title), curr->post_list[j].stat_info);

        for (int i = 0; i < MAXFILENUM && perens[i] != NULL; ++i) {
            if (strcmp((char *)ep->d_name, perens[i]) == 0) {
                curr->post_list[j].ephem = true;
                break;
            } else { curr->post_list[j].ephem = false; }
        }
        ++j;
    }
  }
  closedir(dir);
  return curr->len;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }
    DIR *dp;
    Dict dict;

    // memset(dict.files, '\0', sizeof(char) * MAXFILENUM * 64);
    dict.len = 0;

    if (!(dp = opendir(CONTENTDIR))) {
        fprintf(stderr, "post directory does not exist");
        exit(1);
    }
  
    int num_files = init_dict(&dict, dp);
    printf("%d pages recorded.\n", num_files);
  
    if (strlen(argv[1]) > 1) {
        fprintf(stderr, "Expected one character for 2nd argument; actual (\"%s\") too long\n", argv[1]);
      return 1;
    }
  
    char opt = argv[1][0];
    char title[MAXTITLELEN];
    FILE *outfile;
  
    switch (opt) {
        // build site
        case 'b':
            build(&dict);
            break;

        // new post
        case 'n': 
            printf("Provide a title for your new post:\n");
            fgets(title, MAXTITLELEN, stdin);
            title[strlen(title) - 1] = '\0';
            upper_to_lower(title);
            spaces_to_dashes(title);
            create_site_file(title);
            break;
  
        case 'r': // refresh blog listing
            qsort(dict.post_list, num_files, sizeof(Post), comp); // sort listing by most to least recent 
                                                                  //
            outfile = fopen(path_join(CONTENTDIR, "journal.html"), "w");
             char *disclaimer = "<figure><img src=\"../media/disclaimer.jpeg\" alt=\"If you are reading this and know me in real life: I'm happy and fulfilled and not mentally ill in any way, shape, or form. Any accusations to the contrary amount to slander and libel.\" /></figure>";
            fwrite(disclaimer, strlen(disclaimer), 1, outfile);
            fwrite("<ul>", strlen("<ul>"), 1, outfile);
            for (int i = 0; i < MAXFILENUM; i++) {
                if (dict.post_list[i].title == NULL) break;
                char *post_name = malloc(strlen(dict.post_list[i].title) + 1);
                if (!post_name) {
                    fprintf(stderr, "%d: malloc error", __LINE__);
                    free(post_name);
                    exit(1);
                }
                strcpy(post_name, dict.post_list[i].title);
                strtok(post_name, ".");

                if (dict.post_list[i].ephem == false) {
                    // bro you gotta fix this it's so bad
                    char buff[64];
                    char buf2[64];
                    strcpy(buff, "<span class=\"right\">");
                    strftime(buf2, 64, "\t%Y-%m-%d", localtime(&dict.post_list[i].stat_info->st_mtime));
                    strcat(buff, buf2);
                    strcat(buff, "</span>");
                    char *link = path_join("../site", dict.post_list[i].title);
                    int len = strlen(link) + 128;
                    char *html_link = (char *)malloc(sizeof(char) * len + 64);
                    if (!html_link) {
                        free(html_link);
                        fprintf(stderr, "%d: malloc error", __LINE__);
                    }
                    strcpy(html_link, "<a href=\"");
                    strcat(html_link, link);
                    strcat(html_link, "\" class=\"title\">");
                    strcat(html_link, post_name);
                    strcat(html_link, "</a>");
                    strcat(html_link, buff);
                    fwrite(strcat(surround(html_link, "li"), "\n"), strlen(surround(html_link, "li")) + 1, 1, outfile);
                } else { continue; }
            }
        fwrite("</ul>", strlen("</ul>"), 1, outfile);
        fclose(outfile);
        create_site_file("journal");
        break;
  
    default:
        printf("Not a command I know.\n");
        break;
    }

    Dict *d = &dict;
  // debugging; examine dict.files
    for (int i = 0; i < d->len; ++i) {
        if (d->post_list[i].ephem == false) printf("%s\n", d->post_list[i].title);
    }

    // debug for kr cal
    printf("%s", get_korean_date());

    return 0;
}
