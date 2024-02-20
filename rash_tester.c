/*
* USE: gcc -etests ... everything else
* ^ Needs to enter at tests()
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "rash_command.h"
#include "rash_utils.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void _test_getPath(void) {
    fputs("Testing getPath()\n", stdout);
    char** pathBuffer;
    int bufferSize = getPath(&pathBuffer);
    if (bufferSize == -1) {
        fputs("Error with getPath()", stderr);
        exit(-1);
    }
    for (int i = 0; i < MIN(bufferSize, 5); ++i)
        printf("%s\n", pathBuffer[i]);
    fputs("getPath() passed tests\n\n", stdout);
}

void _test_parseCommand(void) {
    fputs("Testing parseCommand()\n", stdout);
    char command[] = "test arg1 arg2 &"; // commands are arrays (not pointers) so strtok() works in parseCommand()
    struct Command cmd;
    command_ctor(&cmd);

    parseCommand(command, &cmd);
    if(strcmp(cmd.args[0], "test") != 0) {
        fputs("Problem with parseCommand(): cmd.command != \"test\"\n", stderr);
        exit(-1);
    }
    if(strcmp(cmd.args[1], "arg1") != 0) {
        fputs("Problem with parseCommand(): cmd.args[0] != \"arg1\"\n", stderr);
        exit(-1);
    }
    if(strcmp(cmd.args[2], "arg2") != 0) {
        fputs("Problem with parseCommand(): cmd.args[1] != \"arg2\"\n", stderr);
        exit(-1);
    }
    if(cmd.background == false) {
        fputs("Problem with parseCommand(): cmd.background == false\n", stderr);
        exit(-1);
    }
    fputs("parseCommand() passed tests\n\n", stdout);
}

void _test_pathSearchCmd(void) {
    fputs("Testing pathSearchCmd()\n", stdout);
    char* cmd_test = pathSearchCommand("cat");
    if (strcmp(cmd_test, "/usr/bin/cat") != 0) {
        fputs("Problem with pathSearchCmd(): cat not found at /usr/bin/cat\n-- THIS MAY BE OKAY DEPENDING ON YOUR SYSTEM!", stderr);
    }
    fputs("pathSearchCmd() passed tests\n\n", stdout);
}

int main() {
    _test_getPath();
    _test_parseCommand();
    _test_pathSearchCmd();
    return 0;
}