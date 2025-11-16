
#include "define.h"
#include "serial_port.h"

/*
 *
 */
SerialPort::SerialPort( void )
{
  fd = -1 ;
  memset( buf, 0, RING_BUF ) ;
  memset( ring.buf, 0, RING_BUF ) ;
  ring.head = 0 ;
  ring.tail = 0 ;
  return ;
}

/*
 *
 */
SerialPort::~SerialPort( void )
{
  Close() ;
  return ;
}

/*
 *
 */
int SerialPort::Open( const char *dev )
{

  int rv = 0 ;

  /* Ensure dev filename exists */
  rv = stat( dev, &stat_s ) ;
  if( rv ) return( EBADF ) ;

  /* Ensure it's a character device */
  if( ! S_ISCHR( stat_s.st_mode ) ) return( EBADF ) ;

  fd = open( dev, O_RDWR | O_NONBLOCK | O_NOCTTY ) ;

  /* Did we get a valid file descriptor */
  if( fd < 0 ) return( EBADF ) ;

  /* Save Terminal Settings */
  rv = tcgetattr( fd, &old_ts ) ;
  if( rv ) return( EBADF ) ;

  /* Get working copy of Terminal Settings */
  rv = tcgetattr( fd, &new_ts ) ;
  if( rv ) return( EBADF ) ;

  /* Force 'raw' mode */
  cfmakeraw( &new_ts ) ;

  /* Don't convert NL to CR */
  new_ts.c_iflag &= ~ INLCR ;

  /* Don't convert NL to CR */
  new_ts.c_oflag &= ~ ONLCR ;

  /* Change terminal settings */
  rv = tcsetattr( fd, TCSANOW, &new_ts ) ;

  if( rv ) {
    /* Make an effort to revert to old terminal settings */
    tcsetattr( fd, TCSANOW, &old_ts ) ;
    return( EBADF ) ;
  }

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Open( const char *dev, int baud, int dbits, char parity, int sbits )
{

  int rv = 0 ;

  /* Ensure dev filename exists */
  rv = stat( dev, &stat_s ) ;
  if( rv ) return( EBADF ) ;

  /* Ensure it's a character device */
#ifdef PRODUCTION
  if( ! S_ISLNK( stat_s.st_mode ) ) return( EBADF ) ;
#else /* PRODUCTION */
  if( ! S_ISCHR( stat_s.st_mode ) ) return( EBADF ) ;
#endif /* PRODUCTION */

  fd = open( dev, O_RDWR | O_NONBLOCK | O_NOCTTY ) ;

  /* Did we get a valid file descriptor */
  if( fd < 0 ) return( EBADF ) ;

  /* Save Terminal Settings */
  rv = tcgetattr( fd, &old_ts ) ;
  if( rv ) return( EBADF ) ;

  /* Get working copy of Terminal Settings */
  rv = tcgetattr( fd, &new_ts ) ;
  if( rv ) return( EBADF ) ;

  /* Force 'raw' mode */
  cfmakeraw( &new_ts ) ;

  rv = Set_Baud( &new_ts, baud ) ;
  if( rv ) return( EBADF ) ;

  rv = Set_DBits( &new_ts, dbits ) ;
  if( rv ) return( EBADF ) ;

  rv = Set_Parity( &new_ts, parity ) ;
  if( rv ) return( EBADF ) ;

  rv = Set_SBits( &new_ts, sbits ) ;
  if( rv ) return( EBADF ) ;

  /* Don't convert NL to CR */
  new_ts.c_iflag &= ~ INLCR ;

  /* Don't convert NL to CR */
  new_ts.c_oflag &= ~ ONLCR ;

  /* Change terminal settings */
  rv = tcsetattr( fd, TCSANOW, &new_ts ) ;

  if( rv ) {
    /* Make an effort to revert to old terminal settings */
    tcsetattr( fd, TCSANOW, &old_ts ) ;
    return( EBADF ) ;
  }

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Close( void )
{

  int rv = 0 ;

  if( fd < 0 ) return( EBADF ) ;

  /* Restore old terminal settings */
  tcsetattr( fd, TCSANOW, &old_ts ) ;

  /* Flush to prevent 'lockup' */
  tcflush( fd, TCOFLUSH ) ;

  rv = close( fd ) ;

  fd = -1 ;

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Set_Baud( struct termios *t, int baud )
{

  int rv ;
  speed_t speed ;

  switch( baud ) {
    case 50:      speed = B50 ;      break ;
    case 75:      speed = B75 ;      break ;
    case 110:     speed = B110 ;     break ;
    case 134:     speed = B134 ;     break ;
    case 150:     speed = B150 ;     break ;
    case 200:     speed = B200 ;     break ;
    case 300:     speed = B300 ;     break ;
    case 600:     speed = B600 ;     break ;
    case 1200:    speed = B1200 ;    break ;
    case 1800:    speed = B1800 ;    break ;
    case 2400:    speed = B2400 ;    break ;
    case 4800:    speed = B4800 ;    break ;
    case 9600:    speed = B9600 ;    break ;
    case 19200:   speed = B19200 ;   break ;
    case 38400:   speed = B38400 ;   break ;
    case 57600:   speed = B57600 ;   break ;
    case 115200:  speed = B115200 ;  break ;
    case 230400:  speed = B230400 ;  break ;
    case 460800:  speed = B460800 ;  break ;
    case 500000:  speed = B500000 ;  break ;
    case 576000:  speed = B576000 ;  break ;
    case 921600:  speed = B921600 ;  break ;
    case 1000000: speed = B1000000 ; break ;
    case 1152000: speed = B1152000 ; break ;
    case 1500000: speed = B1500000 ; break ;
    case 2000000: speed = B2000000 ; break ;
    case 2500000: speed = B2500000 ; break ;
    case 3000000: speed = B3000000 ; break ;
    case 3500000: speed = B3500000 ; break ;
    case 4000000: speed = B4000000 ; break ;
    default:      speed = B38400 ;   break ;
  }

  rv = cfsetspeed( t, speed ) ;

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Set_DBits( struct termios *t, int dbits )
{

  int rv ;

  switch( dbits ) {
    case 5: t->c_cflag &= ~ CSIZE ; t->c_cflag |= CS5 ; break ;
    case 6: t->c_cflag &= ~ CSIZE ; t->c_cflag |= CS6 ; break ;
    case 7: t->c_cflag &= ~ CSIZE ; t->c_cflag |= CS7 ; break ;
    default:
    case 8: t->c_cflag &= ~ CSIZE ; t->c_cflag |= CS8 ; break ;
  }

  rv = tcsetattr( fd, TCSANOW, t ) ;

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Set_Parity( struct termios *t, char parity )
{

  int rv ;

  switch( parity ) {
    default:
    case 'N': t->c_cflag &= ~ PARENB ; break ;
    case 'O': t->c_cflag |= PARENB ; t->c_cflag |= PARODD ; break ;
    case 'E': t->c_cflag |= PARENB ; t->c_cflag &= ~ PARODD ; break ;
    case 'M': t->c_cflag |= PARENB | CMSPAR | PARODD ; break ;
    case 'S': t->c_cflag |= PARENB | CMSPAR ; t->c_cflag &= ~ PARODD ; break ;
  }

  rv = tcsetattr( fd, TCSANOW, t ) ;

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Set_SBits( struct termios *t, int sbits )
{

  int rv ;

  if( sbits == 2 ) {
    t->c_cflag |= CSTOPB ;
  } else {
    t->c_cflag &= ~ CSTOPB ;
  }

  rv = tcsetattr( fd, TCSANOW, t ) ;

  return( rv ) ;

}

/*
 *
 */
int SerialPort::Peek( long int timeout, char *buf, int *len )
{

  unsigned int index ;
  int used ;

  if( fd < 0 ) return( -1 ) ;

  Ring( timeout ) ;

  if( 0 == *len ) return( 0 ) ;
  if( *len >= RING_BUF ) return( -1 ) ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used < 0 ) used += RING_BUF ;
  if( *len > used ) return( 0 ) ;

  index = ring.tail ;

  for( int i = 0 ; i < *len ; ++i ) {
    buf[i] = ring.buf[index] ;
    index++ ;
    index &= RING_MSK ;
  }

  return( *len ) ;

}

/*
 *
 */
int SerialPort::Read( long int timeout, char *buf, int *len )
{

  int i, used ;

  if( fd < 0 ) return( -1 ) ;

  Ring( timeout ) ;

  if( 0 == *len ) return( 0 ) ;
  if( *len >= RING_BUF ) return( -1 ) ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used < 0 ) used += RING_BUF ;
  if( *len > used ) return( 0 ) ;

  for( i = 0 ; i < *len ; ++i ) {
    buf[i] = ring.buf[ring.tail] ;
    ring.tail++ ;
    ring.tail &= RING_MSK ;
  }

  return( *len ) ;

}

/*
 *
 */
int SerialPort::Write( long int rate, char *buf, int *len )
{

  int rv = 0 ;

  if( fd < 0 ) return( 0 ) ;

  if( rate ) {
    for( int i = 0 ; i < *len ; i++ ) {
      rv += write( fd, &buf[i], 1 ) ;
      usleep( rate ) ;
    }
  } else {
    rv += write( fd, buf, *len ) ;
  }

  return( rv ) ;

}

/*
 *
 */
void SerialPort::Ring( long int timeout )
{

  int rv, i, actual ;
  fd_set rfds ;
  struct timeval tv ;
  unsigned int used ;

  if( fd < 0 ) return ;

  /* Get bytes used in circular buffer */
  used = ring.head - ring.tail ;
  if( used > RING_BUF ) used += RING_BUF ;

  FD_ZERO( &rfds ) ;
  FD_SET( fd, &rfds ) ;

  tv.tv_sec = 0 ;
  tv.tv_usec = timeout ;
  rv = select( fd + 1, &rfds, NULL, NULL, &tv ) ;

  if( 0 == rv ) return ;

  if( 0 == FD_ISSET( fd, &rfds ) ) return ;

  actual = read( fd, buf, RING_BUF ) ;
  if( actual > -1 ) {
    for( i = 0 ; i < actual ; ++i ) {
      ring.buf[ring.head] = buf[i] ;
      ring.head++ ;
      ring.head &= RING_MSK ;
    }
  }

  return ;

}

/*
 *
 */
void SerialPort::Buf_Asc( char *buf, int *len )
{

  unsigned char c ;
  int i ;

  fprintf( stderr, "len [%d] buf ", *len ) ;
  for( i = 0 ; i < *len ; ++i ) {
    c = 0xff & buf[i] ;
    if( c < 0x20 ) {
      fprintf( stderr, "{%02x}", c ) ;
    } else {
      fprintf( stderr, "%c", c ) ;
    }
  }
  fprintf( stderr, "\n" ) ;

  return ;

}

/*
 *
 */
void SerialPort::Buf_Hex( char *buf, int *len )
{

  int i ;

  fprintf( stderr, "len [%d] buf", *len ) ;
  for( i = 0 ; i < *len ; ++i ) {
    fprintf( stderr, " 0x%02x", 0xff & buf[i] ) ;
  }
  fprintf( stderr, "\n" ) ;

  return ;

}

/*
 *
 */
void SerialPort::Rng_Asc( void )
{

  unsigned char c ;
  unsigned int i ;

  fprintf( stderr, "head [%d] tail [%d] ", ring.head, ring.tail ) ;
  for( i = ring.tail ; i != ring.head ; ++i ) {
    i &= RING_MSK ;
    c = 0xff & ring.buf[i] ;
    if( c < 0x20 ) {
      fprintf( stderr, "{%02x}", c ) ;
    } else {
      fprintf( stderr, "%c", c ) ;
    }
  }
  fprintf( stderr, "\n" ) ;

  return ;

}

/*
 *
 */
void SerialPort::Rng_Hex( void )
{

  unsigned int i ;

  fprintf( stderr, "head [%d] tail [%d]", ring.head, ring.tail ) ;
  for( i = ring.tail ; i != ring.head ; ++i ) {
    i &= RING_MSK ;
    fprintf( stderr, " 0x%02x", 0xff & ring.buf[i] ) ;
  }
  fprintf( stderr, "\n" ) ;

  return ;

}

