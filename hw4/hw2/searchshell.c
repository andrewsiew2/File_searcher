/*
 * Copyright ©2018 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Summer Quarter 2018 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

static void Usage(void);
int INPUT_SIZE = 1024;
int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.

  //  Step 1
  DocTable doctable = AllocateDocTable();
  MemIndex index = AllocateMemIndex();
  char *str1;
  char **saveptr = malloc(INPUT_SIZE);
  LinkedList result;
  SearchResult* res;
  printf("Indexing \'%s\'\n", argv[1]);
  if (CrawlFileTree(argv[1], &doctable, &index) == 0) {
    printf("Unable to crawl file\n");
  }
  char input[INPUT_SIZE];
  while(1) {  // Program begins
    // Step 2
    printf("enter query:\n");

    if(fgets(input, INPUT_SIZE, stdin) != NULL) {
      char** query =  malloc(INPUT_SIZE);
      if(query == NULL) {
        printf("Out of Memory\n");
        return EXIT_FAILURE;
      }
      int count = 0;
      // Step 3
      str1 = input;
      while(1) {  // runs until there are no more words

        char* word = strtok_r(str1, " ", saveptr);
        if(word == NULL) {
          break;
        }
        query[count] = word;
        count++;
        str1 = NULL;
      }
      // remove the \n a d replace with \0
      char* temp = strchr(query[count - 1], '\n');
      *temp = '\0';
      result = MIProcessQuery(index, query, count);  // runs query
      if(result != NULL) {
        LLIter iter = LLMakeIterator(result, 0);
        if(iter == NULL) {
          printf("Out of Memory\n");
          return EXIT_FAILURE;
        }
        // Step 4
        if(NumElementsInLinkedList(result) > 0) {
          LLIteratorGetPayload(iter, (void**) &res);
          printf("  %s (%i)\n", DTLookupDocID(doctable, res->docid), res->rank);
          while(LLIteratorHasNext(iter)) {
            LLIteratorNext(iter);
            LLIteratorGetPayload(iter, (void**) &res);
            printf("  %s (%i)\n", DTLookupDocID(doctable, res->docid), res->rank);
          }
        }
        LLIteratorFree(iter);
      }
      free(query);
    }
  }
  FreeMemIndex(index);
  FreeDocTable(doctable);
  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

