#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "rash_command.h"

/* _cmd_hash() returns the RASH hash sum of a given string 
* for use with executeCommand() */
int _cmd_hash(const char* cmd) {
    /* _cmd_hash will *rarely* have two strings that hash to the same sum
    * e.x. "exit" and "meow" both == 0x460
    * this is rare. I think it adds flair to keep it in
    */
    int hash = 0;
    unsigned i = 1; // use this to make sure "exit" and "xtei" don't have same hash sum
    while (*cmd) { // while char in cmd is not null char
        hash += (*cmd) * i;
        cmd++; // move through the string
        i++; // increment i to keep track of where char was added to hash
    }
    return hash;
}

/* exit command exits program */
void _shellcmd_exit(void) {
    exit(0);
}

/* cd command changes working directory */
void _shellcmd_cd(char* const cmd_args[], const int arg_count) {
    if (arg_count > 1) {
        if (chdir(cmd_args[1]) != 0)
            perror("Problem changing directory");
    } else {
        // not valid command
        fputs("Use of cd is: cd <dir>\n", stdout);
    }
}

/* pwd command prints current working directory */
void _shellcmd_pwd(void) {
    fprintf(stdout, "%s\n", getcwd(NULL, 0));
}

/* clr command clears the screen */
void _shellcmd_clr(void) {
    // \033[2J for get away from previous output
    // \033[H for bring next line to the top
    // together form clear screen
    fputs("\033[2J\033[H", stdout);
}

/* prodhash command produces the RASH hash sum
* of a given string*/
void _shellcmd_prodhash(char* const cmdsToHash[], const int cmdCount) {
    if (cmdCount <= 1) {
        fputs("Use of prodhash is: prodhash <cmd 1> ... <cmd n>\n", stdout);
    }
    for (int i = 1; i < cmdCount; i++) {
        fprintf(stdout, "%s:\t0x%x\n", cmdsToHash[i], _cmd_hash(cmdsToHash[i]));
    }
}

/* getPath() gets the PATH environment variable and puts it in buffer
* getPath() does NOT free buffer, pass at your own risk of memory leaks */
int getPath(char*** buffer) {
    /* get and validate PATH */
    char* path = strdup(getenv("PATH")); // strdup because strtok (used below) will modify the PATH variable even between function calls
    if (path == NULL) {
        fprintf(stderr, "Could not retrieve PATH environment variable.\n");
        return -1; // couldn't get path error
    }

    /* set up the buffer with 10 string slots to start, reallocs if something is already there */
    int bufferSize = 10;
    *buffer = (char**)malloc(bufferSize * sizeof(char*));
    if (*buffer == NULL) {
        fprintf(stderr, "No heap space for buffer allocation.\n");
        return -1; // ran out of dynamic memory error
    }
    
    /* split up path */
    char* token = strtok(path, ":"); // read first token (path directory) into token, get strtok started
    int i;
    for(i = 0; token != NULL; ++i) {
        // check if buffer needs more space (i.e. if index i will pass array segment)
        if (i >= bufferSize) {
            bufferSize *= 2;
            *buffer = (char**)realloc(*buffer, bufferSize * sizeof(char*));
            if (*buffer == NULL) {
                fprintf(stderr, "No more heap space for buffer allocation.\n");
                return -1; // ran out of dynamic memory error
            }
        }

        /* put token in the buffer */
        /* set aside space for string (+1 for null char)
        * and check for reallocation if allocated memory is already there */
        (*buffer)[i] = (char*)malloc( (strlen(token) + 1) * sizeof(char) );
        if((*buffer)[i] == NULL) { 
            fprintf(stderr, "No more heap space for token allocation.\n");
            return -1; // ran out of dynamic memory error
        }
        strcpy((*buffer)[i], token); // copy the parsed directory token into the buffer slot

        token = strtok(NULL, ":"); // get next token, NULL says keep reading from path
    }

    return i; // success, return number of directories in buffer
}

/* pathSearchCommand() searches PATH to find a command 
* returns string of command's path, if found
* otherwise returns empty string*/
char* pathSearchCommand(const char* cmd) {
    /* get the PATH environment variable */
    char** pathBuffer;
    int pathDirsCount = getPath(&pathBuffer);
    if (pathDirsCount == -1) {
        fputs("Could not get PATH variable.", stderr);
        exit(-1);
    }
    
    /* go through all directories on path
    * scandir() use idea from: 
    * https://man7.org/linux/man-pages/man3/scandir.3.html
    */
    struct dirent** subFiles;
    int subFileNum;
    for (int i = 0; i < pathDirsCount; ++i) { 
        // pathBuffer[i] is current directory in PATH being searched through
        subFileNum = scandir(pathBuffer[i], &subFiles, NULL, alphasort);
        if (subFileNum == -1) {
            perror("Error getting info from PATH directories");
            exit(-1);
        }

        /* search sub files for desired command 
        * PATH searching is NOT recursive 
        * https://www.google.com/search?channel=fs&client=ubuntu-sn&q=is+path+search+recursive
        */
        char* foundPath = NULL;
        while (subFileNum--) {
            if (
                // if the sub file is a regular or symbolic (soft) link file and matches the desired command by name
                (subFiles[subFileNum]->d_type == DT_REG || subFiles[subFileNum]->d_type == DT_LNK)
                && strcmp(subFiles[subFileNum]->d_name, cmd) == 0
            ) {
                strcat(pathBuffer[i], "/"); // add slash to end of PATH directory command is in
                foundPath = strcat(pathBuffer[i], subFiles[subFileNum]->d_name); // return the full path of the found command
            }
            free(subFiles[subFileNum]);
        }
        free(subFiles);

        if (foundPath != NULL) {
            // if path was found and set
            return foundPath;
        }
    }
    
    return ""; // not found, return empty string
}

/* parseCommand() fills a Command struct with respective information
* parseCommand() does not free attributes of commandInfo_out */
int parseCommand(char* restrict command, struct Command* commandInfo_out) {
    /* check if args string array already has stuff or not */
    int bufferSize = 10;
    commandInfo_out->args = (char**)malloc(bufferSize * sizeof(char*));

    if (commandInfo_out == NULL) {
        fprintf(stderr, "No more heap space for command arguments array allocation.");
        return -1;
    }

    char* token = strtok(command, " ");
    int i = 0;
    while (token != NULL) {
        // check i + 1 to leave room for null termination (for execvp() later)
        if (i + 1 >= bufferSize) {
            bufferSize *= 2;
            commandInfo_out->args = (char**)realloc(commandInfo_out->args, bufferSize * sizeof(char*));
            if (commandInfo_out->args == NULL) {
                fprintf(stderr, "No more heap space for buffer allocation.\n");
                return -1; // ran out of dynamic memory error
            }
        }

        /* put token in the args string array */
        /* set aside space for arg string (+1 for null char)
        * and check for reallocation if allocated memory is already there */
        commandInfo_out->args[i] = (char*)malloc( (strlen(token) + 1) * sizeof(char) );
        if (commandInfo_out->args[i] == NULL) { 
            fprintf(stderr, "No more heap space for token allocation.\n");
            return -1; // ran out of dynamic memory error
        }
        strcpy(commandInfo_out->args[i], token); // copy the parsed arg token into the buffer slot

        /* increment stuff */
        i++;
        token = strtok(NULL, " ");
    }

    /* check if last argument was ampersand, if so, make it a background command*/
    if (i > 0 && strcmp(commandInfo_out->args[i - 1], "&") == 0) { 
        // then put in background and remove & from arg list
        commandInfo_out->background = true;
        i--; // decrement i
        // free(commandInfo_out->args[i]); // free & arg
    }
    else
        commandInfo_out->background = false;

    commandInfo_out->args[i] = NULL; // null terminate the string array

    commandInfo_out->args_size = i; // "return" number of arguments
    return 0; // success
}

/* executeCommand() executes the command information in a Command struct
* returns -1 if not found
* 0 if found and executed */
int executeCommand(const struct Command* commandInfo_in) {
    /* built in commands */
    int cmd_code = _cmd_hash(commandInfo_in->args[0]);
    switch (cmd_code) {
        case (0x460): // exit
            _shellcmd_exit();
            return 0;
        case (0x28A): // pwd
            _shellcmd_pwd();
            return 0;
        case (0x12B): // cd
            _shellcmd_cd(commandInfo_in->args, commandInfo_in->args_size);
            return 0;
        case (0xEE4): // prodhash
            _shellcmd_prodhash(commandInfo_in->args, commandInfo_in->args_size);
            return 0;
        case (0x291): // clr
            _shellcmd_clr();
            return 0;
    }
    /* end built in commands */

    /* use relative or absolute path for command */

    /* cmdPath is the variable to be filled in and executed if it exists
    * from relative or absolute path or is on PATH */
    char* cmdPath = commandInfo_in->args[0];
    
    struct stat statBuffer;
    if (stat(cmdPath, &statBuffer) == 0 && S_ISREG(statBuffer.st_mode)) {
        // if found file is regular, skip checking PATH, go straight to execution
        // is goto messy and bad? typically, but judgement call in favor of ease of use and readability
        goto execute;
    }

    /* end use relative or absolute path for command */

    /* use PATH for command */
    cmdPath = pathSearchCommand(cmdPath);

    if (strcmp(cmdPath, "") == 0) {
        // if PATH search did not find a path, i.e. is an empty string
        return -1; // command could not be found
    }
    /* end use PATH for command */

execute:; // semicolon to make this an empty statement, int pid = fork() can't come right after
    /* execute the found command path */
    int pid = fork();

    if (pid == 0) {
        // child process
        execvp(cmdPath, commandInfo_in->args);
    }
    else {
        // main process
        fprintf(stdout, "[%s pid: %d]\n", commandInfo_in->args[0], pid);
        if (!commandInfo_in->background) {
            int status;
            int waitResult = waitpid(pid, &status, W_OK); // wait for the child's pid to finish
            if (waitResult != pid) {
                perror("Problem with calling command");
            } else {
                fprintf(stdout, "[%d -> %d]\n", pid, WEXITSTATUS(status));
            }
        }
    }

    return 0;
}