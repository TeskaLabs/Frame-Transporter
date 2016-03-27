#include "all.h"

/***
  This file is modified version of dfork.c from libdaemon.

  Copyright 2003-2008 Lennart Poettering

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

***/

///

static int _daemon_retval_pipe[2] = { -1, -1 };

static int _null_open(int f, int fd)
{
	int fd2;

	if ((fd2 = open("/dev/null", f)) < 0)
		return -1;

	if (fd2 == fd)
		return fd;

	if (dup2(fd2, fd) < 0)
		return -1;

	close(fd2);
	return fd;
}

static ssize_t atomic_read(int fd, void *d, size_t l)
{
	ssize_t t = 0;

	while (l > 0)
	{
		ssize_t r;

		if ((r = read(fd, d, l)) <= 0)
		{

			if (r < 0)
				return t > 0 ? t : -1;
			else
				return t;
		}

		t += r;
		d = (char*) d + r;
		l -= r;
	}

	return t;
}

static ssize_t atomic_write(int fd, const void *d, size_t l)
{
	ssize_t t = 0;

	while (l > 0)
	{
		ssize_t r;

		if ((r = write(fd, d, l)) <= 0)
		{

			if (r < 0)
				return t > 0 ? t : -1;
			else
				return t;
		}

		t += r;
		d = (const char*) d + r;
		l -= r;
	}

	return t;
}

static int move_fd_up(int *fd)
{
	assert(fd);

	while (*fd <= 2)
	{
		if ((*fd = dup(*fd)) < 0)
		{
			L_ERROR_ERRNO(errno, "dup()");
			return -1;
		}
	}

	return 0;
}

static void sigchld(int s)
{
}

///

pid_t daemonise(void)
{
	pid_t pid;
	int pipe_fds[2] = {-1, -1};
	struct sigaction sa_old, sa_new;
	sigset_t ss_old, ss_new;
	int saved_errno;

	memset(&sa_new, 0, sizeof(sa_new));
	sa_new.sa_handler = sigchld;
	sa_new.sa_flags = SA_RESTART;

	if (sigemptyset(&ss_new) < 0)
	{
		L_ERROR_ERRNO(errno, "sigemptyset() failed");
		return (pid_t) -1;
	}

	if (sigaddset(&ss_new, SIGCHLD) < 0)
	{
		L_ERROR_ERRNO(errno, "sigaddset() failed");
		return (pid_t) -1;
	}

	if (sigaction(SIGCHLD, &sa_new, &sa_old) < 0)
	{
		L_ERROR_ERRNO(errno, "sigaction() failed");
		return (pid_t) -1;
	}

	if (sigprocmask(SIG_UNBLOCK, &ss_new, &ss_old) < 0)
	{
		L_ERROR_ERRNO(errno, "sigprocmask() failed");

		saved_errno = errno;
		sigaction(SIGCHLD, &sa_old, NULL);
		errno = saved_errno;

		return (pid_t) -1;
	}

	if (pipe(pipe_fds) < 0)
	{
		L_ERROR_ERRNO(errno, "pipe() failed");

		saved_errno = errno;
		sigaction(SIGCHLD, &sa_old, NULL);
		sigprocmask(SIG_SETMASK, &ss_old, NULL);
		errno = saved_errno;

		return (pid_t) -1;
	}

	if ((pid = fork()) < 0)
	{ /* First fork */
		L_ERROR_ERRNO(errno, "First fork() failed");

		saved_errno = errno;
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		sigaction(SIGCHLD, &sa_old, NULL);
		sigprocmask(SIG_SETMASK, &ss_old, NULL);
		errno = saved_errno;

		return (pid_t) -1;

	}

	else if (pid == 0)
	{
		pid_t dpid;

		/* First child. Now we are sure not to be a session leader or
		 * process group leader anymore, i.e. we know that setsid()
		 * will succeed. */

		if (close(pipe_fds[0]) < 0)
		{
			L_ERROR_ERRNO(errno, "close() failed");
			goto fail;
		}

		/* Move file descriptors up*/
		if (move_fd_up(&pipe_fds[1]) < 0)
			goto fail;

		if (_daemon_retval_pipe[0] >= 0 && move_fd_up(&_daemon_retval_pipe[0]) < 0)
			goto fail;
		if (_daemon_retval_pipe[1] >= 0 && move_fd_up(&_daemon_retval_pipe[1]) < 0)
			goto fail;

		if (_null_open(O_RDONLY, 0) < 0)
		{
			L_ERROR_ERRNO(errno,"Failed to open /dev/null for STDIN");
			goto fail;
		}

		if (_null_open(O_WRONLY, 1) < 0)
		{
			L_ERROR_ERRNO(errno, "Failed to open /dev/null for STDOUT");
			goto fail;
		}

		if (_null_open(O_WRONLY, 2) < 0)
		{
			L_ERROR_ERRNO(errno, "Failed to open /dev/null for STDERR");
			goto fail;
		}

		/* Create a new session. This will create a new session and a
		 * new process group for us and we will be the ledaer of
		 * both. This should always succeed because we cannot be the
		 * process group leader because we just forked. */
		if (setsid() < 0)
		{
			L_ERROR_ERRNO(errno, "setsid() failed");
			goto fail;
		}

/*
		umask(0077);

		if (chdir("/") < 0)
		{
			L_ERROR_ERRNO(errno, "chdir() failed");
			goto fail;
		}
*/

		if ((pid = fork()) < 0)
		{ /* Second fork */
			L_ERROR_ERRNO(errno, "Second fork() failed");
			goto fail;
		}

		else if (pid == 0)
		{
			/* Second child. Our father will exit right-away. That way
			 * we can be sure that we are a child of init now, even if
			 * the process which spawned us stays around for a longer
			 * time. Also, since we are no session leader anymore we
			 * can be sure that we will never acquire a controlling
			 * TTY. */

			if (sigaction(SIGCHLD, &sa_old, NULL) < 0)
			{
				L_ERROR_ERRNO(errno, "close() failed");
				goto fail;
			}

			if (sigprocmask(SIG_SETMASK, &ss_old, NULL) < 0)
			{
				L_ERROR_ERRNO(errno, "sigprocmask() failed");
				goto fail;
			}

			if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
			{
				L_ERROR_ERRNO(errno, "signal(SIGTTOU, SIG_IGN) failed");
				goto fail;
			}

			if (signal(SIGTTIN, SIG_IGN) == SIG_ERR)
			{
				L_ERROR_ERRNO(errno, "signal(SIGTTIN, SIG_IGN) failed");
				goto fail;
			}

			if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
			{
				L_ERROR_ERRNO(errno, "signal(SIGTSTP, SIG_IGN) failed");
				goto fail;
			}

			dpid = getpid();
			if (atomic_write(pipe_fds[1], &dpid, sizeof(dpid)) != sizeof(dpid))
			{
				L_ERROR_ERRNO(errno, "write() failed");
				goto fail;
			}

			if (close(pipe_fds[1]) < 0)
			{
				L_ERROR_ERRNO(errno, "close() failed");
				goto fail;
			}

			return 0;

		}

		else
		{
			/* Second father */
			close(pipe_fds[1]);
			_exit(0);
		}

	fail:
		dpid = (pid_t) -1;

		if (atomic_write(pipe_fds[1], &dpid, sizeof(dpid)) != sizeof(dpid))
			L_ERROR_ERRNO(errno, "Failed to write error PID");

		close(pipe_fds[1]);
		_exit(0);

	}

	else
	{
		/* First father */
		pid_t dpid;

		close(pipe_fds[1]);

		if (waitpid(pid, NULL, WUNTRACED) < 0)
		{
			saved_errno = errno;
			close(pipe_fds[0]);
			sigaction(SIGCHLD, &sa_old, NULL);
			sigprocmask(SIG_SETMASK, &ss_old, NULL);
			errno = saved_errno;
			return -1;
		}

		sigprocmask(SIG_SETMASK, &ss_old, NULL);
		sigaction(SIGCHLD, &sa_old, NULL);

		if (atomic_read(pipe_fds[0], &dpid, sizeof(dpid)) != sizeof(dpid))
		{
			L_ERROR("Failed to read daemon PID.");
			dpid = (pid_t) -1;
			errno = EINVAL;
		}

		else if (dpid == (pid_t) -1)
			errno = EIO;

		saved_errno = errno;
		close(pipe_fds[0]);
		errno = saved_errno;

		L_DEBUG("Daemonised");
		pidfile_set_filename(NULL); // Prevent removal od the PID file

		if (libsccmn_config.ev_loop != NULL)
			ev_loop_fork(libsccmn_config.ev_loop);

		return dpid;
	}
}

///

void pidfile_set_filename(const char * fname)
{
	if (libsccmn_config.pid_file != NULL)
	{
		free((void *)libsccmn_config.pid_file);
		libsccmn_config.pid_file = NULL;
	}

	libsccmn_config.pid_file = (fname != NULL) ? strdup(fname) : NULL;
}


static int pidfile_lock(int fd, int enable)
{
	struct flock f;

	memset(&f, 0, sizeof(f));
	f.l_type = enable ? F_WRLCK : F_UNLCK;
	f.l_whence = SEEK_SET;
	f.l_start = 0;
	f.l_len = 0;

	if (fcntl(fd, F_SETLKW, &f) < 0) {

		if (enable && errno == EBADF) {
			f.l_type = F_RDLCK;

			if (fcntl(fd, F_SETLKW, &f) >= 0)
				return 0;
		}

		L_ERROR_ERRNO(errno, "fcntl(F_SETLKW) failed");
		return -1;
	}

	return 0;
}


bool pidfile_create(void)
{
	int fd = -1;
	int ret = -1;
	int locked = -1;
	char t[64];
	ssize_t l;
	mode_t u;

	u = umask(022);


	if (libsccmn_config.pid_file == NULL)
	{
		// Pid file will not be created because it is not specified
		return true;
	}

	fd = open(libsccmn_config.pid_file, O_CREAT | O_RDWR, 0644);
	if (fd < 0)
	{
		L_ERROR_ERRNO(errno, "pidfile_create/open(%s)", libsccmn_config.pid_file);
		return false;
	}

	locked = pidfile_lock(fd, 1);
	if (locked < 0)
	{
		int saved_errno = errno;
		unlink(libsccmn_config.pid_file);
		errno = saved_errno;
		goto finish;
	}

	snprintf(t, sizeof(t), "%lu\n", (unsigned long) getpid());

	l = strlen(t);
	if (write(fd, t, l) != l)
	{
		int saved_errno = errno;
		L_ERROR_ERRNO(saved_errno, "pidfile_create / write()");
		unlink(libsccmn_config.pid_file);
		errno = saved_errno;
		goto finish;
	}

	ret = 0;

finish:

	if (fd >= 0)
	{
		int saved_errno = errno;

		if (locked >= 0)
			pidfile_lock(fd, 0);

		close(fd);
		errno = saved_errno;
	}

	umask(u);

	return ret == 0;
}


bool pidfile_remove(void)
{
	if (libsccmn_config.pid_file == NULL)
		return true;

	int rc = unlink(libsccmn_config.pid_file);
	if (rc != 0)
	{
		L_ERROR_ERRNO(errno, "pidfile_remove / unlink(%s)", libsccmn_config.pid_file);
	}

	return rc == 0;
}

///

pid_t pidfile_is_running(void)
{
    static char txt[256];
    int fd = -1, locked = -1;
    pid_t ret = (pid_t) -1, pid;
    ssize_t l;
    long lpid;
    char *e = NULL;


	if (libsccmn_config.pid_file == NULL)
	{
		L_ERROR("Pid file location is not specified.");
        goto finish;
    }

    if ((fd = open(libsccmn_config.pid_file, O_RDWR, 0644)) < 0) {
        if ((fd = open(libsccmn_config.pid_file, O_RDONLY, 0644)) < 0) {
            if (errno != ENOENT)
                L_ERROR_ERRNO(errno, "Failed to open PID file");

            goto finish;
        }
    }

    if ((locked = pidfile_lock(fd, 1)) < 0)
        goto finish;

    if ((l = read(fd, txt, sizeof(txt)-1)) < 0) {
        int saved_errno = errno;
        L_ERROR_ERRNO(errno, "daemon_pid_file_is_running / read()");
        unlink(libsccmn_config.pid_file);
        errno = saved_errno;
        goto finish;
    }

    txt[l] = 0;
    txt[strcspn(txt, "\r\n")] = 0;

    errno = 0;
    lpid = strtol(txt, &e, 10);
    pid = (pid_t) lpid;

    if (errno != 0 || !e || *e || (long) pid != lpid) {
        L_WARN("PID file corrupt, removing. (%s)", libsccmn_config.pid_file);
        unlink(libsccmn_config.pid_file);
        errno = EINVAL;
        goto finish;
    }

    if (kill(pid, 0) != 0 && errno != EPERM) {
        int saved_errno = errno;
        L_ERROR_ERRNO(errno, "Process %lu died", (unsigned long) pid);
        unlink(libsccmn_config.pid_file);
        errno = saved_errno;
        goto finish;
    }

    ret = pid;

finish:

    if (fd >= 0) {
        int saved_errno = errno;
        if (locked >= 0)
            pidfile_lock(fd, 0);
        close(fd);
        errno = saved_errno;
    }

    return ret;
}
