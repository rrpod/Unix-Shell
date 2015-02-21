Implemented a micro shell for Linux Operating system. Execution of built in commands as well as system defined binaries are handled by the shell.
In summary the Interactive Shell supports the following features:
- I/O Redirection
- Command Line Parsing
- Command Execution
- Signal Handling (SIGQUIT, SIGTERM, SIGINT)

This project contains 5 files including the Readme.
1)ushell.c , parse.c and parse.h contain the source code required to create microshell.
2)MAKEFILE. Write make in terminal to compile the program and create the executable.
3)After executing make, an executable .ush will be created which can be used to run the program.