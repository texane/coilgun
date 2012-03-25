#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "serial.h"

static int ser_set_dtr_rts(int fd, int is_on)
{
  unsigned int	ctl;
  int           r;

  r = ioctl(fd, TIOCMGET, &ctl);
  if (r < 0) {
    perror("ioctl(\"TIOCMGET\")");
    return -1;
  }

  if (is_on) {
    /* Set DTR and RTS */
    ctl |= (TIOCM_DTR | TIOCM_RTS);
  }
  else {
    /* Clear DTR and RTS */
    ctl &= ~(TIOCM_DTR | TIOCM_RTS);
  }

  r = ioctl(fd, TIOCMSET, &ctl);
  if (r < 0) {
    perror("ioctl(\"TIOCMSET\")");
    return -1;
  }

  return 0;
}

int main(int ac, char** av)
{
  static const serial_conf_t conf = { 9600, 8, SERIAL_PARITY_DISABLED, 1 };

  serial_handle_t h;
  uint8_t buf[32];

  if (serial_open(&h, "/dev/ttyACM0") == -1)
  {
    printf("serial_open() == -1\n");
    return -1;
  }

  if (serial_set_conf(&h, &conf) == -1)
  {
    printf("serial_set_conf() == -1\n");
    goto on_error;
  }

  ser_set_dtr_rts(h.fd, 0);
  usleep(50 * 1000);
  ser_set_dtr_rts(h.fd, 1);
  usleep(50 * 1000);

  /* dont remove, arduino locked otherwise */
  usleep(1000000);

  if (serial_writen(&h, "f", 1))
  {
    printf("serial_writen() == -1\n");
    goto on_error;
  }

  printf("readn()\n");

  if (serial_readn(&h, buf, 8))
  {
    printf("serial_readn() == -1\n");
    goto on_error;
  }
  buf[8] = 0;

  printf("%s\n", buf);

 on_error:
  serial_close(&h);
  return 0;
}
