#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a;"

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_launch(char **);
int lsh_execute(char **);
/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **);
int lsh_help(char **);
int lsh_exit(char **);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0, index = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char **tmpTokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ";");
  while (token != NULL) {
    tmpTokens[index] = token;
    index++;

    if (index >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, " ;");
  }
  tmpTokens[index] = NULL;
  
  index = 0;

  while(tmpTokens[index] != NULL) {

    token = strtok(tmpTokens[index], LSH_TOK_DELIM);
    while (token != NULL) {
      tokens[position] = token;
      position++;

      if (position >= bufsize) {
        bufsize += LSH_TOK_BUFSIZE;
        tokens = realloc(tokens, bufsize * sizeof(char*));
        if (!tokens) {
          fprintf(stderr, "lsh: allocation error\n");
          exit(EXIT_FAILURE);
        }
      }

      token = strtok(NULL, LSH_TOK_DELIM);
    }
    index++;
    tokens[position++] = "\n";
  }
  tokens[position] = NULL;
/*////////////////////////////////////////////
  int i = 0;
  while(tokens[i] != NULL) {
  	printf("tok = %s\n", tokens[i++]);
  }
////////////////////////////////////////////*/
  return tokens;
}

int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;
  
  int bufsize = LSH_TOK_BUFSIZE;
  char **tmpArgs = malloc(bufsize * sizeof(char*));

  int i = 0;
  int j = 0;
  while(args[i] != NULL) {
    
    if(args[i] != "\n") {
      printf("args[%d] = %s\n", i, args[i]);
      tmpArgs[j++] = args[i++];
    }
    else {
      tmpArgs[j] = NULL;
      pid = fork();
      if (pid == 0) {
        // Child process
        if (execvp(tmpArgs[0], tmpArgs) == -1) {
          perror("lsh");
        }
        exit(EXIT_FAILURE);
      } 
      else if (pid < 0) {
        // Error forking
        perror("lsh");
      } 
      else {
        // Parent process
        do {
          wpid = waitpid(pid, &status, WUNTRACED);
        } 
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
      }
      //////////printf("Finish\n");
      
      free(tmpArgs);
      tmpArgs = malloc(bufsize * sizeof(char*));
      i++;
      j = 0;
    }
  }

  return 1;
}

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
  int i = 0;
  while(args[i] != NULL){printf("AAA: %s", args[i++]); }

  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("AAA lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  /*for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }*/

  return lsh_launch(args);
}

