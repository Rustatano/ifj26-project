/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 01.10.2026
File: main.c

Description: Main entry point for the IFJ26 language translator

-------------------------------------------------------
*/

#include "def.h"
#include "scanner.h"

int main(/* int argc, char *argv[] */) {

    // Creating Scanner configuration struct
    scanner_conf_t scanner_conf;

    // Initialization of that struct with starting values
    scanner_init(&scanner_conf, stdin);

    // Calling parser function with scanner configuration
    // parser(&scanner_conf, ...) <- get token called inside

    return 0;
}
