#include "systemcalls.h"
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret = system(cmd);
    if (ret == -1 || WEXITSTATUS(ret) != 0) {

        //provide the user with some information about the failure using errno
        perror("system");
        
        return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork");
            va_end(args);
            return false;
        case 0:
            /*
             * Child process: replace this process with the requested program.
             * `execv` does not search PATH — command[0] must be an absolute
             * path. If execv fails we print an error and exit the child with
             * a non-zero status so the parent can observe the failure.
             */
            if (execv(command[0], command) == -1) {
                perror("execv");
                _exit(1);
            }
            /* not reached */
            break;
        default: {
            /*
             * Parent process: wait for the child to terminate and interpret
             * its exit status. Only return true for a normal exit with
             * status 0. `va_end` must be called before returning.
             */
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                va_end(args);
                return false;
            }
            int success = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
            if (WIFEXITED(status)) {
                printf("Child exited with status %d\n", WEXITSTATUS(status));
            }
            va_end(args);
            return success ? true : false;
        }
    }
    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    pid_t pid;
    /* Open the output file (rw for owner, and read for group/others). */
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd < 0) {
        perror("open");
        va_end(args);
        return false;
    }

    switch (pid = fork()) {
        case -1:
            perror("fork");
            close(fd);
            va_end(args);
            return false;
        case 0:
            /*
             * Child: redirect stdout to the file and exec the command. On
             * failure we must exit the child with a non-zero status.
             */
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                _exit(1);
            }
            close(fd);
            if (execv(command[0], command) == -1) {
                perror("execv");
                _exit(1);
            }
            /* not reached */
            break;
        default: {
            /*
             * Parent: close the file descriptor (it's duplicated in the child),
             * wait for the child and return true only for a successful exit.
             */
            close(fd);
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                va_end(args);
                return false;
            }
            int success = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
            if (WIFEXITED(status)) {
                printf("Child exited with status %d\n", WEXITSTATUS(status));
            }
            va_end(args);
            return success ? true : false;
        }
    }
    return false;
}
