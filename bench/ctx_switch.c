#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		int cpu, int group_fd, unsigned long flags)
{
  int ret;

  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

int
perf_event_start()
{
  int fd;
  struct perf_event_attr pe;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = PERF_COUNT_HW_INSTRUCTIONS;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening leader %llx\n", pe.config);
    exit(EXIT_FAILURE);
  }

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  return fd;
}

void
perf_event_stop(int fd)
{
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  close(fd);
}

#define BUF_LEN 1

struct thread_arg {
	int niters;
	int rfd;
	int wfd;
};

void handle_error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void *thread_main(void *arg)
{
	struct thread_arg *targ = (struct thread_arg*)arg;

	char buf[BUF_LEN] = {1};
	int i;
	int niters = targ->niters;
	int rfd = targ->rfd;
	int wfd = targ->wfd;
	ssize_t n;

	//int fd = perf_event_start();

	for (i = 0; i < niters; i++) {
		n = read(rfd, buf, BUF_LEN);
		if (n != BUF_LEN)
			handle_error("thread read");

		n = write(wfd, buf, BUF_LEN);
		if (n != BUF_LEN)
			handle_error("thread write");
	}

	//perf_event_stop(fd);

	return EXIT_SUCCESS;
}

void init_threads(int n, struct thread_arg *args, pthread_t *threads)
{
	int i;
	int ret;
	pthread_attr_t attr;

	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		errno = ret;
		handle_error("pthread_attr_init");
	}

	for (i = 0; i < n; i++) {
		ret = pthread_create(&threads[i], &attr, &thread_main, &args[i]);
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

void destroy_threads(int n, pthread_t *threads)
{
	int i;
	int ret;

	for (i = 0; i < n; i++) {
		ret = pthread_join(threads[i], NULL);
		if (ret != 0) {
			errno = ret;
			handle_error("pthread_join");
		}
	}
}

void init_args(int nswitch, int nthreads, int nwarmups,
	struct thread_arg *args)
{
	int i;
	int niters = ((nswitch + (nthreads - 1)) / nthreads) + nwarmups;
	int pipefd[2];

	for (i = 0; i < nthreads; i++) {
		if (pipe(pipefd) != 0)
			handle_error("pipe");

		args[i].niters = niters;
		args[i].wfd = pipefd[1];
		args[(i+1) % nthreads].rfd = pipefd[0];
	}
}

void destroy_args(int n, struct thread_arg *args)
{
	int i;

	for (i = 0; i < n; i++) {
		close(args[i].rfd);
		close(args[i].wfd);
	}
}

void warmup(int rfd, int wfd, int nwarmups)
{
	char buf[BUF_LEN] = {1};
	int i;

	for (i = 0; i < nwarmups; i++) {
		if (write(wfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("write");

		if (read(rfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("read");
	}
}

void run_bench(int rfd, int wfd, int nwarmups, int nswitch, int nthreads)
{
	char buf[BUF_LEN] = {1};
	int i;
	struct timespec end;
	struct timespec start;
	uint64_t delta;

	//int fd = perf_event_start();

	warmup(rfd, wfd, nwarmups);

	clock_gettime(CLOCK_REALTIME, &start);
	for (i = 0; i < nswitch; i += nthreads) {
		if (write(wfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("write");

		if (read(rfd, buf, BUF_LEN) != BUF_LEN)
			handle_error("read");
	}
	clock_gettime(CLOCK_REALTIME, &end);

	//perf_event_stop(fd);

	delta = 1e9 * (end.tv_sec - start.tv_sec)
		+ end.tv_nsec - start.tv_nsec;

	printf("nswitch = %d, delta = %ldns, avg = %ldns\n", i, delta, delta/i);
}

void print_usage(char *name)
{
	fprintf(stderr, "Usage: %s [-w nwarmups] -n nswitches -t nthreads\n",
		name);

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	char c;
	int nswitch = -1;
	int nthreads = -1;
	int nwarmups = 1;
	pthread_t *threads;
	struct thread_arg *args;

	opterr = 0;

	while ((c = getopt(argc, argv, "w:n:t:")) != -1)
		switch (c) {
		case 'n':
			nswitch = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'w':
			nwarmups = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
		}


	if (nswitch == -1 || nthreads == -1)
		print_usage(argv[0]);

	printf("pid: %d\n", getpid());

	args = calloc((size_t)nthreads, sizeof(struct thread_arg));
	if (args == NULL)
		handle_error("calloc");

	threads = calloc((size_t)nthreads - 1, sizeof(pthread_t));
	if (threads == NULL)
		handle_error("calloc");

	init_args(nswitch, nthreads, nwarmups, args);
	init_threads(nthreads - 1, &args[1], threads); // Thread 0 == main

	run_bench(args[0].rfd, args[0].wfd, nwarmups, nswitch, nthreads);

	destroy_threads(nthreads - 1, threads);
	destroy_args(nthreads, args);

	free(threads);
	free(args);

	return EXIT_SUCCESS;
}
