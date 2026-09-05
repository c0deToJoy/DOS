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
#include <deque>
#include <fstream>
#include <string>

using namespace std;

void printState(ostream &output, int lineNumber, const deque<char> &values,
                char pending, bool pendingAtFront, bool emptyRemoval)
{
  output << lineNumber << ": " << values.size() << " |";

  if (emptyRemoval)
    output << " ()^";
  else
    {
      if (pending != '\0' && pendingAtFront)
        output << " " << pending << "^";

      for (char value : values)
        output << " " << value;

      if (pending != '\0' && !pendingAtFront)
        output << " " << pending << "^";
    }

  output << "\n";
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

  string specification;
  int capacity;
  if (!(input >> specification >> capacity) || specification != "spec")
    {
      cerr << "error: invalid list specification\n";
      return 0;
    }

  deque<char> values;
  printState(output, 0, values, '\0', false, false);
  printState(cout, 0, values, '\0', false, false);

  string command;
  int lineNumber = 0;
  while (input >> command)
    {
      ++lineNumber;
      char value;
      char pending = '\0';
      bool pendingAtFront = false;
      bool emptyRemoval = false;

      if (command == "insert-front")
        {
          if (!(input >> value))
            {
              cerr << "error: insert-front requires a character\n";
              return 0;
            }
          if (capacity >= 0 && values.size() >= static_cast<size_t>(capacity))
            {
              pending = value;
              pendingAtFront = true;
            }
          else
            values.push_front(value);
        }
      else if (command == "insert-back")
        {
          if (!(input >> value))
            {
              cerr << "error: insert-back requires a character\n";
              return 0;
            }
          if (capacity >= 0 && values.size() >= static_cast<size_t>(capacity))
            pending = value;
          else
            values.push_back(value);
        }
      else if (command == "remove-front")
        {
          if (values.empty())
            emptyRemoval = true;
          else
            values.pop_front();
        }
      else if (command == "remove-back")
        {
          if (values.empty())
            emptyRemoval = true;
          else
            values.pop_back();
        }
      else
        {
          string ignoredArguments;
          getline(input, ignoredArguments);
          cerr << "error: unknown command '" << command << "'\n";
        }

      printState(output, lineNumber, values, pending, pendingAtFront,
                 emptyRemoval);
      printState(cout, lineNumber, values, pending, pendingAtFront,
                 emptyRemoval);
    }
  
  return 0;
}
