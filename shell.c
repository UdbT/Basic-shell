#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Buffer Size Definations
*/
#define SHELL_RL_BUFSIZE 1024
#define SHELL_TOK_BUFSIZE 64

/*
  Delimiters
*/
#define SHELL_TOK_DELIM " \t\r\n\a"

/*
  Function Declarations for shell commands:
*/
void read_file(char *, char *);
void sigint_handler(int, siginfo_t *, void *);
void shell_loop(char *);
char *shell_read_line(void);
char **shell_split_line(char *);
int shell_num_builtins(void);
int shell_execute(char **);
int shell_launch(char **);

/*
  Function Declarations for built-in shell commands:
*/
int shell_cd(char **);
int shell_help(char **);
int shell_exit(char **);

/*
  List of built-in commands, followed by their corresponding functions.
*/
char *builtin_str[] = {"cd", "help", "quit"};

/*
  corresponding functions
*/
int (*builtin_func[]) (char **) = {

  &shell_cd,
  &shell_help,
  &shell_exit
};

/*
  main function
*/
int main(int argc, char **argv) {

  int bufsize = SHELL_RL_BUFSIZE;
  char *buffer = malloc(sizeof(char) * bufsize);

  if (!buffer) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  if(argc > 1) {
    // Load config files, if any.
    read_file(buffer, argv[1]);
  }
  else {
    buffer = NULL;
  }

  // Run command loop.
  shell_loop(buffer);
  free(buffer);

  // Perform any shutdown/cleanup.
  return EXIT_SUCCESS;
}

/*
  to get string from a file
*/
void read_file(char *buffer, char *fname) {

  FILE * fp;
  fp = fopen (fname, "r");
  char c;
  int i = 0;
  while(1) {
    c = fgetc(fp);
    if( c == EOF ) { 
       buffer[i] = '\0';
       break;
    }
    else {
      buffer[i] = c;
      printf("%c", buffer[i]);
      i++;
    }
  }
  fclose(fp);
}

/*
  to handle with incoming signal from Ctrl+c
*/
void sigint_handler(int sig, siginfo_t *siginfo, void *context) {
  
  // wait for all outputs come out
  sleep(0.1);
  printf("\n");

  return;
}

/*
  loop of program
*/
void shell_loop(char *cmd) {
  
  int bufsize = SHELL_RL_BUFSIZE;
  char *line;
  char **args;
  int status;
  struct sigaction act;
  char *cwd = malloc(sizeof(char) * bufsize);

  if(cmd != NULL) {
    // for batch
    args = shell_split_line(cmd); // split line
    status = shell_execute(args); // execute commands
    free(args);

    return;
  }
  else {
    if (!cwd) {
      // Error allocation
      fprintf(stderr, "shell: allocation error\n");
      exit(EXIT_FAILURE);
    }

    // for interactive
    do {
      act.sa_sigaction = &sigint_handler;
      act.sa_flags = SA_SIGINFO;

      // detect Ctrl+c (cannot exit program by pressing Ctrl+c)
      sigaction(SIGINT, &act, NULL);
      
      // display prompt
      printf("\x1b[32m" "\x1b[1m" "shell:" "\x1b[0m");

      // display current directory
      if (getcwd(cwd, bufsize) != NULL) {
        fprintf(stdout, "\x1b[34m" "\x1b[1m" "~%s" "\x1b[0m" "> " , cwd);
      }
      else {
        // Error getting the current directory fails
        fprintf(stdout, "shell: getting the current directory fails\n");

        return;
      }
      
      line = shell_read_line(); // read line
      args = shell_split_line(line); // split line
      status = shell_execute(args); // execute commands
      free(line);
      free(args);
      // check program status
    } while (status);
  }
  free(cwd);
}

/*
  get inputs from user
*/
char *shell_read_line(void) {

  int bufsize = SHELL_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';

      return buffer;
    }
    else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += SHELL_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        // Error allocation
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/*
  split string to tokens:list of commands
*/
char **shell_split_line(char *line) {

  int bufsize = SHELL_TOK_BUFSIZE, position = 0, index = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char **tmpTokens = malloc(bufsize * sizeof(char*));
  char *token;
  int ind;
  int allSpace;

  if (!tokens) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  if (!tmpTokens) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // split a token from the string seperated by semicolon (;)
  token = strtok(line, ";");
  while (token != NULL) {

    ind = 0;
    allSpace = 1;
    while(token[ind] != '\0') {
      if(token[ind] != ' '){
        allSpace = 0;
        break;
      }
      ind++;
    }

    if(!allSpace){
      // collect not-all-space strings
      tmpTokens[index] = token;
      index++;
    }

    if (index >= bufsize) {
      bufsize += SHELL_TOK_BUFSIZE;
      // If we have exceeded the buffer, reallocate.
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        // Error allocation
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ";");
  }
  tmpTokens[index] = NULL;  
  index = 0;

  while(tmpTokens[index] != NULL) {
    // split a token from the string seperated by delimiters
    token = strtok(tmpTokens[index], SHELL_TOK_DELIM);
    while (token != NULL) {
      tokens[position] = token;
      position++;

      if (position >= bufsize) {
        bufsize += SHELL_TOK_BUFSIZE;
        // If we have exceeded the buffer, reallocate.
        tokens = realloc(tokens, bufsize * sizeof(char*));
        if (!tokens) {
          // Error allocation
          fprintf(stderr, "shell: allocation error\n");
          exit(EXIT_FAILURE);
        }
      }

      token = strtok(NULL, SHELL_TOK_DELIM);
    }
    index++;
    tokens[position++] = "\n";
  }
  tokens[position] = NULL;

  return tokens;
}

/*
  return number of built-in commands
*/
int shell_num_builtins() {

  return sizeof(builtin_str) / sizeof(char *);
}

/*
  cd Builtin function implementations.
*/
int shell_cd(char **args) {

  if (args[1] == NULL) {
    // cd with no argument
    int bufsize = SHELL_TOK_BUFSIZE;
    char *buffer = malloc(sizeof(char) * bufsize);
    char *str = "/home/";
    char *tmpstr = malloc(sizeof(char) * (strlen(str)+strlen(buffer)+1));

    if (!buffer) {
      // Error allocation
      fprintf(stderr, "shell: allocation error\n");
      exit(EXIT_FAILURE);
    }

    if (!tmpstr) {
      // Error allocation
      fprintf(stderr, "shell: allocation error\n");
      exit(EXIT_FAILURE);
    }

    if((buffer = getenv("USER")) != NULL) {
      strcat(tmpstr, str);
      strcat(tmpstr, buffer);
      if (chdir(tmpstr) != 0) {
        fprintf(stderr, "shell: chaging directory fails\n");
      }
    }
    else {
      fprintf(stderr, "shell: getting username fails\n");
    }
  }
  else {
    // cd with argument
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "shell: %s: %s: No such file or directory\n", args[0], args[1]);
    }
  }
  
  return 1;
}

/*
  help Builtin function implementations.
*/
int shell_help(char **args) {

  int i;
  printf("SHELL FOR EDUCATION\n");
  printf("The following are built in:\n");

  for (i = 0; i < shell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

/*
  exit Builtin function implementations.
*/
int shell_exit(char **args) {

  // terminate all running processes
  kill(0, SIGTERM);

  return 0;
}

/*
  check commands before launching
*/
int shell_execute(char **args) {

  if (args[0] == NULL) {
    // do nothing with empty command
    return 1;
  }

  //wait for status from executing commands
  return shell_launch(args);
}

/*
  execute splitted commands
*/
int shell_launch(char **args) {

  int bufsize = SHELL_TOK_BUFSIZE;
  pid_t pid, wpid;
  pid_t *pids = malloc(bufsize * sizeof(int));
  char **tmpArgs = malloc(bufsize * sizeof(char*));
  int status;
  int retVal;
  int isBuiltin = 0;
  int i = 0;
  int j = 0;
  int pidind = 0;

  if (!pids) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  if (!tmpArgs) {
    // Error allocation
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while(args[i] != NULL) {
    // seperate each command by token "\n"
    if(args[i] != "\n") {
      tmpArgs[j++] = args[i++];
    }
    else {
      tmpArgs[j] = NULL;
      // check whether a command is built-in
      isBuiltin = 0;
      for (int i = 0; i < shell_num_builtins(); i++) {
        if (strcmp(tmpArgs[0], builtin_str[i]) == 0) {
          // execute built-in command
          retVal = (*builtin_func[i])(tmpArgs);
          if(retVal == 0) {

            return retVal;
          }
          isBuiltin = 1;
        }
      }

      // execute others
      if(!isBuiltin) {
        pid = fork();
        pids[pidind++] = pid;
        if (pid == 0) {
          // Child process
          if (execvp(tmpArgs[0], tmpArgs) == -1) {
            // Error command not found
            fprintf(stderr, "%s: command not found\n", tmpArgs[0]);
          }
          exit(EXIT_FAILURE);
        } 
        else if (pid < 0) {
          // Error forking
          fprintf(stderr, "shell: Forking fails\n");
        } 
        else {
          // Parent process	

        }
      }
      
      free(tmpArgs);
      tmpArgs = malloc(bufsize * sizeof(char*));

      if (!tmpArgs) {
        // Error allocation
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }

      i++;
      j = 0;
    }
  }

  // wait for all child processes
  for(int i = 0; i < pidind; i++) {
    waitpid(pids[i], &status, 0);
  }
  free(pids);

  return 1;
}
