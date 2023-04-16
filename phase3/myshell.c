/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#include<signal.h>
#define MAXARGS   128

/* Global variables */
volatile sig_atomic_t total_jobs = 0;
volatile sig_atomic_t total_bg_jobs = 0;
volatile sig_atomic_t reaped_pid = 0;
volatile sig_atomic_t myshell_pgid = 0;

struct each_job {
    int status;         // 0 if terminated, 1 if running, 2 if suspended
    int pid;            // pid of running job
    int bg_job_idx;     // if bg job, 1, 2, 3, ... (0 for fg job)
    char job_name[50];
} jobs[99999];

/* Function prototypes */
void eval(char *cmdline, FILE *fp, int save_in_history, int *fd1, int *fd2);
void eval_pipeline(char *cmdline, FILE *fp);
int parseline(char *buf, char **argv);
int builtin_command(char **argv, FILE *fp, char *cmdline, int save_in_history);
void save_history(char *cmdline, FILE *fp, int save_in_history);

void sigint_handler() {
    Sio_puts("\n");
    Sio_puts("CSE4100-MP-PL> ");
}

void sigtstp_handler() {

}

void sigchld_handler() {
    int old_errno = errno;
    pid_t pid;
    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_SETMASK, &mask, &prev); /* Block SIGCHLD */

    /* SIGCHLD from child will raise when child process finish, or suspend */
    int status;
    if ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED))>0) {
        reaped_pid = pid;
        // Sio_puts("Reaped or Suspended: ");
        // Sio_putl((long)pid);
        // Sio_puts("\n");
        
        /* If child process is suspended */
        if(WIFSTOPPED(status)) {
            for(int i=0; i<total_jobs; i++) {
                if(pid==jobs[i].pid) {
                    jobs[i].status=2;
                    if(!jobs[i].bg_job_idx) jobs[i].bg_job_idx = ++total_bg_jobs;
                    break;
                }
            }
        } 
        /* If child process is terminated */
        else {
            for(int i=0; i<total_jobs; i++) {
                if(pid==jobs[i].pid) {
                    jobs[i].status=0;
                    break;
                }
            }
        }
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL); /* Optionally unblock SIGCHLD */
    errno = old_errno;
}


int main()  
{
    char cmdline[MAXLINE]; /* Command line */
    FILE *fp = Fopen(".myshell_history", "a+");

    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);

    myshell_pgid = getpgid(0);

    while (1) {
        /* Read */
        printf("CSE4100-MP-PL> ");                   
        fgets(cmdline, MAXLINE, stdin); 

        if (feof(stdin))
            exit(0);
        if (!strchr(cmdline, '|')) eval(cmdline, fp, 1, NULL, NULL);   /* If no pipeline included */
        else eval_pipeline(cmdline, fp);                /* If pipeline included */
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
/* If called from eval_pipeline, don't save in history. Else, save in history. */
void eval(char *cmdline, FILE* fp, int save_in_history, int *fd1, int *fd2) 
{
    sigset_t mask, prev;
    Sigemptyset(&prev);
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigaddset(&mask, SIGINT);
    Sigaddset(&mask, SIGTSTP);
    Sigaddset(&mask, SIGTTOU);
    Sigprocmask(SIG_SETMASK, &mask, &prev); /* Block signals */

    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  /* parseline() returns 1 if bg, or blank line input */
    if (bg==-1) return;
    if (argv[0] == NULL)  return;   /* Ignore empty lines */

    if (!builtin_command(argv, fp, cmdline, save_in_history)) {  /* If builtin command, execute in current process */
        if(argv[0][0]!='!') save_history(cmdline, fp, save_in_history);              /* Store in history */
        if((pid=Fork())==0) {
            sigset_t empty;
            Sigemptyset(&empty);
            Sigprocmask(SIG_SETMASK, &empty, NULL);

            if (setpgid(0, 0) == -1 && errno == EINTR) {
                while (setpgid(0, 0) == -1 && errno == EINTR);
            }
            pid_t my_pgid = getpgid(0);
            
            /* If pipeline set, set the IO descripters */
            if (fd1 && !fd2) {
                close(fd1[0]);                  /* Will only use write-end in child */
                Dup2(fd1[1], 1);                /* Substitute stdout to fd1[1] */
            }
            else if (fd1 && fd2) {
                close(fd1[1]);
                Dup2(fd1[0], 0);
                close(fd2[0]);
                Dup2(fd2[1], 1);
            }
            else if (!fd1 && fd2) {
                close(fd2[1]);
                Dup2(fd2[0], 0);
            }

            if (!strcmp(argv[0], "jobs")) {
                for(int i=0; i<total_jobs; i++) {
                    if(jobs[i].status && jobs[i].bg_job_idx) {
                        char status[15];
                        jobs[i].status==1 ? strcpy(status, "running") : strcpy(status, "suspended");
                        printf("[%d] %s %s\n", jobs[i].bg_job_idx, status, jobs[i].job_name);
                    }
                }

                exit(0);
            }

            else if (!strcmp(argv[0], "history")) {
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
                exit(0);
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
                        char new_cmdline[MAXLINE]="";

                        int prev_history_len = strlen(prev_history)-1;
                        int cmdline_len = strlen(cmdline)-1;
                                         
                        strncpy(new_cmdline, prev_history, prev_history_len);
                        strncpy(new_cmdline + prev_history_len, cmdline+2, strlen(cmdline)-2);
                        new_cmdline[prev_history_len + cmdline_len - 2] = '\n';
                        new_cmdline[prev_history_len + cmdline_len - 1] = '\0';

                        printf("%s", new_cmdline);
                        if (!strchr(new_cmdline, '|')) eval(new_cmdline, fp, save_in_history, NULL, NULL);   /* If no pipeline included */
                        else eval_pipeline(new_cmdline, fp);  
                    }
                    else printf("-myshell: !!: event not found\n");
                    exit(0);
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
                                if (!strchr(new_cmdline, '|')) eval(new_cmdline, fp, save_in_history, NULL, NULL);   /* If no pipeline included */
                                else eval_pipeline(new_cmdline, fp);  
                                break;
                            }
                        }
                    }

                    fseek(fp, 0, SEEK_END);         /* Moves file pointer to the end of the file */

                    if(!matching_history) printf("-myshell: %s: event not found\n", argv[0]);

                    exit(0);
                }
            }
            if (execvp(argv[0],argv) < 0) {     /* Execute with the right location (execvp automatically finds) */
                if (strcmp(argv[0], "fg")!=0 && strcmp(argv[0], "bg")!=0) printf("%s: Command not found.\n", argv[0]);
                exit(0);
            };
        }
        else {                      /* Close not using fd in parent */
            if (fd1 && !fd2) close(fd1[1]);
            else if (fd1 && fd2) {
                close(fd1[0]);
                close(fd2[1]);
            }
            else if (!fd1 && fd2) close(fd2[0]);

            /* Parent waits for foreground job to terminate */
            if (!bg){ 
                tcsetpgrp(STDIN_FILENO, pid);

                /* Assign to job managing Data Structure */
                jobs[total_jobs].status = 1;
                jobs[total_jobs].pid = pid;
                jobs[total_jobs].bg_job_idx = 0;
                char job_name[50];
                strcpy(job_name, argv[0]);
                for(int i=1; ; i++) {
                    if(!argv[i]) break;
                    strcat(job_name, " ");
                    strcat(job_name, argv[i]);
                }
                strcpy(jobs[total_jobs].job_name, job_name);
                total_jobs++;

                while (pid != reaped_pid) {
                    sigset_t empty;
                    Sigemptyset(&empty);

                    // Sio_puts("Suspended\n");
                    Sigsuspend(&empty); /* Wait for any signal. SIGCHLD, SIGTSTP, SIGINT */
                    // Sio_puts("Finished\n");

                }
                tcsetpgrp(STDIN_FILENO, myshell_pgid);
                Sigprocmask(SIG_SETMASK, &prev, NULL); /* Optionally unblock SIGCHLD */
            }
            /* When background process */
            else {
                jobs[total_jobs].status = 1;
                jobs[total_jobs].pid = pid;
                jobs[total_jobs].bg_job_idx = ++total_bg_jobs;
                char job_name[50];
                strcpy(job_name, argv[0]);
                for(int i=1; ; i++) {
                    if(!argv[i]) break;
                    strcat(job_name, " ");
                    strcat(job_name, argv[i]);
                }
                strcpy(jobs[total_jobs].job_name, job_name);
                total_jobs++;

                Sigprocmask(SIG_SETMASK, &prev, NULL); /* Optionally unblock SIGCHLD */
            }
        }
    }
    return;
}


/* $begin eval_pipeline */
/* eval_pipeline - Evaluate a command line with '|' included*/
void eval_pipeline(char *cmdline, FILE* fp) 
{
    int i=0;

    /* Count number of command connected with pipe */
    char temp[MAXLINE], original_cmdline[MAXLINE];
    strcpy(temp, cmdline); strcpy(original_cmdline, cmdline);
    int each_cmd_cnt=0;
    char *each_cmdline = strtok(temp, "|");
    while(each_cmdline) {
        i++;
        each_cmdline = strtok(NULL, "|");
    }

    int *fd1 = (int*) malloc(sizeof(int)*2);          /* File descripter for pipeline */
    int *fd2 = (int*) malloc(sizeof(int)*2);
    if (pipe(fd1)<0) unix_error("pipe error");
    if (pipe(fd2)<0) unix_error("pipe error");

    /* Execute the first command, send the result to first pipe */
    each_cmdline = strtok(cmdline, "|");
    eval(each_cmdline, fp, 0, fd1, NULL);

    /* Get input from pipe fd1, Execute the between commands, send the result to next pipe fd2 */
    int j=0;
    while(j < i-2) {
        j++;
        each_cmdline = strtok(NULL, "|");
        eval(each_cmdline, fp, 0, fd1, fd2);    // fd1로부터 읽음, fd2로 씀.

        free(fd1);
        fd1 = (int*) malloc(sizeof(int)*2);
        fd1[0] = fd2[0]; fd1[1] = fd2[1];       // fd2로 fd1덮어씀. (fd1으로 fd2 복사)

        free(fd2);
        fd2 = (int*) malloc(sizeof(int)*2);
        if (pipe(fd2)<0) unix_error("pipe error");
    }

    /* Execute the last command, print the result to stdout */
    each_cmdline = strtok(NULL, "|");
    eval(each_cmdline, fp, 0, NULL, fd1); //fd1로부터 읽음, stdout으로 씀.
    free(fd2);

    /* Substitute !!, !# to corresponding commandline and save to history */
    // !! | !# | grep -> ls | history | grep
    char substituted_cmdline[MAXLINE];
    int len_original_cmdline = strlen(original_cmdline);
    int pos=0;  // which is current position for substituted_cmdline
    int dont_save = 0; // If error occurred with ! commands, don't save to history flag

    for(int k=0; k<len_original_cmdline-1;) {
        if(original_cmdline[k]=='!') {
            /* If command was !!, Substitute command !! with corresponding cmdline */
            if(original_cmdline[k+1]=='!') {
                k+=2; // Skip by the length of "!!".
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

                    strncpy(new_cmdline, prev_history, prev_history_len);
                    new_cmdline[prev_history_len+1] = '\0';
                    
                    int new_cmdline_len = strlen(new_cmdline);
                    strcpy(substituted_cmdline+pos, new_cmdline);
                    pos += new_cmdline_len;
                } else {
                    dont_save = 1;
                    break;
                }
            } 
            /* If command was !#, Substitute command !# with corresponding cmdline */
            else {
                char *tmpargv[MAXARGS]; /* For parsing !# */
                char tmpbuf[MAXLINE];
                strcpy(tmpbuf, original_cmdline);
                parseline(tmpbuf+k, tmpargv);
                char history[MAXLINE];
                char history_idx_str[MAXLINE];
                strcpy(history_idx_str, tmpargv[0]+1);
                int history_idx_int = atoi(history_idx_str);
                int matching_history = 0;

                int tmp = history_idx_int;
                int digit=0;
                while(tmp>0) {
                    tmp /= 10;
                    digit++;
                }

                k+=(digit+1); // Skip by the length of !#
                
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
                            char new_cmdline[MAXLINE];

                            int history_len = strlen(history)-1;

                            strncpy(new_cmdline, history, history_len);
                            new_cmdline[history_len+1] = '\0';
                            
                            int new_cmdline_len = strlen(new_cmdline);
                            strcpy(substituted_cmdline+pos, new_cmdline);
                            pos += new_cmdline_len;
                            
                            break;
                        }
                    }
                }

                fseek(fp, 0, SEEK_END);         /* Moves file pointer to the end of the file */

                if(!matching_history) dont_save = 1;
            }
        }
        else {
            substituted_cmdline[pos] = original_cmdline[k];
            pos++; k++;

            continue;
        } 
    }
    substituted_cmdline[pos++] = '\n';
    substituted_cmdline[pos] = '\0';
    
    if(!dont_save) save_history(substituted_cmdline, fp, 1);
    return ;
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
    else if (!strcmp(argv[0], "fg")) {
        int wrong_idx = 1;
        pid_t pid;
        int i;
        if(argv[1] && argv[1][0]=='%') {
            int job_idx = atoi(argv[1]+1);      /* skip '%', get job idx */
            for(i=0; i<total_jobs; i++) {
                if(job_idx==jobs[i].bg_job_idx && jobs[i].status) {
                    wrong_idx = 0;
                    pid = jobs[i].pid;
                    jobs[i].status = 1;
                    char status[15] = "running";
                    printf("[%d] %s %s\n", jobs[i].bg_job_idx, status, jobs[i].job_name);
                    break;
                }
            }
        }

        if(wrong_idx) printf("No Such Job\n");
        else {
            tcsetpgrp(STDIN_FILENO, getpgid(pid));
            kill(jobs[i].pid, SIGCONT);

            while (pid != reaped_pid) {
                sigset_t empty;
                Sigemptyset(&empty);
                Sigsuspend(&empty); /* Wait for any signal. SIGCHLD, SIGTSTP, SIGINT */
            }
            tcsetpgrp(STDIN_FILENO, myshell_pgid);
        }
        return 1;
    }
    else if (!strcmp(argv[0], "bg")) {
        int wrong_idx = 1;
        pid_t pid;
        int i;
        if(argv[1] && argv[1][0]=='%') {
            int job_idx = atoi(argv[1]+1);      /* skip '%', get job idx */
            for(i=0; i<total_jobs; i++) {
                if(job_idx==jobs[i].bg_job_idx && jobs[i].status) {
                    wrong_idx = 0;
                    pid = jobs[i].pid;
                    jobs[i].status = 1;
                    char status[15] = "running";
                    printf("[%d] %s %s\n", jobs[i].bg_job_idx, status, jobs[i].job_name);
                    break;
                }
            }
        }

        if(wrong_idx) printf("No Such Job\n");
        else {
            kill(jobs[i].pid, SIGCONT);
        }
        return 1;
    }
    else if (!strcmp(argv[0], "kill")) {
        int wrong_idx = 1;
        pid_t pid;
        int i;
        if(argv[1] && argv[1][0]=='%') {
            int job_idx = atoi(argv[1]+1);      /* skip '%', get job idx */
            for(i=0; i<total_jobs; i++) {
                if(job_idx==jobs[i].bg_job_idx && jobs[i].status) {
                    wrong_idx = 0;
                    pid = jobs[i].pid;
                    jobs[i].status = 0;
                    break;
                }
            }
        }

        if(wrong_idx) printf("No Such Job\n");
        else {
            kill(jobs[i].pid, SIGKILL);
        }
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

    /* Replace trailing '\n' with space, if exists */
    if(buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]= ' ';
    while (*buf && (*buf == ' '))   /* Ignore leading spaces */
	    buf++;                      /* By making buffer to point address of next character by adding 1*/

    /* Build the argv list */
    argc = 0;
    
    while (*buf != '\0') {
        /* Check for double quotes */
        if (*buf == '"') {
            char *dq_start = ++buf;
            char *dq_end = strchr(dq_start, '"');
            if (!dq_end) {
                printf("Error: Double quote is not closed.\n");
                return -1;
            }
            *dq_end++ = '\0';
            argv[argc++] = dq_start;
            buf = dq_end;
        } else if (*buf == ' ' || *buf =='\'') {    /* Ignore blank or single quotation marks */
            *buf++ = '\0';
        } else {
            if (argc == 0 || *(buf - 1) == '\0') {
                argv[argc++] = buf;
            }
            buf++;
        }
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