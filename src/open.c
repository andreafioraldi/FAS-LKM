#include "fas_private.h"

int fas_ioctl_open(char *filename, int flags, mode_t mode) {

  int r;

  FAS_DEBUG("fas_ioctl_open: %s, %x, %x", filename, flags, mode);

  /* Session temporary files are not a thing. For O_PATH use regular open() */
  if (flags & (O_TMPFILE | O_PATH)) return -EINVAL;

  struct file *a_filp = NULL;
  mm_segment_t oldfs;

  int is_w = (flags & O_WRONLY) | (flags & O_RDWR);
  int a_flags = flags & ~(O_WRONLY | O_RDONLY);
  if (is_w)
    a_flags &= O_RDWR;
  else
    a_flags &= O_RDONLY;

  oldfs = get_fs();
  set_fs(KERNEL_DS);                      /* Set fs related to kernel space */
  a_filp = filp_open(filename, a_flags, mode);
  set_fs(oldfs);

  FAS_DEBUG("fas_ioctl_open: a_filp = %p", a_filp);

  if (IS_ERR(a_filp)) return PTR_ERR(a_filp);

  int fd = get_unused_fd_flags(O_TMPFILE | O_EXCL | O_RDWR);
  if (fd < 0) return fd;

  FAS_DEBUG("fas_ioctl_open: fd = %d", fd);

  oldfs = get_fs();
  set_fs(KERNEL_DS);                      /* Set fs related to kernel space */
  struct file *b_filp = filp_open("/tmp", O_TMPFILE | O_EXCL | O_RDWR, 0644);
  set_fs(oldfs);

  FAS_DEBUG("fas_ioctl_open: b_filp = %p", b_filp);

  if (IS_ERR(b_filp)) {

    filp_close(a_filp, NULL);
    put_unused_fd(fd);
    return PTR_ERR(a_filp);

  }

  // fsnotify_open(b_filp);
  fd_install(fd, b_filp);

  r = fas_filp_copy(a_filp, b_filp);
  if (r < 0) {

    filp_close(a_filp, NULL);
    filp_close(b_filp, NULL);
    return r;

  }

  struct fas_filp_info *finfo =
      kmalloc(sizeof(struct fas_filp_info), GFP_KERNEL);

  finfo->filp = a_filp;
  finfo->orig_f_op = (struct file_operations *)b_filp->f_op;
  finfo->is_w = (is_w != 0);

  FAS_DEBUG("fas_ioctl_open: generated finfo = %p", finfo);
  FAS_DEBUG("fas_ioctl_open:   finfo->filp = %p", finfo->filp);
  FAS_DEBUG("fas_ioctl_open:   finfo->orig_f_op = %p", finfo->orig_f_op);
  FAS_DEBUG("fas_ioctl_open:   finfo->is_w = %d", finfo->is_w);

  radix_tree_insert(&fas_files_tree, (unsigned long)b_filp, finfo);

  struct file_operations *new_fops =
      kmalloc(sizeof(struct file_operations), GFP_KERNEL);

  FAS_DEBUG("fas_ioctl_open: new_fops = %p", new_fops);

  memcpy(new_fops, b_filp->f_op, sizeof(struct file_operations));

  new_fops->release = &fas_file_release;
  new_fops->flush = &fas_file_flush;

  b_filp->f_op = new_fops;

  return fd;

}

