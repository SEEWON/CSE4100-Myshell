/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline, FILE *fp, int save_in_history, int *fd);
void eval_pipeline(char *cmdline, FILE *fp);
int parseline(char *buf, char **argv);
int builtin_command(char **argv, FILE *fp, char *cmdline, int save_in_history);
void save_history(char *cmdline, FILE *fp, int save_in_history);

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
    if (!strchr(cmdline, '|')) eval(cmdline, fp, 1, NULL);   /* If no pipeline included */
    else eval_pipeline(cmdline, fp);                /* If pipeline included */
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
/* If called from eval_pipeline, don't save in history. Else, save in history. */
void eval(char *cmdline, FILE* fp, int save_in_history, int *fd) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (argv[0] == NULL)  
	    return;                                 /* Ignore empty lines */
    if (!builtin_command(argv, fp, cmdline, save_in_history)) {  /* If builtin command, execute in current process */
        save_history(cmdline, fp, save_in_history);              /* Store in history */
        if((pid=Fork())==0) {
            
            if (fd) {
                close(fd[0]);                   /* Will only use write-end in child */
                Dup2(fd[1], 1);                 /* Substitte stdout to fd[1] */
            }              
            if (execvp(argv[0],argv) < 0) {     /* Execute with the right location (execvp automatically finds) */
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            };                    
        }
        if (fd) close(fd[1]);                    /* Will only use read-end in child */

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

void eval2(char *cmdline, FILE* fp, int save_in_history, int *fd) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (argv[0] == NULL)  
	    return;                                 /* Ignore empty lines */
    if (!builtin_command(argv, fp, cmdline, save_in_history)) {  /* If builtin command, execute in current process */
        save_history(cmdline, fp, save_in_history);              /* Store in history */
        if((pid=Fork())==0) {
            
            if (fd) {
                Dup2(fd[0], 0);
                Dup2(fd[1], 1);
            }              
            if (execvp(argv[0],argv) < 0) {     /* Execute with the right location (execvp automatically finds) */
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            };                    
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

void eval3(char *cmdline, FILE* fp, int save_in_history, int *fd) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (argv[0] == NULL)  
	    return;                                 /* Ignore empty lines */
    if (!builtin_command(argv, fp, cmdline, save_in_history)) {  /* If builtin command, execute in current process */
        save_history(cmdline, fp, save_in_history);              /* Store in history */
        if((pid=Fork())==0) {
            
            if (fd) {
                Dup2(fd[0], 0);
            }              
            if (execvp(argv[0],argv) < 0) {     /* Execute with the right location (execvp automatically finds) */
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            };                    
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

/* $begin eval_pipeline */
/* eval_pipeline - Evaluate a command line with '|' included*/
void eval_pipeline(char *cmdline, FILE* fp) 
{
    char *each_cmdline = strtok(cmdline, "|");
    save_history(cmdline, fp, 1);

    int fd[2];          /* File descripter for pipeline */

    // Dup2(0, fd[0]);

    // while(each_cmdline != NULL) {
        if (pipe(fd)<0) unix_error("pipe error");
        eval(each_cmdline, fp, 0, fd);      //ls
        each_cmdline = strtok(NULL, "|");

        eval2(each_cmdline, fp, 0, fd);     //grep abc
        each_cmdline = strtok(NULL, "|");

        eval3(each_cmdline, fp, 0, fd);     //grep c
        each_cmdline = strtok(NULL, "|");


        // char temp[255];
        // read(fd[0], temp, 255);
        // printf("%s", temp);


    // }
    close(fd[0]);     /* Close file descriptor when fisnished using */
    close(fd[1]);
    


    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv, FILE* fp, char* cmdline, int save_in_history) 
{
    if (!strcmp(argv[0], "exit")) { /* exit command */
        save_history(cmdline, fp, save_in_history);
        Fclose(fp);
	    exit(0);
    }
    else if (!strcmp(argv[0], "cd")) {
        save_history(cmdline, fp, save_in_history);
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
        else {                          /* If argv[1] is specific(user-input) location */
            strcpy(destination, argv[1]);
        }
        if(chdir(destination) < 0) {    /* Navigate to location, print error if occurs */
            printf("-myshell: cd: %s: No such file or directory.\n", argv[1]);
        }
        return 1;
    }
    else if (!strcmp(argv[0], "history")) {
        save_history(cmdline, fp, save_in_history);     /* Store in history */
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
        /* In case cmdline starts with !! */
        if (argv[0][1]=='!') {
            int history_exists = 0;
            char prev_history[MAXLINE];
            char history[MAXLINE];
            fseek(fp, 0, SEEK_SET);         /* Move file pointer to the beginning of the file */

            /* Read history from beginning, figure out the latest history */
            while(1) {
                if(!Fgets(history, MAXLINE, fp)) break;
                history_exists = 1;
                prev_history[0] = '\0';
                strcpy(prev_history, history);
            }

            fseek(fp, 0, SEEK_END);         /* Move file pointer to the end of the file */

            /* In case latest history exists */
            if (history_exists) {
                /* Make new cmdline, substituting !! with previous command. -1 is for removing the '\n' in the end */
                char new_cmdline[MAXLINE];

                int prev_history_len = strlen(prev_history)-1;
                int cmdline_len = strlen(cmdline)-1;

                strncpy(new_cmdline, prev_history, prev_history_len);
                strncpy(new_cmdline + prev_history_len, cmdline+2, strlen(cmdline)-2);
                new_cmdline[prev_history_len + cmdline_len - 2] = '\n';
                new_cmdline[prev_history_len + cmdline_len - 1] = '\0';

                printf("%s", new_cmdline);
                eval(new_cmdline, fp, save_in_history, NULL);
            }
            else printf("-myshell: !!: event not found\n");

            return 1;
        } 

        /* In case cmdline starts with !, and pass # */
        else {
            char history[MAXLINE];
            char history_idx_str[MAXLINE];
            strcpy(history_idx_str, argv[0]+1);
            int history_idx_int = atoi(history_idx_str);
            int matching_history = 0;

            fseek(fp, 0, SEEK_SET);         /* Moves file pointer to the beginning of the file */

            for(int i=1;;i++) {
                if(!Fgets(history, MAXLINE, fp)) {
                    break;
                }
                /* If matching(with #) history exists  */
                else {
                    if(history_idx_int==i) {    /* Execute corresponding history */
                        matching_history = 1;

                        /* Make new cmdline, substituting !# with corresponding cmdline. -1 is for removing the '\n' in the end */
                        char new_cmdline[MAXLINE]="";
                        char exc_part_in_cmdline[MAXLINE]="";
                        char option_part_in_cmdline[MAXLINE]="";
                        sprintf(exc_part_in_cmdline, "%d", history_idx_int);
                        int cmdline_len = strlen(cmdline)-1;
                        int exc_part_len = strlen(exc_part_in_cmdline)+1; // +1 for the length of '!'
                        int history_len = strlen(history)-1;

                        /* Passes the rest of cmdline, as input (After substituting !#) */
                        strncpy(option_part_in_cmdline, cmdline + exc_part_len, cmdline_len-exc_part_len);
                        int option_part_len = strlen(option_part_in_cmdline);
                        strncpy(new_cmdline, history, history_len);
                        strcat(new_cmdline, option_part_in_cmdline);
                        new_cmdline[history_len + option_part_len] = '\n';
                        new_cmdline[history_len + option_part_len +1] = '\0';
                        
                        printf("%s", new_cmdline);
                        eval(new_cmdline, fp, save_in_history, NULL);
                        break;
                    }
                }
            }

            fseek(fp, 0, SEEK_END);         /* Moves file pointer to the end of the file */

            if(!matching_history) printf("-myshell: %s: event not found\n", argv[0]);

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

    /* Replace trailing '\n' with space, if exists */
    if(buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]= ' ';       
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
void save_history(char *cmdline, FILE *fp, int save_in_history)
{
    if(!save_in_history) return;
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
    fseek(fp, 0, SEEK_END);         /* Move file pointer to the end again, since Fputs might executed */

}
/* $end save_history */