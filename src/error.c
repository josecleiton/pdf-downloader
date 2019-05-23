#include "error.h"

char* error_msg(const char* msg) {
   size_t result_size = 128 + strlen(msg);
   char* result = (char*) malloc(result_size * sizeof(char));
   if(result == NULL) {
      exit_and_report();
   }
   *result = '\0';
   sprintf(result, "%s\n%s", msg, strerror(errno));
   return result;
}

void exit_and_report(void) {
   fprintf(stderr, "%s", strerror(errno));
   exit(EXIT_FAILURE);
}
