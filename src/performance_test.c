//
// Created by maxxie on 16-5-20.
//
#include <unistd.h>
#include "performance.h"
int main(int argc, char **args) {
    while (1) {
        p_get_performance("127.0.0.1", 30010);
        sleep(5);
    }
}