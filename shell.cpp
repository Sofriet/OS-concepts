/**
	* Shell framework
	* course Operating Systems
	* Radboud University
	* v22.09.05
	Student names:
	- Sofie Vos
	- Philipp Küppers
*/

/**
 * Hint: in most IDEs (Visual Studio Code, Qt Creator, neovim) you can:
 * - Control-click on a function name to go to the definition
 * - Ctrl-space to auto complete functions and variables
 */

// function/class definitions you are going to use
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <list>
#include <optional>

// although it is good habit, you don't have to type 'std' before many objects by including this line
using namespace std;

struct Command {
  vector<string> parts = {};
};

struct Expression {
  vector<Command> commands; // An expression is a chain of commands, e.g. 'ls | wc -l' gets split into 'ls' and 'wc -l'
  string inputFromFile; // if empty input is from keyboard
  string outputToFile; // if empty output goes to screen
  bool background = false; // indicate whether the user entered &
};

// Parses a string to form a vector of arguments. The separator is a space char (' ').
vector<string> split_string(const string& str, char delimiter = ' ') {
  vector<string> retval;
  for (size_t pos = 0; pos < str.length(); ) {
    // look for the next space
    size_t found = str.find(delimiter, pos);
    // if no space was found, this is the last word
    if (found == string::npos) {
      retval.push_back(str.substr(pos));
      break;
    }
    // filter out consequetive spaces
    if (found != pos)
      retval.push_back(str.substr(pos, found-pos));
    pos = found+1;
  }
  return retval;
}

// wrapper around the C execvp so it can be called with C++ strings (easier to work with)
// always start with the command itself
// DO NOT CHANGE THIS FUNCTION UNDER ANY CIRCUMSTANCE
int execvp(const vector<string>& args) {
  // build argument list
  const char** c_args = new const char*[args.size()+1];
  for (size_t i = 0; i < args.size(); ++i) {
    c_args[i] = args[i].c_str();
  }
  c_args[args.size()] = nullptr;
  // replace current process with new process as specified
  int rc = ::execvp(c_args[0], const_cast<char**>(c_args));
  // if we got this far, there must be an error
  int error = errno;
  // in case of failure, clean up memory (this won't overwrite errno normally, but let's be sure)
  delete[] c_args;
  errno = error;
  return rc;
}

// Executes a command with arguments. In case of failure, returns error code.
int execute_command(const Command& cmd) {
  auto& parts = cmd.parts;

  if (parts.size() == 0)
    return EINVAL;

  // execute external commands
  int retval = execvp(parts);
  // When the user enters some invalid command
  if (retval == -1) {
    cout << strerror(errno) << endl;
  }
  return retval;
}

void display_prompt() {
  char buffer[512];
  char* dir = getcwd(buffer, sizeof(buffer));
  if (dir) {
    cout << "\e[32m" << dir << "\e[39m"; // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
  }
  cout << "$ ";
  flush(cout);
}

string request_command_line(bool showPrompt) {
  if (showPrompt) {
    display_prompt();
  }
  string retval;
  getline(cin, retval);
  return retval;
}

// note: For such a simple shell, there is little need for a full-blown parser (as in an LL or LR capable parser).
// Here, the user input can be parsed using the following approach.
// First, divide the input into the distinct commands (as they can be chained, separated by `|`).
// Next, these commands are parsed separately. The first command is checked for the `<` operator, and the last command for the `>` operator.
Expression parse_command_line(string commandLine) {
  Expression expression;
  vector<string> commands = split_string(commandLine, '|');
  for (size_t i = 0; i < commands.size(); ++i) {
    string& line = commands[i];
    vector<string> args = split_string(line, ' ');
    if (i == commands.size() - 1 && args.size() > 1 && args[args.size()-1] == "&") {
      expression.background = true;
      args.resize(args.size()-1);
    }
    if (i == commands.size() - 1 && args.size() > 2 && args[args.size()-2] == ">") {
      expression.outputToFile = args[args.size()-1];
      args.resize(args.size()-2);
    }
    if (i == 0 && args.size() > 2 && args[args.size()-2] == "<") {
      expression.inputFromFile = args[args.size()-1];
      args.resize(args.size()-2);
    }
    expression.commands.push_back({args});
  }
  return expression;
}


int execute_expression(Expression& expression) {

  // first check if the expression includes exit or cd
  for (long unsigned int i = 0; i < expression.commands.size(); i++){
    if (expression.commands[i].parts[0] == "exit") {
      exit(EXIT_SUCCESS);
      cout << "exit failed" << endl;
      return 0;
    }

    if (expression.commands[i].parts[0] == "cd" && expression.commands[i].parts.size() == 2) {
      if( chdir(expression.commands[i].parts[1].c_str()) == -1 ){ 
        cout << "error chdir" << endl;      
        return 0;
      }
    }  
  }

  // Check for empty expression
  if (expression.commands.size() == 0) {
    cout << "No input given" << endl;
    return 0;
  }

  // we use this variable to connect two pipes, to the output from the previous pipe becomes the new input of the next pipe
  int new_input = STDIN_FILENO;
  vector<pid_t> cmd_children(expression.commands.size()); //store the children to wait for them later

  for (long unsigned int i = 0; i < expression.commands.size(); i++) {
    Command curr_cmd = expression.commands[i];
    int fd[2];
    int input = new_input; //input for the first command, will be fd[0] for next
    int output;

    // for first n-1 iterations we make a new pipe
    if (i != expression.commands.size() - 1 ) {
      if (pipe(fd) == -1){
        cout << "Pipe failed" << endl;
        return 1;
      } 
      new_input = fd[0];
      output = fd[1];
    } else {
      output = STDOUT_FILENO; //output of last command shown and not piped
    }

    pid_t child = fork();
    cmd_children[i] = child;
    if (child == 0) {

      // if first command
      if(i == 0){
        if(expression.background) {
          close(STDIN_FILENO);
        }
        if(expression.inputFromFile != ""){
          input = open(expression.inputFromFile.c_str(), O_RDONLY);
          if (input == -1){
            cout << "file could not be opened" << endl;
            abort();
          }
        }
      } 
      // if last command
      if (i == expression.commands.size() - 1){
        if(expression.outputToFile != ""){
          output = open(expression.outputToFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
          if (output == -1){
            cout << "file could not be opened" << endl;
            abort();
          }
        }
      }
      dup2(output, STDIN_FILENO);
      dup2(input, STDOUT_FILENO);
      execute_command(curr_cmd);
      abort();
    } else {
      // If parent, close pipes
      if(input != STDIN_FILENO) {
          close(input);
      }
      if(output != STDOUT_FILENO) {
          close(output);
      }
    }
    // Wait for child processes to finish
    if (expression.background == false){
      for (const auto &item: cmd_children) {
        waitpid(item, nullptr, 0);
      }
    }
  }
  return 0;
}

// framework for executing "date | tail -c 5" using raw commands
// two processes are created, and connected to each other
int step1(bool showPrompt, Command cmd1, Command cmd2) {
  //create communication channel shared between the two processes
  int fd[2];
  if (pipe(fd) == -1){
    return 1;
  } //make pipe

  pid_t child1 = fork();
  if (child1 == 0) {
    // redirect standard output (STDOUT_FILENO) to the input of the shared communication channel
    // free non used resources 
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    execute_command(cmd1);
    // display nice warning that the executable could not be found
    abort(); // if the executable is not found, we should abort. 
  }

  pid_t child2 = fork(); 
  if (child2 == 0) {
    // redirect the output of the shared communication channel to the standard input (STDIN_FILENO).
    // free non used resources
    dup2(fd[0], STDIN_FILENO);
    close(fd[1]);
    execute_command(cmd2);
    abort(); // if the executable is not found, we should abort. 
  }
  // free non used resources 
  // wait on child processes to finish 
  close(fd[0]);
  close(fd[1]);

  waitpid(child1, nullptr, 0);
  waitpid(child2, nullptr, 0);
  return 0;
}

int shell(bool showPrompt) {
  //* <- remove one '/' in front of the other '/' to switch from the normal code to step1 code
  while (cin.good()) {
     string commandLine = request_command_line(showPrompt);
     Expression expression = parse_command_line(commandLine);
     int rc = execute_expression(expression);
     if (rc != 0)
       cerr << strerror(rc) << endl;
  }
  // cout << ":'(" << endl;
  return 0;
  // return step1(showPrompt);
}
