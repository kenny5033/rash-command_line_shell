/* RASH - Rationality Abandonded SHell
* 
* rash.c implements the main RASH REPL
* 
* Author: Kenny Howes
* Date: 02/12/24
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#include "rash_command.h"
#include "rash_utils.h"

void _int_handle(int n) {
    puts("\nGoodbye!");
    exit(0);
}

int main(int argc, char* argv) {
    struct Command currentCommand;
    char command[256];

    /* set up signals */
    signal(SIGINT, _int_handle);

    while (1) {
        // Give the current directory
        printf("\033[32m%s >> \033[0m", getcwd(NULL, 0));

        // Get command from standard in
        if (fgets(command, 256, stdin) == NULL) {
            // if EOF was encountered
            break;
        } 
        command[strcspn(command, "\n")] = '\0'; // strip newline and put null char in place
        if (command[0] == '\0') {
            // just new line entered, don't want to send that to parseCommand()
            continue;
        }

        command_ctor(&currentCommand);
        parseCommand(command, &currentCommand);
        if (executeCommand(&currentCommand) != 0) {
            fprintf(stderr, "Could not find %s\n", currentCommand.args[0]);
        }
        command_dtor(&currentCommand);
        continue;
    }

    fputc('\n', stdout);
    return 0;
}