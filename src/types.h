#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if _WIN32 || _WIN64
   #if _WIN64
     #define ENVBIT 64
  #else
    #define ENVBIT 32
  #endif
#endif

// Check GCC
#if __GNUC__
  #if __x86_64__ || __ppc64__
    #define ENVBIT 64
  #else
    #define ENVBIT 32
  #endif
#endif

/*
 * least significant one bit of an integer
 * relevant to iterate through struct pages
 */
#define LSONE(i) (i&(-i))

struct book_t {
  char *title;
  char *url;
  char *authors;
  char *year;
  char *publisher;
  char *pages;
  char *lang;
  char *size;
  char *ext;
  char *id;
  char *volume;
  char *edition;
  char *isbn;
  char *series;
  char *description;
  char *download_url;
  char *path;
};

typedef struct {
   struct book_t *books;
   int size;
} BOOK_CONTAINER;

struct pages {
   BOOK_CONTAINER lib[64];
   uint64_t bitset;
   int max_pages;
};

void book_t_print(const struct book_t *book);
void book_t_free(const struct book_t *book);
void array_book_t_free(BOOK_CONTAINER* book_arr);
void pages_book_t_free(struct pages* lib);

#endif
