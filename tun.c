#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <linux/if_tun.h>

#ifdef ANDROID
#define TUNNAME "/dev/tun"
#else
#define TUNNAME "/dev/net/tun"
#endif

#define SIN_ADDR(x) (((struct sockaddr_in *) (&(x)))->sin_addr.s_addr)

/*
 * SET_SA_FAMILY - set the sa_family field of a struct sockaddr,
 * if it exists.
 */
#define SET_SA_FAMILY(addr, family)  \
    memset ((char *) &(addr), '\0', sizeof(addr)); \
    addr.sa_family = (family);

const uint32_t pppIfIp = 0x0B000001; // 11.0.0.1
const uint32_t pppIfMask = 0xFF000000; // 255.0.0.0
const uint32_t pppIfBcastIp = 0x0BFFFFFF; // 11.255.255.255

int tun_alloc(char *dev, int flags)
{
     struct ifreq ifr;
     int fd, err;

     /* Arguments taken by the function:
      *
      * char *dev: the name of an interface (or '\0'). MUST have enough
      *   space to hold the interface name if '\0' is passed
      * int flags: interface flags (eg, IFF_TUN etc.)
      */

     /* open the clone device */
     if( (fd = open(TUNNAME, O_RDWR)) < 0 )
     {
          return fd;
     }

     /* preparation of the struct ifr, of type "struct ifreq" */
     memset(&ifr, 0, sizeof(ifr));
     ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

     if (dev && *dev)
     {
          /* if a device name was specified, put it in the structure; otherwise,
           * the kernel will try to allocate the "next" device of the
           * specified type */
          strncpy(ifr.ifr_name, dev, IFNAMSIZ);
     }

     /* try to create the device */
     if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 )
     {
          perror("TUNSETIFF");
          close(fd);
          return err;
     }

     /*
     if ( (err = ioctl(fd, TUNSETPERSIST, 1)) < 0 )
     {
          perror("TUNSETPERSIST");
          close(fd);
          return err;
     }
     */

     if (dev)
     {
          /* if the operation was successful, write back the name of the
           * interface to the variable "dev", so the caller can know
           * it. Note that the caller MUST reserve space in *dev (see calling
           * code below) */
          strncpy(dev, ifr.ifr_name, IFNAMSIZ);
     }

     /* this is the special file descriptor that the caller will use to talk
      * with the virtual interface */
     return fd;
}

int setup_ip_address(char *devname)
{
     struct ifreq ifr;
     int fd = 0, ret = 0;
     while(1)
     {
          // Get an internet socket for doing socket ioctls
          ret = fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
          if (ret < 0)
          {
               perror("Open socket ioctls");
               break;
          }

          memset(&ifr, 0, sizeof(ifr));
          SET_SA_FAMILY (ifr.ifr_addr,    AF_INET);
          SET_SA_FAMILY (ifr.ifr_dstaddr, AF_INET);
          SET_SA_FAMILY (ifr.ifr_netmask, AF_INET);

          // Name the interface
          strncpy(ifr.ifr_name, devname, IFNAMSIZ);

          // Set our IP address
          SIN_ADDR(ifr.ifr_addr) = htonl(pppIfIp);
          ret = ioctl(fd, SIOCSIFADDR, &ifr);
          if (ret < 0)
          {
               perror("ioctl(SIOCSIFADDR)");
               break;
          }

          // Set the gateway address
          SIN_ADDR(ifr.ifr_dstaddr) = htonl(pppIfIp + 1);
          ret = ioctl(fd, SIOCSIFDSTADDR, &ifr);
          if (ret < 0)
          {
               perror("ioctl(SIOCSIFDSTADDR)");
               break;
          }

          // Set network mask
          /*
          SIN_ADDR(ifr.ifr_netmask) = htonl(pppIfMask);
          ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
          if (ret < 0)
          {
               perror("ioctl(SIOCSIFNETMASK)");
               break;
          }
          */

          // Up interface
          ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
          if (ret < 0)
          {
               perror("ioctl(SIOCGIFFLAGS)");
               break;
          }

          strncpy(ifr.ifr_name, devname, IFNAMSIZ);
          ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
          ret = ioctl(fd , SIOCSIFFLAGS, &ifr);
          if (ret < 0)
          {
               perror("ioctl(SIOCSIFFLAGS)");
               break;
          }
          break;
     }/*while(1)*/
     if (fd > 0)
     {
          close(fd);
     }
     return ret;
}

int main()
{
  char buffer[2048];
  char tun_name[IFNAMSIZ] = "";
  int nread, tun_fd;

  printf("Starting...\n");

  /* Connect to the device */
  tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface */

  if (tun_fd < 0)
  {
    perror("Allocating interface");
    exit(1);
  }
  else
  {
       if ( setup_ip_address(tun_name) < 0)
       {
            exit(1);
       }
       printf("connected to %s on fd: %i\n", tun_name, tun_fd);
  }

  /* Now read data coming from the kernel */
  while (1)
  {
    /* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
    nread = read(tun_fd, buffer, sizeof(buffer));
    if (nread < 0)
    {
      perror("Reading from interface");
      close(tun_fd);
      exit(1);
    }
    /* Do whatever with the data */
    printf("Read %d bytes from device %s\n", nread, tun_name);
  }
  return EXIT_SUCCESS;
}
