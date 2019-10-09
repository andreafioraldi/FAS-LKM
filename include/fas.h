#ifndef __FAS_H__
#define __FAS_H__

#include <linux/limits.h>

#define FAS_VERSION "0.1"

#define FAS_FILE_NAME "/dev/fas"

#define FAS_IOCTL_MAGIC 52634

struct fas_open_args {

  char pathname[PATH_MAX];
  int flags;
  mode_t mode;
};

#define FAS_IOCTL_NOP _IO(FAS_IOCTL_MAGIC, 1)
#define FAS_IOCTL_OPEN _IOR(FAS_IOCTL_MAGIC, 2, struct fas_open_args*)

#endif