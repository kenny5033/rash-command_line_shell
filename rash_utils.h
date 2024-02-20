#ifndef RASH_H
#define RASH_H

int getPath(char*** buffer);
char* pathSearchCommand(const char* cmd);
int parseCommand(char* command, struct Command* commandInfo_out);
int executeCommand(const struct Command* commandInfo_in);

#endif