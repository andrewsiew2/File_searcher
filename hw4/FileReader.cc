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

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
 #include <fcntl.h>
#include <unistd.h>
#include <errno.h>
extern "C" {
  #include "libhw2/fileparser.h"
}

#include "./HttpUtils.h"
#include "./FileReader.h"

namespace hw4 {

bool FileReader::ReadFile(std::string *str) {
  std::string filepath = basedir_ + "/" + fname_;

  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).
  //
  // You might find ::ReadFile() from HW2's fileparser.h useful
  // here.  Be careful, though; remember that it uses malloc to
  // allocate memory, so you'll need to use free() to free up that
  // memory.  Alternatively, you can use a unique_ptr with a malloc/free
  // deleter to automatically manage this for you; see the comment in
  // HttpUtils.h above the MallocDeleter class for details.

  // MISSING:
  const char* filename = filepath.c_str();
  struct stat filestat;
  char *buf;
  int result, fd;
  ssize_t numread;
  size_t left_to_read;

  // if path is safe
  if (!IsPathSafe(basedir_, filepath)) {
    std::cout << "there is an unsafe path" << std::endl;
    return false;
  }
  // STEP 1.
  // Use the stat system call to fetch a "struct stat" that describes
  // properties of the file. ("man 2 stat"). [You can assume we're on a 64-bit
  // system, with a 64-bit off_t field.]
  if (stat(filename, &filestat) == -1) {
    return NULL;
  }

  // STEP 2.
  // Make sure this is a "regular file" and not a directory
  // or something else.  (use the S_ISREG macro described
  // in "man 2 stat")
  if (!S_ISREG(filestat.st_mode)) {
    return NULL;
  }

  // STEP 3.
  // Attempt to open the file for reading.  (man 2 open)
  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    return NULL;
  }

  // STEP 4.
  // Allocate space for the file, plus 1 extra byte to
  // NULL-terminate the string.
  buf = (char*) malloc(filestat.st_size + 1);
  if (buf == nullptr) {
    return NULL;
  }

  // STEP 5.
  // Read in the file contents.  Use the read system call. (man 2 read.)  Be
  // sure to handle the case that (read returns -1) and errno is (either
  // EAGAIN or EINTR).  Also, note that it is not an error for read to return
  // fewer bytes than what you asked for; you'll need to put read() inside a
  // while loop, looping until you've read to the end of file or a non-
  // recoverable error.  (Read the man page for "read()" carefully, in
  // particular what the return values -1 and 0 imply.)
  left_to_read = filestat.st_size;
  numread = left_to_read;
  while (left_to_read > 0) {
    result = read(fd, buf + (numread - left_to_read), left_to_read); 
    if (result == 0) {
      // EOF
      break;
    } else if (result == -1) {
      if (errno != EAGAIN || errno != EINTR) {
        free(buf);
        return false;
      }
      continue;
    }
    left_to_read -= result;
  }

  // Great, we're done!  We hit the end of the file and we
  // read (filestat.st_size - left_to_read) bytes. Close the
  // file descriptor returned by open(), and return through the
  // "size" output parameter how many bytes we read.
  close(fd);
  int size = (HWSize_t) (filestat.st_size - left_to_read);

  // Add a '\0' after the end of what we read to NULL-terminate
  // the string.
  buf[size] = '\0';
  *str = buf; 
  return true;
}

}  // namespace hw4
