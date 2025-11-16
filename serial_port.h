
#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include "include.h"
#include "define.h"
#include "struct.h"

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>

/*
 * Ring Buffer - MUST be a power of 2
 */
#define RING_BUF ( 1 << 12 )
#define RING_MSK ( RING_BUF - 1 )

class SerialPort
{

  public:

    int fd ;

    SerialPort( void ) ;
    ~SerialPort( void ) ;

    int Open( const char *dev ) ;
    int Open( const char *dev, int baud, int dbits, char parity, int sbits ) ;
    int Close( void ) ;

    int Peek( long int timeout, char *buf, int *len ) ;
    int Read( long int timeout, char *buf, int *len ) ;
    int Write( long int rate, char *buf, int *len ) ;

    void Buf_Asc( char *buf, int *len ) ;
    void Buf_Hex( char *buf, int *len ) ;

    void Rng_Asc( void ) ;
    void Rng_Hex( void ) ;

  private:

    char buf[RING_BUF] ;

    struct {
      char buf[RING_BUF] ;
      unsigned int head ;
      unsigned int tail ;
    } ring ;

    struct stat stat_s ;    /* device stats */

    struct termios old_ts ; /* Original Terminal Settings */
    struct termios new_ts ; /* Current Terminal Settings */

    int Set_Baud( struct termios *t, int baud ) ;
    int Set_DBits( struct termios *t, int dbits ) ;
    int Set_Parity( struct termios *t, char parity ) ;
    int Set_SBits( struct termios *t, int sbits ) ;

    void Ring( long int timeout ) ;

} ;

#endif /* SERIAL_PORT_H */

