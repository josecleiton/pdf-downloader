#include "parse.h"

void format_file(FILE *stream, register char *buffer) {
  /* take care of end of lines and multiple spaces in linear time */
  register char aux;
  register bool catched_space = false;
  while ((aux = fgetc(stream)) != EOF) {
    if ((!iscntrl(aux) && aux != ' ') || (aux == ' ' && !catched_space))
      *buffer++ = aux;
    catched_space = aux == ' ';
  }
  *buffer = '\0';
  fclose(stream);
}

int handle_td_element(const char *buffer, const int buffer_len,
                      struct book_t *book, const int which_td) {
  register char *match = NULL;
  if (which_td == BOOK_AUTHORS) {
    size_t authors_len = 0LL, max_authors_len = LOGMSG_SIZE;
    int authors_count = 0;
    book->authors = ecalloc(1, max_authors_len);
    while ((match = strstr(buffer, "author"))) {
      TEST_STR_PTR(match, FAILURE);
      if (authors_len > max_authors_len) {
        book->authors = (char *)erealloc(book->authors, max_authors_len * 2);
        max_authors_len *= 2;
      }
      if (authors_count)
        book->authors[authors_len++] = ',';
      match = strchr(match, '>') + 1;
      TEST_STR_PTR(match - 1, FAILURE);
      char *close_tag = strchr(match, '<');
      TEST_STR_PTR(match, FAILURE);
      *close_tag = '\0';
      for (int cursor = 0; match[cursor]; cursor += 1, authors_len += 1) {
        book->authors[authors_len] = match[cursor];
        if (authors_len > max_authors_len) {
          book->authors = (char *)erealloc(book->authors, max_authors_len * 2);
          max_authors_len *= 2;
        }
      }
      buffer = close_tag + 1;
      authors_count += 1;
    }
    book->authors[authors_len] = '\0';
  } else if (which_td == BOOK_TITLE) {
    /* search for the book's url and it's title */
    match = strstr(buffer, "book");
    TEST_STR_PTR(match, FAILURE);
    book->url = (char *)ecalloc(strchr(match, '\'') - match + 1, sizeof(char));
    sscanf(match, "%[^']", book->url);
    match = strchr(match, '>') + 1;
    TEST_STR_PTR(match - 1, FAILURE);
    book->title = (char *)ecalloc(strchr(match, '<') - match + 1, sizeof(char));
    sscanf(match, "%[^<]", book->title);
  } else if (which_td == BOOK_YEAR) {
    /* just jump the nowrap parameter */
    book->year = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->year, ((match = strchr(buffer, '>'))) ? match + 1 : buffer);
  } else if (which_td == BOOK_PUBLISHER) {
    book->publisher = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->publisher, buffer);
  } else if (which_td == BOOK_PAGES) {
    book->pages = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->pages, ((match = strchr(buffer, '>'))) ? match + 1 : buffer);
  } else if (which_td == BOOK_LANG) {
    book->lang = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->lang, buffer);
  } else if (which_td == BOOK_SIZE) {
    book->size = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->size, ((match = strchr(buffer, '>'))) ? match + 1 : buffer);
  } else {
    book->ext = (char *)ecalloc(buffer_len, sizeof(char));
    strcpy(book->ext, ((match = strchr(buffer, '>'))) ? match + 1 : buffer);
  }
  return SUCCESS;
}

/*
 * GREP THE [2-9] TD AFTER FIRST TR
 * AUTHORS | TITLE | PUBLISHER | YEAR | PAGES | LANG | SIZE | EXT
 */
char *search_page(FILE *page_file, const int file_size,
                  struct book_t **book_array, int *book_array_len, int *status,
                  int *max_pages_in_search) {

  enum {
    PATTERNS = 4,
    MAX_TD_PER_SEARCH = 8,
    FILE_BEG = 40000, /* file beginning position */
  };

  /* all patterns we are looking for in html page */
  static char *pattern[] = {
      "table width=100% cellspacing=1 cellpadding=1 rules=rows",
      "<tr valign=top bgcolor",
      "<td",
      "</td",
  };

  /* set the file's cursor and compute it's size */
  if (fseek(page_file, FILE_BEG, SEEK_SET) && ferror(page_file))
    die("parse.c - fseek in search_page");

  // char buffer[BOOK_PARSE_BUFFER_SIZE] = {'\0'};
  char *buffer = (char *)ecalloc(file_size, sizeof(char));
  /* put the formated file in memory */
  format_file(page_file, (char *)buffer);
  /* printf("%lu\n", strlen(buffer)); */

  struct book_t *books;
  books = *book_array =
      (struct book_t *)ecalloc(MAX_BOOKS_PER_PAGE, sizeof(struct book_t));

  char *match_cursor = NULL;
  int book_count = 0;
  *status = FAILURE;

  /* jump after book's table and it's first tr in html buffer */
  char *buffer_cursor = strstr(buffer, pattern[0]) + 85;
  TEST_STR_PTR(buffer_cursor - 85,
               error_msg("[ERROR] parse.c - Only part of the page "
                         "received. Try again. \nHTTP REQUEST #1"));
  bool book_td_parse_finalized = false;

  while (book_count < MAX_BOOKS_PER_PAGE)  {
    buffer_cursor = strstr(buffer_cursor, pattern[1]) + 1;
    if (!(buffer_cursor - 1) && book_count)
      break;
    else if (!(buffer_cursor - 1))
      return error_msg("[ERROR] parse.c - BOOK NOT FOUND");
    for (int i = 0; i < 2; i += 1) {
      buffer_cursor = strstr(buffer_cursor, "td") + 1;
      TEST_STR_PTR(buffer_cursor - 1,
                   error_msg("[ERROR] parse.c - BOOK NOT FOUND 2"));
    }
    int curr_td = 0;
    while (curr_td < MAX_TD_PER_SEARCH) {
      if ((match_cursor = strstr(buffer_cursor, "<td"))) {
        match_cursor = strchr(match_cursor, '>') + 1;
        char *close_td = strstr(match_cursor, "td>");
        char *handle_close_td = close_td;
        TEST_STR_PTR(close_td,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #3"));
        close_td = strrchr_b(close_td, '<');
        *close_td = '\0';
        if ((*status =
                 handle_td_element(match_cursor, close_td - match_cursor + 1,
                                   &books[book_count], curr_td + 1)) == FAILURE)
          goto STATUS_CHECK;
        match_cursor = buffer_cursor = handle_close_td + 1;
        curr_td += 1;
      } else {
      STATUS_CHECK:
        if (!book_count)
          return error_msg("[ERROR] parse.c - BOOK NOT FOUND 3");
        book_td_parse_finalized = true;
      }
    }
    if (book_td_parse_finalized)
      break;
    book_count += 1;
  }

  *book_array_len = book_count;
  if (*max_pages_in_search == INT_MAX &&
      ((*max_pages_in_search = get_max_pages(match_cursor)) == FAILURE))
    *max_pages_in_search = 1;

  /* post rotines */
  *status = SUCCESS;
  free(buffer);
  return (char *)ecalloc(1, sizeof(char));
}

char *book_page(FILE *page_file, const int file_size,
                struct book_t *selected_book) {
  enum {
    BOOK_DOWNLOAD_LINK,
    BOOK_SERIES,
    BOOK_ISBN,
    BOOK_DESCRIPTION,
    PATTERNS = 4,
  };

  char *match, *buffer_cursor, *close_tag;
  /* all patterns we are looking for in html page */
  static char *pattern[] = {
      "<h2 style=\"text-align:center\">",
      "Series",
      "ISBN",
      "Description",
  };

  char *buffer = (char *)ecalloc(file_size, sizeof(char));

  /* put the formated file in memory */
  rewind(page_file);
  format_file(page_file, buffer);

  buffer_cursor = strstr(buffer, "<body>");

  for (int curr_pattern = 0; curr_pattern < PATTERNS; curr_pattern += 1) {
    if ((match = strstr(buffer_cursor, pattern[curr_pattern]))) {
      if (curr_pattern == BOOK_DOWNLOAD_LINK) {
        match = strstr(match, "href") + 6;
        TEST_STR_PTR(match - 6,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #7"));
        selected_book->download_url =
            (char *)ecalloc(strchr(match, '\"') - match + 1, sizeof(char));
        sscanf(match, "%[^\"]", selected_book->download_url);
      } else if (curr_pattern == BOOK_SERIES) {
        match += 8;
        selected_book->series =
            (char *)ecalloc(strchr(match, '<') - match + 1, sizeof(char));
        sscanf(match, "%[^<]", selected_book->series);
      } else if (curr_pattern == BOOK_ISBN) {
        match += 6;
        selected_book->isbn =
            (char *)ecalloc(strchr(match, '<') - match + 1, sizeof(char));
        sscanf(match, "%[^<]", selected_book->isbn);
      } else if (match) { /* may not have description */
        match = strchr(match, '>') + 1;
        TEST_STR_PTR(match - 1,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #10"));
        close_tag = strstr(match, "</div");
        TEST_STR_PTR(close_tag, error_msg("[ERROR] parse.c - HTTP"));
        *close_tag = '\0';
        selected_book->description =
            (char *)ecalloc(close_tag - match + 1, sizeof(char));
        strcpy(selected_book->description, match);
        match = close_tag + 1;
      }
      buffer_cursor = match + 1;
    } else if (!curr_pattern)
      return error_msg("[ERROR] parse.c - Only part of the page received. Try "
                       "again. \nHTTP REQUEST #6");
  }
  /* post rotines */
  free(buffer);
  return (char *)ecalloc(1, sizeof(char));
}

char *mirror_page(FILE *page_file, const int file_size,
                  struct book_t *selected_book) {
  enum {
    BOOK_VOLUME,
    BOOK_PERIODICAL,
    BOOK_EDITION,
    BOOK_ID,
    BOOK_MIRRORS,
    PATTERNS = 5,
  };

  /* all patterns we are looking for in html page */
  static char *pattern[] = {
      "gray'>Volume:", "gray'>Periodical:", "gray'>Edition:",
      "gray'>ID:",     "gray'>Mirrors:",
  };
  char *buffer = (char *)ecalloc(file_size, sizeof(char));
  char *match, *buffer_cursor = buffer, *close_tag;

  rewind(page_file);
  format_file(page_file, buffer);
  for (int curr_pattern = 0; curr_pattern < PATTERNS; curr_pattern += 1) {
    if ((match = strstr(buffer_cursor, pattern[curr_pattern]))) {
      // buffer_cursor = match + 1;
      if (curr_pattern <= BOOK_ID) {
        char *close_td = strstr(match, "</td");
        match = strstr(match, "<td") + 4;
        TEST_STR_PTR(match,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #14"));
        close_tag = strstr(match, "</td");
        *close_tag = '\0';
        size_t len = close_tag - match;
        if (len < 30) {
          if (curr_pattern == BOOK_ID) {
            selected_book->id = (char *)ecalloc(len + 1, sizeof(char));
            strcpy(selected_book->id, match);
          } else if (curr_pattern == BOOK_PERIODICAL && len) {
            selected_book->periodical = (char *)ecalloc(len + 1, sizeof(char));
            strcpy(selected_book->periodical, match);
          } else if (curr_pattern == BOOK_EDITION && len) {
            selected_book->edition = (char *)ecalloc(len + 1, sizeof(char));
            strcpy(selected_book->edition, match);
          } else if (curr_pattern == BOOK_VOLUME && len) {
            selected_book->volume = (char *)ecalloc(len + 1, sizeof(char));
            strcpy(selected_book->volume, match);
          }
        }
      } else if (curr_pattern == BOOK_MIRRORS) {
        match = strstr(match, "<a href=\"");
        TEST_STR_PTR(match,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #15"));
        match += 9;
        close_tag = strchr(match, '"');
        TEST_STR_PTR(close_tag,
                     error_msg("[ERROR] parse.c - Only part of the page "
                               "received. Try again. \nHTTP REQUEST #16"));
        *close_tag = '\0';
        selected_book->url = (char *)erealloc(
            selected_book->url, (close_tag - match + 1) * sizeof(char));
        strcpy(selected_book->url, match);
      } else
        error_msg("[ERROR] parse.c - Only part of the page received. Try "
                  "again. \nHTTP REQUEST #13");
    }
    buffer_cursor = close_tag + 1;
  }
  free(buffer);
  return (char *)ecalloc(1, sizeof(char));
}

int get_max_pages(const char *buffer) {
  TEST_STR_PTR(buffer, FAILURE);
  char *match_cursor = strstr(buffer, "new Paginator");
  TEST_STR_PTR(match_cursor, FAILURE);
  for (int comma = 0; comma < 3; comma += 1) {
    match_cursor = strchr(match_cursor, ',') + 1;
    TEST_STR_PTR(match_cursor, FAILURE);
  }
  int ans;
  char *close_tag = match_cursor;
  *close_tag = '\0';
  while (*match_cursor != ' ')
    match_cursor -= 1;
  sscanf(match_cursor, "%d", &ans);
  if (verbose)
    printf("Max pages: %d\n", ans);
  return ans;
}

char *strrchr_b(char *s, const int c) {
  while (*s != c)
    s--;
  return s;
}
