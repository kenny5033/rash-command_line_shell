#ifndef RASH_COMMAND_H
#define RASH_COMMAND_H

struct Command{
    char** args;
    int args_size;
    bool background;
};
void command_ctor(struct Command* cmd);
void command_dtor(struct Command* cmd);

#endif