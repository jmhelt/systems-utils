#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BUF_LEN 1

struct targ {
	int rfd;
	int wfd;
};

void sighandler(int signum)
{
	pthread_exit(EXIT_SUCCESS);
}

void handle_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void *thread_main(void *arg)
{
	struct targ *targ = (struct targ*)arg;
	ssize_t n;
	int rfd = targ->rfd;
	int wfd = targ->wfd;
	char buf[BUF_LEN];

	while (true) {
		n = read(rfd, buf, BUF_LEN);
		if (n != BUF_LEN)
			handle_error("thread read");

		n = write(wfd, buf, BUF_LEN);
		if (n != BUF_LEN)
			handle_error("thread write");
	}

	return EXIT_SUCCESS;
}

void init_thrs(int n, struct targ *args, pthread_t *thrs)
{
	int i, ret;
	pthread_attr_t attr;
	struct sigaction sa;

	sa.sa_handler = sighandler;
	ret = sigaction(SIGUSR1, &sa, NULL);
	if (ret)
		handle_error("sigaction");

	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		errno = ret;
		handle_error("pthread_attr_init");
	}

	for (i = 0; i < n; i++) {
		ret = pthread_create(&thrs[i], &attr, &thread_main, &args[i]);
		if (ret != 0) {
			errno = ret;
			handle_error("pthread_create");
		}
	}

	ret = pthread_attr_destroy(&attr);
	if (ret != 0) {
		errno = ret;
		handle_error("pthread_attr_destroy");
	}
}

void destroy_thrs(int n, pthread_t *thrs)
{
	int i, ret;

	for (i = 0; i < n; i++) {
		ret = pthread_kill(thrs[i], SIGUSR1);
		if (ret != 0) {
			errno = ret;
			handle_error("pthread_kill");
		}

		ret = pthread_join(thrs[i], NULL);
		if (ret != 0) {
			errno = ret;
			handle_error("pthread_join");
		}
	}
}

void init_args(int n, struct targ *args)
{
	int i;
	int pipefd[2];

	for (i = 0; i < n; i++) {
		if (pipe(pipefd) != 0)
			handle_error("pipe");

		args[i].wfd = pipefd[1];
		args[(i+1) % n].rfd = pipefd[0];
	}
}

void destroy_args(int n, struct targ *args)
{
	int i;
	for (i = 0; i < n; i++) {
		close(args[i].rfd);
		close(args[i].wfd);
	}
}

void warmup(int rfd, int wfd)
{
	char buf[BUF_LEN];
	if (write(wfd, buf, BUF_LEN) != BUF_LEN)
		handle_error("write");

	if (read(rfd, buf, BUF_LEN) != BUF_LEN)
		handle_error("read");
}

void run_bench(int rfd, int wfd, int n_ctx, int n_thrs)
{
	char buf[BUF_LEN];
	int i;
	struct timespec start, end;
	uint64_t delta;

	warmup(rfd, wfd);

	clock_gettime(CLOCK_REALTIME, &start);
	for (i = 0; i < n_ctx; i += n_thrs) {
		if (write(wfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("write");

		if (read(rfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("read");
	}
	clock_gettime(CLOCK_REALTIME, &end);

	delta = 1e9 * (end.tv_sec - start.tv_sec)
		+ end.tv_nsec - start.tv_nsec;

	printf("n_ctx = %d, delta = %ldns, avg=%ldns\n", i, delta, delta/i);
}

int main(int argc, char *argv)
{
	int n_ctx = 1000000, n_thrs = 3;
	struct targ *args;
	pthread_t *thrs;

	// Parse n_ctx and n_thrs

	args = calloc(n_thrs, sizeof(struct targ));
	if (args == NULL)
		handle_error("calloc");

	thrs = calloc(n_thrs - 1, sizeof(pthread_t));
	if (thrs == NULL)
		handle_error("calloc");

	init_args(n_thrs, args);
	init_thrs(n_thrs - 1, &args[1], thrs); // Thread 0 == main

	run_bench(args[0].rfd, args[0].wfd, n_ctx, n_thrs);

	destroy_thrs(n_thrs - 1, thrs);
	destroy_args(n_thrs, args);

	return EXIT_SUCCESS;
}
