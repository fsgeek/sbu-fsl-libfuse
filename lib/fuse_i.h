/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#include "fuse.h"
#include "fuse_lowlevel.h"
#include <uuid/uuid.h>

struct mount_opts;

struct fuse_req {
	struct fuse_session *se;
	uint64_t unique;
	int ctr;
#if !defined(OMIT_SBU_FSL_CODE)
	int opcode;
	struct timespec ts1;
	struct timespec ts2;
#endif // SBU_FSL_INSTRUMENT
	pthread_mutex_t lock;
	struct fuse_ctx ctx;
	struct fuse_chan *ch;
	int interrupted;
	unsigned int ioctl_64bit : 1;
	/* BEGIN NICCOLUM CHANGE */
	unsigned int niccolum : 1; // niccolum allocated
	unsigned int niccolum_notify : 1; // notify niccolum
	void *niccolum_req;
	void *niccolum_rsp;
	struct fuse_req *original_fuse_req;
	/* END NICCOLUM CHANGE */	
	union {
		struct {
			uint64_t unique;
		} i;
		struct {
			fuse_interrupt_func_t func;
			void *data;
		} ni;
		struct {
			uuid_t recipient; 
			void *response; 
		} nic;
	} u;
	struct fuse_req *next;
	struct fuse_req *prev;
};

struct fuse_notify_req {
	uint64_t unique;
	void (*reply)(struct fuse_notify_req *, fuse_req_t, fuse_ino_t,
		      const void *, const struct fuse_buf *);
	struct fuse_notify_req *next;
	struct fuse_notify_req *prev;
};

struct fuse_session {
	char *mountpoint;
	volatile int exited;
	int fd;
	struct mount_opts *mo;
	int debug;
	int deny_others;
	struct fuse_lowlevel_ops op;
	int got_init;
	struct cuse_data *cuse_data;
	void *userdata;
	uid_t owner;
	struct fuse_conn_info conn;
	struct fuse_req list;
	struct fuse_req interrupts;
	pthread_mutex_t lock;
	int got_destroy;
	pthread_key_t pipe_key;
	int broken_splice_nonblock;
	uint64_t notify_ctr;
	struct fuse_notify_req notify_list;
	size_t bufsize;
	int error;
#if !defined(OMIT_SBU_FSL_CODE)
	pthread_spinlock_t fsl_lock;
	// TODO: why the magic numbers here?
	long long unsigned int processing[46][33];
	char *statsDir;
#endif // SBU_FSL_INSTRUMENT
	/* BEGIN NICCOLUM CHANGE */
	const char *message_queue_name;
	int message_queue_descriptor;
	/* END NICCOLUM CHANGE */
};

struct fuse_chan {
	pthread_mutex_t lock;
	int ctr;
	int fd;
};

/**
 * Filesystem module
 *
 * Filesystem modules are registered with the FUSE_REGISTER_MODULE()
 * macro.
 *
 */
struct fuse_module {
	char *name;
	fuse_module_factory_t factory;
	struct fuse_module *next;
	struct fusemod_so *so;
	int ctr;
};

/* ----------------------------------------------------------- *
 * Channel interface (when using -o clone_fd)		       *
 * ----------------------------------------------------------- */

/**
 * Obtain counted reference to the channel
 *
 * @param ch the channel
 * @return the channel
 */
struct fuse_chan *fuse_chan_get(struct fuse_chan *ch);

/**
 * Drop counted reference to a channel
 *
 * @param ch the channel
 */
void fuse_chan_put(struct fuse_chan *ch);

struct mount_opts *parse_mount_opts(struct fuse_args *args);
void destroy_mount_opts(struct mount_opts *mo);
void fuse_mount_version(void);
unsigned get_max_read(struct mount_opts *o);
void fuse_kern_unmount(const char *mountpoint, int fd);
int fuse_kern_mount(const char *mountpoint, struct mount_opts *mo);

int fuse_send_reply_iov_nofree(fuse_req_t req, int error, struct iovec *iov,
			       int count);
void fuse_free_req(fuse_req_t req);

void cuse_lowlevel_init(fuse_req_t req, fuse_ino_t nodeide, const void *inarg);

int fuse_start_thread(pthread_t *thread_id, void *(*func)(void *), void *arg);

int fuse_session_receive_buf_int(struct fuse_session *se, struct fuse_buf *buf,
				 struct fuse_chan *ch);
void fuse_session_process_buf_int(struct fuse_session *se,
				  const struct fuse_buf *buf, struct fuse_chan *ch);

struct fuse *fuse_new_31(struct fuse_args *args, const struct fuse_operations *op,
		      size_t op_size, void *private_data);
