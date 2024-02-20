#include <stdbool.h>
#include <stdlib.h>

struct Command {
    char** args;
    int args_size;
    bool background;
};

void command_ctor(struct Command* cmd) {
    cmd->args = NULL;
    cmd->background = false;
}

void command_dtor(struct Command* cmd) {
    /* free args and all arguments in it */
    if (cmd->args != NULL) {
        for (int i = 0; i < cmd->args_size; i++) {
            if (cmd->args[i] != NULL)
                free(cmd->args[i]);
        }

        free(cmd->args);
    }
}