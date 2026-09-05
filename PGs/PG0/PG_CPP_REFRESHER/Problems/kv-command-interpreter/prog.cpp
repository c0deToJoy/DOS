/*
  This is a small program to demonstrate gcov. It
  can take no arguments or 1 argument. It prints
  a message if more than one is given. If the argument's
  value is odd it prints out odd, otherwise even.
  The goal is to show you need multiple test cases
  to cover all of the code.

  -richard.m.veras@ou.edu
*/

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

void printNoKey(ostream &output, char key)
{
  output << "no-key " << key << "\n";
}

void printEntry(ostream &output, char key, const vector<char> &values)
{
  output << "datastore[" << key << "] = {";
  bool first = true;
  for (char value : values)
    {
      if (!first)
        output << ", ";
      output << value;
      first = false;
    }
  output << "}\n";
}

int main(int argc, char *argv[])
{
  if (argc != 3)
    {
      cerr << "error: expected input and output filenames\n";
      return 0;
    }

  ifstream input(argv[1]);
  ofstream output(argv[2]);
  if (!input)
    {
      cerr << "error: unable to open input file '" << argv[1] << "'\n";
      return 0;
    }
  if (!output)
    {
      cerr << "error: unable to open output file '" << argv[2] << "'\n";
      return 0;
    }

  map<char, vector<char>> datastore;
  string command;
  char key;
  char value;

  while (input >> command >> key)
    {
      if (command == "create")
        {
          datastore[key] = {};
          output << "datastore[" << key << "] = {}\n";
          cout << "datastore[" << key << "] = {}\n";
        }
      else if (command == "read")
        {
          auto entry = datastore.find(key);
          if (entry == datastore.end())
            {
              printNoKey(output, key);
              printNoKey(cout, key);
            }
          else
            {
              printEntry(output, key, entry->second);
              printEntry(cout, key, entry->second);
            }
        }
      else if (command == "insert")
        {
          if (!(input >> value))
            {
              cerr << "error: insert requires a key and value\n";
              return 0;
            }
          auto entry = datastore.find(key);
          if (entry == datastore.end())
            {
              printNoKey(output, key);
              printNoKey(cout, key);
            }
          else
            {
              bool alreadyStored = false;
              for (char storedValue : entry->second)
                {
                  if (storedValue == value)
                    alreadyStored = true;
                }
              if (!alreadyStored)
                entry->second.push_back(value);
              printEntry(output, key, entry->second);
              printEntry(cout, key, entry->second);
            }
        }
      else if (command == "delete")
        {
          datastore.erase(key);
          printNoKey(output, key);
          printNoKey(cout, key);
        }
      else
        cerr << "error: unknown command '" << command << "'\n";
    }

  return 0;
}
