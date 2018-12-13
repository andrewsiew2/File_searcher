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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using namespace std;
using namespace boost;
namespace hw4 {

bool HttpConnection::GetNextRequest(HttpRequest *request) {
  // Use "WrappedRead" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use ParseRequest()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes GetNextRequest()!

  // MISSING:
  // Assumes \r\n\r\n not already in buffer_
  size_t markerPos = buffer_.find("\r\n\r\n");
  if (markerPos == string::npos) {
    while(1) {

      unsigned char buf[1080];
      int bytes_read = WrappedRead(fd_, buf, 1080);
      // EOF or connection cut
      if(bytes_read == 0) {
        break;
      } else if(bytes_read == -1) {
        // fatal error
        return false;
      } else {
        buffer_ += string((char*) buf);
        markerPos = buffer_.find("\r\n\r\n");
        // if it is found
        if(markerPos != string::npos) {
          break;
        }
      }
    }
  }
  // adds 4 for the \r\n\r\n
  *request = ParseRequest(markerPos + 4); 
  buffer_ = buffer_.substr(markerPos + 4);
  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse &response) {
  std::string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         (unsigned char *) str.c_str(),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(size_t end) {
  HttpRequest req;
  req.URI = "/";  // by default, get "/".

  // Get the header.
  std::string str = buffer_.substr(0, end);

  // Split the header into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers (i.e., req.headers[headername] = headervalue).
  // You should look at HttpResponse.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.

  // MISSING:
  // Process Header in form [http_protocol] [response_code] [message]\r\n
  
  vector<string> allLines;
  vector<string> firstLine;
  split(allLines, str, is_any_of("\r\n"), token_compress_on);
  split(firstLine, allLines[0], is_any_of(" "), token_compress_on);
  req.URI = firstLine[1];
  for(size_t i = 1; i < allLines.size() - 1; i++) {
    trim(allLines[i]);
    int pos = allLines[i].find(": ");
    
    string headername = allLines[i].substr(0, pos);
    to_lower(headername);
    // adds 2 for the " :"
    req.headers[headername] = allLines[i].substr(pos + 2);
  }
  return req;
}

}  // namespace hw4
