#!/bin/bash
gcc -I ./src src/main.c src/cJSON.c -ldl -o ./bin/a
gcc -shared -fPIC -I ./src src/testplugin.c -o ./bin/testplugin.so
