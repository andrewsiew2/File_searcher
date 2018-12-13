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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;
using namespace boost;
using namespace std;
namespace hw4 {

// This is the function that threads are dispatched into
// in order to process new client connections.
void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices);

// Process a file request.
HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir);

// Process a query request.
HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices);

bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!ss_.BindAndListen(AF_INET6, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->basedir = staticfile_dirpath_;
    hst->indices = &indices_;
    if (!ss_.Accept(&hst->client_fd,
                    &hst->caddr,
                    &hst->cport,
                    &hst->cdns,
                    &hst->saddr,
                    &hst->sdns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  std::unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->cdns << ":" << hst->cport << " "
       << "(IP address " << hst->caddr << ")" << " connected." << endl;

  bool done = false;

  HttpConnection connection(hst->client_fd);
  while (!done) {
    // Use the HttpConnection class to read in the next request from
    // this client, process it by invoking ProcessRequest(), and then
    // use the HttpConnection class to write the response.  If the
    // client sent a "Connection: close\r\n" header, then shut down
    // the connection.

    // MISSING:
    HttpRequest request;
    if(!connection.GetNextRequest(&request)) {
      // no more requests
      close(hst->client_fd);
      done = true;
    }
    
    HttpResponse response = ProcessRequest(request, 
                  hst->basedir, hst->indices);
    
    // write it 
    if(!connection.WriteResponse(response)) {
      // no more requests
      close(hst->client_fd);
      done = true;
    }

    // checks if client want to close
    if(request.headers["connection"] == "close") {
      close(hst->client_fd);
      done = true;
    }  
  }
}

HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices) {
  // Is the user asking for a static file?
  if (req.URI.substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.URI, basedir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.URI, indices);
}


HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for.
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  std::string fname = "";

  // MISSING:
  URLParser parser;
  parser.Parse(uri);
  fname += parser.get_path();
  fname = fname.substr(8);
  FileReader reader(basedir, fname);
  if(reader.ReadFile(&ret.body)) {
    size_t pos = fname.rfind(".");
    string fileType = fname.substr(pos);
    if(fileType == ".htm" || fileType == ".html") {
      ret.headers["Content-type"] = "text/html";
    }else if(fileType == ".jpeg" || fileType == ".jpg") {
      ret.headers["Content-type"] = "image/jpeg";
    }else if(fileType == ".png") {
      ret.headers["Content-type"] = "text/png";
    }

    ret.protocol = "HTTP/1.1";
    ret.response_code = 200;
    ret.message = "OK"; 
    return ret;
  }


  // If you couldn't find the file, return an HTTP 404 error.
  ret.protocol = "HTTP/1.1";
  ret.response_code = 404;
  ret.message = "Not Found";
  ret.body = "<html><body>Couldn't find file \"";
  ret.body +=  EscapeHTML(fname);
  ret.body += "\"</body></html>";
  return ret;
}

HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 333gle logo and the
  //    search box/button
  //
  //  - if the user had previously typed in a search query, you also
  //    need to display the search results.
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - you'll want to create and use a hw3::QueryProcessor to process
  //    the query against the search indices
  //
  //  - in your generated search results, see if you can figure out
  //    how to hyperlink results to the file contents, like we did
  //    in our solution_binaries/http333d.

  // MISSING:
  ret.body = "<html>";
  ret.body += "<head>";
  ret.body += "<title>333gle</title>";
  ret.body += "</head>";
  ret.body += "<body>";
  ret.body += "<center style=\"font-size:500%;\">";
  ret.body += "<span style=\"position:relative;bottom:";
  ret.body += "-0.33em;color:orange;\">3</span>";
  ret.body += "<span style=\"color:red;\">3</span>";
  ret.body += "<span style=\"color:gold;\">3</span>";
  ret.body += "<span style=\"color:blue;\">g</span>";
  ret.body += "<span style=\"color:green;\">l</span>";
  ret.body += "<span style=\"color:red;\">e</span>";
  ret.body += "</center>";
  ret.body += "<p></p>";
  ret.body += "<div style=\"height:20px;\"></div>";
  ret.body += "<center>";
  ret.body += "<form action=\"/query\" method=\"get\">";
  ret.body += "<input size=\"30\" name=\"terms\" type=\"text\">";
  ret.body += "<input value=\"Search\" type=\"submit\">";
  ret.body += "</form>";
  ret.body += "</center>";
  
  ret.body += "<p></p>";

  URLParser parser;
  parser.Parse(uri);
  string query = parser.get_args()["terms"];
  trim(query);
  to_lower(query);
  if((int) uri.find("=") != -1) {

    vector<string> allQuerys;
    split(allQuerys, query, is_any_of(" "), token_compress_on);

    hw3::QueryProcessor processer(*indices, true);
    vector<hw3::QueryProcessor::QueryResult> result = 
                            processer.ProcessQuery(allQuerys);

    // no fesults found
    if(result.size() != 0) {
      ret.body += "<p>";
      ret.body += "<br>";
      ret.body += to_string(result.size());
      ret.body += " results found for <b>";
      ret.body += EscapeHTML(query);
      ret.body += "</b>";
      ret.body += "</p>";
      ret.body += "<ul>";
      for(size_t i = 0; i < result.size(); i++) {
        ret.body += "<li><a href=\"";
        if(result[i].document_name.substr(0,4) != "http") {
          ret.body += "/static/";
        }
        ret.body += result[i].document_name;
        ret.body += "\">";
        ret.body += EscapeHTML(result[i].document_name);
        ret.body += "</a>";
        ret.body += " [";
        ret.body += to_string(result[i].rank);
        ret.body += "]";
        ret.body += "</li>";
      }
      ret.body += "</ul>";

    }else {
      ret.body += "<p>";
      ret.body += "<br>No results found for <b>";
      ret.body += EscapeHTML(query);
      ret.body += "</b>";
      ret.body += "</p>";
    }
  }
  
  

  ret.body += "</body>";
  ret.body += "</html>";
  ret.protocol = "HTTP/1.1";
  ret.response_code = 200;
  ret.message = "OK";
  return ret;
}

}  // namespace hw4
