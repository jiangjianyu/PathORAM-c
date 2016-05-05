//
// Created by jyjia on 2016/4/30.
//

#ifndef PATHORAM_DAEMON_H
#define PATHORAM_DAEMON_H

#include "args.h"

/*
   return 0 if success and in child, will also redirect stdout and stderr
   not return if master
   return non-zero if error
*/
int daemon_start(const oram_args_t *args);

/*
   return 0 if success
   return non-zero if error
*/
int daemon_stop(const oram_args_t *args);

#endif //PATHORAM_DAEMON_H
