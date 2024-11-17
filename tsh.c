/////////////// CONTRIBUTIONS /////////////////////////////////////////

// Saad - Lines: 168-344

// Faisal - Lines: 415-491 


// Zain - Lines: 506-560

/////////////// END OF CONTRIBUTIONS /////////////////////////////////////////
/*
 * tsh - A tiny shell program with job control
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
pid_t mainpid;              /* to store the process id of the main function */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
typedef struct job_t job_t; /* We don't want to write struct job_t everytime */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

int valid_argument(char*);                  //Helper Function for do_bgfg to check if the supplied argument is valid or not

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    mainpid = getpid();
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	    default:
            usage();
	    }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* fg %2: No such job* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/////////////////////// Start of SAAD ////////////////////////////////////////////////////////////////////////////////////

// Function to handle redirection
void handle_redirection(char **argv) {
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "<") == 0) {  
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Redirection error: No input file specified\n");
                exit(EXIT_FAILURE);
            }
            argv[i] = NULL; // Split the command from the file part
            int fd0 = open(argv[i + 1], O_RDONLY, 0);
            if (fd0 < 0) {
                perror("Couldn't open input file");
                exit(1);
            }
            dup2(fd0, STDIN_FILENO); // Redirect stdin to fd0
            close(fd0); // Close the file descriptor as it's no longer needed
            i++; // Skip the filename as it's already processed
        } else if (strcmp(argv[i], ">") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Redirection error: No output file specified\n");
                exit(EXIT_FAILURE);
            }
            argv[i] = NULL; // Split the command from the file part
            int fd1 = open(argv[i + 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd1 < 0) {
                perror("Couldn't open output file");
                exit(1);
            }
            dup2(fd1, STDOUT_FILENO); // Redirect stdout to fd1
            close(fd1); // Close the file descriptor as it's no longer needed
            i++; // Skip the filename as it's already processed
        }
    }
}

#define MAXPIPES 10  // Maximum number of commands in a pipeline

// Split the command line into separate commands by "|"
// Each command is still an array of strings (arguments)
void execute_pipeline(char *cmdline, int bg) {
    int num_cmds = 0;
    char *cmds[MAXARGS];  // Array to store individual commands from the pipeline
    int pipefds[2 * MAXPIPES];  // Array to hold the file descriptors for all pipes
    pid_t pids[MAXPIPES];  // Array to hold the PIDs of the child processes

    // Split the command line into separate commands by "|"
    char *next_cmd = strtok(cmdline, "|");
    while (next_cmd != NULL && num_cmds < MAXPIPES) {
        cmds[num_cmds++] = next_cmd;
        next_cmd = strtok(NULL, "|");
    }

    // Set up all necessary pipes in advance
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Execute each command in the pipeline
    for (int i = 0; i < num_cmds; i++) {
        char *cmd_argv[MAXARGS];  // Array for arguments of the command
        parseline(cmds[i], cmd_argv);  // Function to parse the command (you should define it according to your shell's structure)

        pids[i] = fork();
        if (pids[i] == 0) {  // Child process
            // If not the first command, redirect previous pipe's read end to STDIN
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            // If not the last command, redirect next pipe's write end to STDOUT
            if (i < num_cmds - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            // Close all pipe file descriptors in the child
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipefds[j]);
            }
            // Execute the command
            if (execvp(cmd_argv[0], cmd_argv) < 0) {
                fprintf(stderr, "Unknown command: %s\n", cmd_argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipe file descriptors in the parent
    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefds[i]);
    }

    // Wait for all child processes if this is a foreground job
    if (!bg) {
        for (int i = 0; i < num_cmds; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    } else {
        // For background jobs, add each command to the job list
        for (int i = 0; i < num_cmds; i++) {
            addjob(jobs, pids[i], BG, cmds[i]); // Add the pipeline to the job list
        }
        printf("[%d] (%d) %s\n", pid2jid(pids[num_cmds - 1]), pids[num_cmds - 1], cmdline); // Print background job info
    }
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/
void eval(char *cmdline) {
    char buf[MAXLINE];  // Holds modified command line
    char *argv[MAXARGS]; // Argument list execve()
    int bg;             
    pid_t pid;           // Process id
    sigset_t mask;       // Signal mask

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); // Parse the command line

    if (argv[0] == NULL) 
        return; // Ignore empty lines

    if (!builtin_cmd(argv)) {
        // Check for pipe in command line
        if (strchr(cmdline, '|')) {
            // Pipeline detected, handle pipeline
            execute_pipeline(buf, bg);
        } else {
            // No pipeline, proceed as normal
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            sigprocmask(SIG_BLOCK, &mask, NULL); // Block SIGCHLD

            if ((pid = fork()) == 0) {   // Child runs user job
                sigprocmask(SIG_UNBLOCK, &mask, NULL); // Unblock SIGCHLD
                setpgid(0, 0); // Put the child in a new process group

                // Handle redirection
                handle_redirection(argv);
                  
                // Execute the command (for regular terminal commands)
                if (execvp(argv[0], argv) < 0) {
                    printf("%s: Command not found\n", argv[0]);
                    exit(0);
                }
            }

            // Parent waits for foreground job to terminate
            if (!bg) {
                addjob(jobs, pid, FG, cmdline); // Add this job as a foreground job
                sigprocmask(SIG_UNBLOCK, &mask, NULL); // Unblock SIGCHLD
                waitfg(pid); // Wait for the foreground job to complete
            } else {
                addjob(jobs, pid, BG, cmdline); // Add this job as a background job
                sigprocmask(SIG_UNBLOCK, &mask, NULL); // Unblock SIGCHLD
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
            }
        }
    }
    return;
}

/////////////////////// END OF SAAD ////////////////////////////////////////////////////////////////////////////////////

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) {
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while (*buf) {
        // Skip leading spaces
        while (*buf && (*buf == ' '))
            buf++;

        // Start of an argument
        char *start = buf;
        int inQuotes = 0; // Flag for whether we are inside quotes

        // Iterate over characters until end of this argument
        while (*buf) {
            if (*buf == '"' && (buf == start || *(buf - 1) != '\\')) {
                // Toggle inQuotes flag if this quote is not escaped
                inQuotes = !inQuotes;
                // Remove the quote by shifting the rest of the string left
                memmove(buf, buf + 1, strlen(buf));
                continue; // Skip incrementing buf to stay at the same position
            } else if (*buf == ' ' && !inQuotes) {
                // End of the argument if we're not inside quotes
                break;
            }
            buf++;
        }

        // End of one argument
        if (buf > start) {
            argv[argc++] = start;
            if (*buf) {
                *buf++ = '\0'; // Null-terminate the argument and move to the next
            }
        }
    }

    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    bg = (argc > 0 && *argv[argc - 1] == '&');
    if (bg) {
        argv[--argc] = NULL;
    }
    return bg;
}


///////////////////// Start of Faisal ////////////////////////////////////////////////////

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv) {
    if (!strcmp(argv[0], "quit")) { /* quit command */
        exit(0);  
    }
    if (!strcmp(argv[0], "&")) {    /* Ignore singleton & */
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
    struct job_t *jobp = NULL;
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    if (argv[1][0] == '%') { // JID
        int jid = atoi(&argv[1][1]);
        if (!(jobp = getjobjid(jobs, jid))) {
            printf("%s: No such job\n", argv[1]);
            return;
        }
    } else if (isdigit(argv[1][0])) { // PID
        int pid = atoi(argv[1]);
        if (!(jobp = getjobpid(jobs, pid))) {
            printf("(%d): No such process\n", pid);
            return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    kill(-(jobp->pid), SIGCONT); // Send SIGCONT signal to the entire group of the job.

    if (!strcmp(argv[0], "fg")) { // fg command
        jobp->state = FG;
        waitfg(jobp->pid);
    } else { // bg command
        jobp->state = BG;
        printf("[%d] (%d) %s", jobp->jid, jobp->pid, jobp->cmdline);
    }
    return;
}


/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)          //Loop while the state of this entry is FG. Call sleep to sleep for 1 second between checks.
{
    job_t* temp = getjobpid(jobs,pid);
    while(temp->state == FG){
        sleep(1);
    }
    return;
}

///////////////////// End of Faisal ////////////////////////////////////////////////////

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */


///////////////////// Start of Zain ////////////////////////////////////////////////////
void sigchld_handler(int sig)
{
    pid_t pid;
    int status;
    while (1) {
       pid = waitpid(-1, &status, WNOHANG | WUNTRACED);     //non blocking wait
       if (pid <= 0)  break;                                // No more zombie children to reap.
       else{
        job_t* temp = getjobpid(jobs,pid);
        if(WIFEXITED(status)){                          //if child exited normally then delete the job
            temp->state = UNDEF;
            deletejob(jobs,pid);
        }else if(WIFSIGNALED(status)){                       //if child was sent a termination signal then display the message and delete the job
            job_t* temp = getjobpid(jobs,pid);
            printf("Job [%d] (%d) terminated by signal %d\n",pid2jid(pid),pid,WTERMSIG(status));
            if(temp->state == FG) deletejob(jobs,pid);
        }else if(WIFSTOPPED(status)){                           //if child was stopped then change it's status to stopped
            job_t* temp = getjobpid(jobs,pid);
            temp->state = ST;
            printf("Job [%d] (%d) stopped by signal %d\n",pid2jid(pid),pid,WSTOPSIG(status));
        }
       }
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenever the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) {
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        kill(-pid, SIGINT); // Send SIGINT to the entire foreground process group
    }
    return;
}


/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) {
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        kill(-pid, SIGTSTP); // Send SIGTSTP to the entire foreground process group
    }
    return;
}

///////////////////// END of Zain ////////////////////////////////////////////////////


/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* freejid - Returns smallest free job ID */
int freejid(struct job_t *jobs) {
    int i;
    int taken[MAXJOBS + 1] = {0};
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid != 0) 
        taken[jobs[i].jid] = 1;
    for (i = 1; i <= MAXJOBS; i++)
        if (!taken[i])
            return i;
    return 0;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
    int i;
    
    if (pid < 1)
        return 0;
    int free = freejid(jobs);
    if (!free) {
        printf("Tried to create too many jobs\n");
        return 0;
    }
    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = free;
            strcpy(jobs[i].cmdline, cmdline);
            if(verbose){
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    return 0; /*suppress compiler warning*/
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid) {
            return jobs[i].jid;
    }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) {
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state) {
                case BG: 
                    printf("Running ");
                    break;
                case FG: 
                    printf("Foreground ");
                    break;
                case ST: 
                    printf("Stopped ");
                    break;
                default:
                    printf("listjobs: Internal error: job[%d].state=%d ", 
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message and terminate
 */
void usage(void) {
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}


/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}




