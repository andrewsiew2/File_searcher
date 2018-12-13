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

#include <iostream>
#include <algorithm>
#include <iterator>
#include <map>
#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}
using namespace std;
namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

static void insertQuery(map<string, int> &data, 
                        list<docid_element_header> &matches, 
                        DocTableReader* dtr) {
  list<docid_element_header>::iterator k;
  string filename;
  for (k = matches.begin(); k != matches.end(); k++) {
    Verify333(dtr->LookupDocID(k->docid, &filename));
    // if in the map already
    if (data.find(filename) == data.end()) {
      data.insert(pair <string, int>(filename, k->num_positions));
    } else {
      (data.find(filename))->second += k->num_positions;

    }
  }
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  // MISSING:
  map<string, int> data;
  map<string, list<string>> otherData;
  // do for first query
  for (HWSize_t i = 0; i < arraylen_; i++) {
    DocTableReader* dtr = dtr_array_[i];
    IndexTableReader* itr = itr_array_[i];
    DocIDTableReader* ditr = itr->LookupWord(query[0]);
    if (ditr == nullptr) {
      continue;
    }
    list<docid_element_header> matches = ditr->GetDocIDList();
    delete ditr;
    insertQuery(data, matches, dtr);
  }

  // collects data from other queries
  for (HWSize_t j = 1; j < query.size(); j++) {
    list<string> someList;
    otherData.insert(pair <string, list<string>>(query[j], someList));
    for (HWSize_t i = 0; i < arraylen_; i++) {
      DocTableReader* dtr = dtr_array_[i];
      IndexTableReader* itr = itr_array_[i];
      DocIDTableReader* ditr = itr->LookupWord(query[j]);
      if (ditr == nullptr) {
        continue;
      }
      list<docid_element_header> matches = ditr->GetDocIDList();
      list<docid_element_header> :: iterator filter;
      string name;
      for (filter = matches.begin(); filter != matches.end(); filter++) {
        Verify333(dtr->LookupDocID(filter->docid, &name));
        ((otherData.find(query[j]))->second).push_back(name);
      }
    }
  }
  map<string, int> filteredData;
  // take intersection of all data
  map<string, list<string>> :: iterator mapItr;
  for (mapItr = otherData.begin(); mapItr != otherData.end(); mapItr++) {\
    // iterate over otherMaps
    list<string> :: iterator listChecker;
    list<string> innerList = mapItr->second;
    for (listChecker = innerList.begin(); listChecker != innerList.end(); listChecker++) {
      if (data.find(*listChecker) != data.end()) {
        filteredData.insert(pair <string, int>(*listChecker, (data.find(*listChecker))->second));
      }
    }
    data = filteredData;
    map<string, int> newData;
    filteredData = newData;   
  }
  

  
  for (HWSize_t i = 0; i < arraylen_; i++) {
    for (HWSize_t j = 1; j < query.size(); j++) {
      DocTableReader* dtr = dtr_array_[i];
      IndexTableReader* itr = itr_array_[i];
      // MUST DELETE THIS WHEN DONE
      DocIDTableReader* ditr = itr->LookupWord(query[j]);
      if (ditr == nullptr) {
        continue;
      }

      list<docid_element_header> matches = ditr->GetDocIDList();
      list<docid_element_header> filteredMatches;
      delete ditr;

      // filter matches
      list<docid_element_header> :: iterator filter;
      string name;
      for (filter = matches.begin(); filter != matches.end(); filter++) {
        Verify333(dtr->LookupDocID(filter->docid, &name));
        if (data.find(name) != data.end()) {
          docid_element_header temp;
          temp.docid = filter->docid;
          temp.num_positions = filter->num_positions;
          filteredMatches.push_front(temp);
        }
      }
      insertQuery(data, filteredMatches, dtr);
    }
  }






  // insert data from map to final result
  map <string, int> :: iterator itr;
  for (itr = data.begin(); itr != data.end(); itr++) {
    QueryResult qr;
    qr.document_name = itr->first;
    qr.rank = itr->second;
    finalresult.push_back(qr);
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
