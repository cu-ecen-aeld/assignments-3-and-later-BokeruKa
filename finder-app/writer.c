#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    //open context for syslog
    openlog(NULL, 0, LOG_USER);

    //check if correct number of arguments (2) are provided
    if (argc != 3) { //we are checking against 3 because argv[0] is the program name
        syslog(LOG_ERR, "Usage: Writer needs to be called with a filepath and a write string");
        return 1;
    }

    const char *filepath = argv[1]; 
    const char *writestr = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, filepath);

    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Error opening file for writing: %s", filepath);
        return 1;
    }

    if (fputs(writestr, file) == EOF) {
        syslog(LOG_ERR, "Error writing string to file");
        fclose(file);
        return 1;
    }

    if (fclose(file) != 0) {
        syslog(LOG_ERR, "Error closing file");
        return 1;
    }

    closelog(); //https://linux.die.net/man/3/syslog says this is optional, but still feels cleaner this way

    return 0;
}