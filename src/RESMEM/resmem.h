/*-----------------------------------------------------------------------------
(C) Copyright Aveillant Ltd 2012

FILE
    resmem.h

ORIGINAL AUTHOR
    Peter Wurmsdobler

DESCRIPTION
    This module constitutes the driver for the reserved memory into which
    sensor data are streamed to, saved from and loaded into again.

-----------------------------------------------------------------------------*/
#ifndef RESMEM_H
#define RESMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
System level includes
-----------------------------------------------------------------------------*/
#include <asm/ioctl.h>

/*-----------------------------------------------------------------------------
Public Defines
-----------------------------------------------------------------------------*/
#define RESMEM_NAME         "resmem"
#define RESMEM_DEV             "/dev/resmem"
#define RESMEM_HEADER_SIZE    (4*1024*1024)
/*-----------------------------------------------------------------------------
IOCTL Defines
-----------------------------------------------------------------------------*/
#define RESMEM_IOC_MAGIC 0xEE
#define RESMEM_IOC_MAGIC 0xEE
#define RESMEM_IOC_HWADDR _IOR(RESMEM_IOC_MAGIC, 0, unsigned long int *)
#define RESMEM_IOC_LENGTH _IOR(RESMEM_IOC_MAGIC, 1, unsigned long int *)
#define RESMEM_IOC_MAXNR   _IO(RESMEM_IOC_MAGIC, 2)

/*-----------------------------------------------------------------------------
Public Data Types
-----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* RESMEM_H */
