/*!
 * @author  Shrey Patel
 * @date    january 16, 2020
 * @brief   This is a Program 
 *          
 */

//Includes Declaration
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

//Constants Declaration
#define CONFIG_FILE_NAME ".SEEshrc"
#define PROMPT "?"
#define HOME "HOME"
#define MAX_PROMPT 30
#define BUFFFER_SIZE 1024
#define TOKKEN_BUFFER_SIZE 64
#define MAXLIST 100
#define TOKKEN_DELIM " \t\r\n\a="
#define INPUT_TYPE_FILE 'F'
#define INPUT_TYPE_USER 'U'

//Declare supported shell commands
bool change_dir_cmd(char **args);
bool print_working_dir_cmd(char **args);
bool set_env_var_cmd(char **args);
bool unset_env_var_cmd(char ** args);
bool help_cmd(char **args);
bool exit_cmd(char **args);

//Declare supported cmds and their assigned function calls
char *supported_cmds[] = {
    "cd",
    "pwd",
    "set",
    "unset",
    "help",
    "exit"
};

bool (*supported_cmds_func[]) (char **) = {
    &change_dir_cmd,
    &print_working_dir_cmd,
    &set_env_var_cmd,
    &unset_env_var_cmd,
    &help_cmd,
    &exit_cmd
};

int supported_cmds_len() {
    return sizeof(supported_cmds) / sizeof(char *);
}

//This function is used to print output of SEEsh to a file
void print_to_file(char *output) {
    FILE *fp;
    fp = fopen("Output.txt", "a");// "w" means that we are going to write on this file
    fprintf(fp, "%s\n", output);
    fclose(fp); //Don't forget to close the file when finished
    return;
}

//optional features, if want to print in file then turn this on and
//after it goes to interactive mode it automatically turns it off,
//so that in ittteractive mode it will print all the information in the standard output
volatile bool mode_to_print_in_file = false;

//Internal command function to change the directory
bool change_dir_cmd(char **args){
    if (args[1] == NULL)
        args[1] = getenv(HOME);
    if (chdir(args[1]) != 0) {
        perror("Error");
    }
    return true;
}

//Internal command function to print current working directory
bool print_working_dir_cmd(char **args){
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    
    if (mode_to_print_in_file)
        print_to_file(cwd);
    else
        printf("%s\n", cwd);
    return true;
}

//Internal command function to set env variable
bool set_env_var_cmd(char **args){
    if(args[1] == NULL || args[2] == NULL)
        fprintf(stderr, "Command Error: expected argument to \"set\".\n");
    else {
        bool status = (setenv(args[1], args[2], 1) == 0) ? true : false;
        if (!status)
            fprintf(stderr, "Command Execution Error: Something went wrong.\n");
    }
    return true;
}

//Internal command function to unset env variable
bool unset_env_var_cmd(char ** args){
    if(args[1] == NULL)
        fprintf(stderr, "Command Error: expected argument to \"set\".\n");
    else {
        bool status = (unsetenv(args[1]) == 0) ? true : false;
        if (!status)
            fprintf(stderr, "Command Execution Error: Something went wrong.\n");
    }
    return true;
}

//Internal command function to help
bool help_cmd(char **args){
    int i;
    printf("SEEsh Help:\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < supported_cmds_len(); i++) {
        printf("  %s\n", supported_cmds[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return true;
}

//Internal command function to exit
bool exit_cmd(char **args){
    return false;
}

// This function will read input from the file as well as from the standard input
char *read_input(char inputType, FILE* file){
    int position = 0, bufferSize = BUFFFER_SIZE;
    char *buffer = malloc(sizeof(char) * bufferSize);
    char c;

    if(!buffer) {
        fprintf(stderr, "Internal Error: Memory allocation failed while reading input.\n");
        exit(EXIT_FAILURE);
    }
    
    for(;;){
        
        if(inputType == INPUT_TYPE_FILE)
            c = getc(file);
        else
            c = getchar(); //getc(File)

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        //In case the size of input exceeds the buffer.
        if (position >= bufferSize) {
            bufferSize += BUFFFER_SIZE;
            buffer = realloc(buffer, bufferSize);
            if (!buffer) {
                fprintf(stderr, "Internal Error: Memory allocation failed while reading input.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **split_cmd_to_args(char *input){
    int position = 0, bufferSize = TOKKEN_BUFFER_SIZE;
    char **args = malloc(bufferSize * sizeof(char*));
    char *arg;

    if(!args){
        fprintf(stderr, "Internal Error: Memory allocation failed while reading input.\n");
        exit(EXIT_FAILURE);
    }

    arg = strtok(input, TOKKEN_DELIM);
    while (arg != NULL) {
        args[position] = arg;
        position++;

        if (position >= bufferSize) {
            bufferSize += TOKKEN_BUFFER_SIZE;
            args = realloc(args, bufferSize * sizeof(char*));
            if (!args) {
                fprintf(stderr, "Internal Error: Memory allocation failed while reading input.\n");
                exit(EXIT_FAILURE);
            }
        }

        arg = strtok(NULL, TOKKEN_DELIM);
    }

    args[position] = NULL;
    return args;
}

bool execute_internal_cmd(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
        perror("Error");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
        // Error forking
        perror("Error");
  } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return true;
}

bool execute_cmd(char **args) {
    if(args[0] == NULL) {
        return true;
    }

    // //Check and execute if the entered command is supported
    for (int i = 0; i < supported_cmds_len(); i++) {
        if (strcmp(args[0], supported_cmds[i]) == 0) {
            return (*supported_cmds_func[i])(args);
        }
    }

    // To use internal command
    return execute_internal_cmd(args);
}

bool execute_piping_cmd(char *input) {
    
    //This function can be further used for piping
    return true;
}

bool load_and_setup_configurations() {

    char *input;
    char **args;
    bool status = false;

    char filePath[500] = "";
    strcat(filePath, getenv(HOME));
    strcat(filePath, "/");
    strcat(filePath, CONFIG_FILE_NAME);

    FILE* file = fopen(filePath, "r");

    if (!file) {
        fprintf(stderr, "Configuration Error: Configuration file open error.\n");
        return status;
    }

    do {
        input = read_input(INPUT_TYPE_FILE, file);
        if(strcmp(input, "") == 0)
            return status;
        
        printf("%s ", PROMPT);
        printf("%s\n", input);
        
        if (strstr(input, "|") != NULL) //This condition is just to check if we have piping
            status = execute_piping_cmd(input);
        else {
            args = split_cmd_to_args(input);
            status = execute_cmd(args);
        }

        free(input);
        free(args);
    } while(status);
    
    
    fclose(file);
    return status;
}

void main_loop(){
    char *input;
    char **args;
    FILE* file = NULL;
    bool status;

    do {
        printf("%s ", PROMPT);
        input = read_input(INPUT_TYPE_USER, file);
        
        if (strstr(input, "|") != NULL) //This condition is just to check if we have piping
            status = execute_piping_cmd(input);
        else {
            args = split_cmd_to_args(input);
            status = execute_cmd(args);
        }

        free(input);
        free(args);
    } while(status);
}

int main(int argc, char **argv){
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    bool status = load_and_setup_configurations();
    mode_to_print_in_file = false;
    if(status) 
        main_loop();
    return 0;
}