/* ===================================================================================== */
/*                                                                                       */
/*                ECSE 427 / COMP 310 OPERATING SYSTEMS --- Assignment 1                 */
/*                             Joey LITALIEN | ID: 260532617                             */
/*                                                                                       */
/* ===================================================================================== */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
 
#define MAX_LINE 80                                          /* 80 characters per input, */
#define MAX_CMDS 10                                     /* 10 commands to store in total */
#define MAX_PIDS 10                                         /* 10 jobs to store in total */
#define PROMPT "[joeysh]> "                                       /* customizable prompt */

int   count = 0;                                              /* monotonic input counter */
int   pidCount = 0;

struct job {                                            /* structure for background jobs */
    pid_t pid;
    char  cmd[MAX_LINE];
};

struct hist {                                                   /* structure for history */
    char  cmd[MAX_LINE];
    int   error;
};

struct job jobs[MAX_PIDS];                                      /* create new structures */
struct hist H[MAX_CMDS];

/* ------------------------------------------------------------------------------------- */
/* FUNCTION  parseArgs:                                                                  */
/*    This function takes an input and parse it into tokens. It first replaces all empty */
/* empty spaces with zeros until it hits a non-empty space character indicating the      */
/* beginning of an argument.                                                             */
/* ------------------------------------------------------------------------------------- */
void  parseArgs(int length, char input[], char *args[], int *background)
{
    if (length == 0)                       /* ^d was entered, end of user command stream */
        exit(0);
    if (length < 0){                                          /* handle other error case */
        perror("error: reading command");
        exit(-1);
    }

    const char delim[] = " \t\n";                                      /* set delimiters */
    char *ptr = strtok(input, delim);           /* set pointer to the tokenizing process */
    int   i;
    for (i = 0; i < length; i++) {                          /* first check for ampersand */
        if (input[i] == '&') {                                         /* if it is there */
            *background = 1;                               /* set background switch to 1 */
            input[i] = '\0';                             /* replace it by an empty space */
        }
    }
    i = 0;
    while (ptr != NULL) {                               /* set pointers to the beginning */
        args[i++] = ptr;                                                /* of each token */
        ptr = strtok(NULL, delim);
    }
}

/* ------------------------------------------------------------------------------------- */
/* FUNCTION  executeArgs:                                                                */
/*    This function receives a command line argument array with the first one being a    */
/* file name followed by its arguments. It then forks a child process to execute the     */
/* command using system call execvp(). The parent process waits for the child to return  */
/* if the background switch is on.                                                       */
/* ------------------------------------------------------------------------------------- */
void  executeArgs(char input[], char *args[], int *background)
{
    pid_t  child = fork();                                       /* fork a child process */
    int    cstatus;                                                      /* child status */

    if (child < 0) {                                              /* handle fork failure */
        printf("error: forking child process failed\n");
        exit(1);
    }
    else if (child == 0) {                              /* when inside the child process */
        if (execvp(*args, args) < 0)                               /* excute the command */
            exit(1);
    } else {
        if (*background == 0)                             /* if background switch is off */
            waitpid(child, &cstatus, WUNTRACED);
        else {
            printf("[%d]  %d\n", pidCount, child);         /* display job number and PID */
            jobs[pidCount % MAX_PIDS].pid = child;            /* store PID in the struct */
            strcpy(jobs[pidCount % MAX_PIDS].cmd, input);       /* store name of command */
            pidCount++;
        }                                          
    }
    if (WIFEXITED(cstatus)) {                          /* check if child exlsit properly */
        if (WEXITSTATUS(cstatus)) {                                            /* if not */
            H[(count - 1) % MAX_CMDS].error = 1;        /* update error code and display */
            printf("%s: command not found\n", args[0]);     /* erroneous command message */
        }
    }
}

/* ------------------------------------------------------------------------------------- */
/* FUNCTION  printJobs:                                                                  */
/*    This function simply prints the current jobs running in background. It only prints */
/* the last MAX_PIDS jobs similar to the history command.                                */
/* ------------------------------------------------------------------------------------- */
void  printJobs()
{
    int  i,
         pids,
         cstatus;
    pids = (pidCount < MAX_PIDS) ? pidCount : MAX_PIDS;  /* find number of jobs to print */
    for (i = pidCount - pids; i < pidCount; i++) {                   /* display all jobs */
        int j = i % MAX_PIDS;
        if (waitpid(jobs[j].pid, &cstatus, WNOHANG) == 0)    /* if child is not done yet */
            printf("[%d]  %d  %s\n", i, jobs[j].pid, jobs[j].cmd);  /* still in the back */
    }
}

/* ------------------------------------------------------------------------------------- */
/*                                Main program starts here                               */
/* ------------------------------------------------------------------------------------- */
int  main(void)
{
    char  input[MAX_LINE],                            /* passed to parseArgs for parsing */
          line[MAX_LINE],                              /* unaltered entered command-line */
          entry[MAX_LINE],          /* entry of the history when scanning for 1st letter */
         *args[MAX_LINE/+1];                                     /* pointer to arguments */
    int   background,     /* background switch: equals 0 if a command is followed by '&' */
          cmds,
          len,                                                   /* length of each input */
          found,                                /* flag if entry is found while scanning */
          errIndex;                                          /* index of error array E[] */

    while (1) {                              /* Program terminates normally inside setup */
        background = 0;
        printf(PROMPT);
        fflush(stdout);
        memset(line, 0, MAX_LINE);                        /* reset line for next command */
        memset(input, 0, MAX_LINE);                      /* reset input for next command */
        memset(args, 0, MAX_LINE);

        cmds = (count < MAX_CMDS) ? count : MAX_CMDS;      /* # of history lines to scan */

        len = read(STDIN_FILENO, line, MAX_LINE);           /* read what the user enters */
        fflush(stdout);

        if (len == 1)                   /* handle the seg fault 'enter' is first command */
            continue;

        line[len - 1] = '\0';                       /* remove carriage return at the end */
        strcpy(input, line);                              /* convert pointer to C string */
        errIndex = count % MAX_CMDS;

        if (input[0] == 'r') {                                       /* check for r only */
            if (len == 2) {
                if (count) {
                    strcpy(entry, H[(count - 1) % MAX_CMDS].cmd);     /* retrieve recent */
                    len = strlen(entry);                                /* update length */
                    errIndex = count - 1;                         /* get its error index */
                } else {                                        /* if nothing in history */
                    printf("error: nothing to retrieve\n");        
                    continue;                                        /* get next command */
                }
            } else if ((input[1] == ' ') && (len == 4)) {               /* check for r ? */
                int  j;
                for (j = 0; j < cmds; j++) {  /* scan history from most recent to oldest */
                    found = 0;                                 /* match flag set to zero */
                    memset(entry, 0, MAX_LINE);
                    strcpy(entry, H[(count - j - 1) % MAX_CMDS].cmd); /* recent 2 oldest */
                    if (entry[0] == input[2]) {                    /* if there's a match */
                        found = 1;                                  /* set match flag on */
                        errIndex = count - j - 1;                  /* get index of match */
                        len = strlen(entry);
                        break;
                    }
                }
                if (!found) {                                   /* if not match is found */
                    printf("error: no recent commands starting with '%c'\n", input[2]);
                    continue;                                        /* get next command */
                }
            }
            printf("%s\n", entry);
            memset(input, 0, MAX_LINE);
            strcpy(input, entry);
            if (H[errIndex].error == 1) {     /* if command failed on first try, display */
                printf("%s: command not found\n", entry);   /* erroneous command message */
                continue;                                            /* get next command */
            }
        } 

        strcpy(H[count % MAX_CMDS].cmd, input);                        /* update history */
        H[count % MAX_CMDS].error = 0;                          /* initialize error code */
        count++;                                              /* increment input counter */

        parseArgs(len, input, args, &background);                     /* parse the input */

        cmds = (count < MAX_CMDS) ? count : MAX_CMDS;     /* # of history lines to print */
        if (strcmp(args[0], "history") == 0) {                  /* if user types history */
            int  i;
            for (i = count - cmds; i < count; i++)              /* print each entry with */
                printf("%2d  %s\n", i + 1, H[i % MAX_CMDS].cmd);  /* appropriate numbers */
            continue;
        } else if (strcmp(args[0], "cd") == 0) {                     /* change directory */
            chdir(args[1]);
            continue;
        } else if (strcmp(args[0], "pwd") == 0) {                /* display current path */
            char path[MAX_LINE];
            if (getcwd(path, MAX_LINE) != NULL) {               /* if path is accessible */
                printf("%s\n", path);                                        /* print it */
                continue;
            }
        } else if (strcmp(args[0], "exit") == 0) {                       /* exit program */
            printf("logout\n");                                        /* hasta la vista */
            exit(0);                                                /* exit successfully */
        } else if (strcmp(args[0], "jobs") == 0) {                       /* display jobs */
            printJobs();
            continue;                                         
        } else if (strcmp(args[0], "fg") == 0) { /* bring program in background to front */
            pid_t cpid;
            cpid = atoi(args[1]);                                    /* get index of PID */
            waitpid(jobs[cpid].pid, NULL, 0);                       /* brint it to front */
            continue;                                         
        }

        if (H[errIndex].error == 0) {              /* if no error where found previously */
            executeArgs(H[(count - 1) % MAX_CMDS].cmd, args, &background);    /* execute */
        }
    }
}