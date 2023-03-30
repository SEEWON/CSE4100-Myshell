/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4100-MP-PL> ");                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if((pid=Fork())==0) {
            if (execve(argv[0], argv, environ) < 0) {	//ex) /bin/ls ls -al &
                char command_from_bin[MAXLINE];         /* Try to execute file from /bin location if error */
                strcpy(command_from_bin, "/bin/\0");
                if (execve(strcat(command_from_bin,argv[0]), argv, environ) < 0) {
                    printf("%s: Command not found.\n", argv[0]);
                    exit(0);
                }
            }
            // execvp(argv[0],argv);
        }

	/* Parent waits for foreground job to terminate */
	if (!bg){ 
	    int status;
        if (waitpid(pid, &status, 0) < 0) unix_error("waitfg: waitpid error");
	}
	else//when there is backgrount process!
	    printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0);
    else if (!strcmp(argv[0], "exit"))
        exit(0);
    else if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    else if (!strcmp(argv[0], "cd")) {

        return 1;
    }
    else if (!strcmp(argv[0], "history")) {

        return 1;
    }
    else return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';       /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' '))   /* Ignore leading spaces */
	    buf++;                      /* By making buffer to point address of next character by adding 1*/

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


