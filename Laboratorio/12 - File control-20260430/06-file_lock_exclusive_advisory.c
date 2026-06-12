/* Simple examle of exclusive (write) and advisory file locking 
 * using the POSIX-compliant fcntl(). Locking with fcntl()
 * requires a file descriptor, not a FILE*, hence we open the
 * file using open() instead of fopen()
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> //file control

int main() {
    int fd = open("data.txt", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct flock lock; 
    /* this comes from fcntl.h and includes the following members:
     * short  l_type   Type of lock; F_RDLCK, F_WRLCK, F_UNLCK
     * short  l_whence Flag for starting offset 
     * off_t  l_start  Relative offset in bytes 
     * off_t  l_len    Size; if 0 then until EOF 
     * pid_t  l_pid    Process ID of the process holding the lock
     */ 

    lock.l_type = F_WRLCK;     	// This will be an exclusive lock
    lock.l_whence = SEEK_SET;  	// The offset l_start will be relative to the beginning of the file
    lock.l_start = 0;	 	// The offset is 0 (so, lock the file from its start
    lock.l_len = 0;            	// Lock l_len bytes from the lock offset. 0 is interpreted as: lock until the end of the file

    printf("Trying to acquire exclusive lock...\n");

    /* F_SETLK -> when a conflicting lock is detected, fcntl() immediately exits with -1
     * F_SETLKW -> when a conflicting lock is detected, fcntl() blocks the process until the lock is released
     */

    if (fcntl(fd, F_SETLK, &lock) == -1) {  // Could not set the lock, has to be already set by someone else
        perror("fcntl");
	fcntl(fd, F_GETLK, &lock); //out of curiosity: who owns the lock? After this call, if a lock is set, lock.l_pid is now populated
	
    	printf("My PID: %d -- PID of file lock owner: %d\n",getpid(),lock.l_pid);

        close(fd);
        return 1;
    }

    printf("My PID: %d -- Exclusive lock acquired. Editing file...\n",getpid());

    sleep(10);  // Simulate work

    lock.l_type = F_UNLCK;     //say to lock that you want to release the lock
    fcntl(fd, F_SETLK, &lock); //actually release the lock
    printf("Exclusive lock released.\n");

    close(fd);
    return 0;
}

