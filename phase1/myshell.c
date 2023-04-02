/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline, FILE *fp);
int parseline(char *buf, char **argv);
int builtin_command(char **argv, FILE *fp, char *cmdline);
int save_history(char *cmdline, FILE *fp);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    FILE *fp = Fopen(".myshell_history", "a+");

    while (1) {
	/* Read */
	printf("CSE4100-MP-PL> ");                   
	fgets(cmdline, MAXLINE, stdin); 

	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline, fp);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, FILE* fp) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv, fp, cmdline)) { /* If builtin command, execute in current process */
        save_history(cmdline, fp);     /* Store in history */
        if((pid=Fork())==0) {
            if (execve(argv[0], argv, environ) < 0) {	//ex) /bin/ls ls -al &
                char command_from_bin[MAXLINE];         /* Try to execute file from /bin location if error */
                strcpy(command_from_bin, "/bin/");
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
int builtin_command(char **argv, FILE* fp, char* cmdline) 
{
    if (!strcmp(argv[0], "exit")) { /* quit command */
        save_history(cmdline, fp);
        Fclose(fp);
	    exit(0);
    }
    else if (!strcmp(argv[0], "cd")) {
        save_history(cmdline, fp);
        char destination[MAXLINE];
        if(!argv[1] || !strcmp(argv[1], "~") || !strcmp(argv[1], "~/")) {    /* set home location for "cd", "cd ~" input */
            strcpy(destination, getenv("HOME"));
        } 
        else if (argv[1][0]=='$') {     /* If argv[1] is environment variable */
            char environment[MAXLINE];
            strncpy(environment, argv[1]+1, strlen(argv[1])-1);
            environment[strlen(argv[1])] = '\0';
            strcpy(destination, getenv(environment));
        }
        else {                          /* If argv[1] is specific location */
            strcpy(destination, argv[1]);
        }
        if(chdir(destination) < 0) {
            printf("-myshell: cd: %s: No such file or directory.\n", argv[1]);
        }
        return 1;
    }
    else if (!strcmp(argv[0], "history")) {
        save_history(cmdline, fp);             /* Store in history */
        char history[MAXLINE];
        int i=0;
        fseek(fp, 0, SEEK_SET);         /* Moves file pointer to the beginning of the file */
        while(1) {
            if(!Fgets(history, MAXLINE, fp)) break;
            else {
                printf("%d\t%s", ++i, history);
            }
        }
        fseek(fp, 0, SEEK_END);         /* Moves file pointer to the end of the file */
        return 1;
    }
    else if (argv[0][0]=='!') {
        /* In case input starts with !! */
        if (argv[0][1]=='!') {
            int history_exists = 0;
            char prev_history[MAXLINE];
            char history[MAXLINE];
            fseek(fp, 0, SEEK_SET);         /* Move file pointer to the beginning of the file */

            /* Read history from beginning, */
            while(1) {
                if(!Fgets(history, MAXLINE, fp)) break;
                history_exists = 1;
                prev_history[0] = '\0';
                strcpy(prev_history, history);
            }

            fseek(fp, 0, SEEK_END);         /* Move file pointer to the end of the file */

            if (history_exists) {
                /* Make new cmdline, substituting !! with previous command. -1 is for removing the '\n' in the end */
                char new_cmdline[MAXLINE];

                int prev_history_len = strlen(prev_history)-1;
                int cmdline_len = strlen(cmdline)-1;

                strncpy(new_cmdline, prev_history, prev_history_len);
                strncpy(new_cmdline + prev_history_len, cmdline+2, strlen(cmdline)-2);
                new_cmdline[prev_history_len + cmdline_len - 2] = '\n';
                new_cmdline[prev_history_len + cmdline_len - 1] = '\0';

                eval(new_cmdline, fp);
            }
            else printf("-myshell: !!: event not found\n");

            return 1;
        } 
        
        else {

            return 1;
        }
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

/* $begin save_history */
/* save_history - Save the history in .myshell_history file */
int save_history(char *cmdline, FILE *fp)
{
    /* Read the latest history */
    int history_exists = 0;

    char prev_history[MAXLINE];
    char history[MAXLINE];
    fseek(fp, 0, SEEK_SET);         /* Move file pointer to the beginning of the file */

    while(1) {
        if(!Fgets(history, MAXLINE, fp)) break;
        history_exists = 1;
        prev_history[0] = '\0';
        strcpy(prev_history, history);
    }

    fseek(fp, 0, SEEK_END);         /* Move file pointer to the end of the file */

    /* Save as history if cmdline is new, or different with latest history */
    if (!history_exists || (history_exists && (strcmp(prev_history, cmdline)!=0))) 
    {
        Fputs(cmdline, fp);
    }
    fseek(fp, 0, SEEK_END);         /* Move file pointer to the end of the file */

}
/* $end save_history */