#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <iomanip>
#include <string>
#include <chrono>
#include <thread>

const int    ON       = 1;
const int    FOREVER  = 1;
const int    BUF_SIZE = 1024;
const size_t MSG_SIZE = 10;

const int    PENDING_CONNECTIONS = 4;
const int    ADDR_LENGTH         = strlen( "000.000.000.000");

const socklen_t  SOCKADDR_IN_LENGTH = sizeof( sockaddr_in );


typedef struct in_addr in_addr;
typedef int Socket;

  class Server
{

  typedef std::unique_ptr< char >   char_ptr ;

public:
  typedef struct Hum_read_addr_port
  {

    std::string  _addr;
    uint16_t     _port;

    Hum_read_addr_port( const std::string& addr_ = "", uint16_t port_ = 0 ) :
      _addr(addr_) , _port(port_)  {}

  } HR_addr_port;


private:

  class Timer
  {
      using hclock       = std::chrono::high_resolution_clock;
      using time_point   = hclock::time_point;
      time_point _start;
      time_point _end;
      long       _period;

  public:

      Timer( long period = 0 ) : _period( period ) {}

      void set_period( long period )
      {
          _period = period;
      }

      void start()
      {
          _start = hclock::now();
      }

      void end()
      {
          _end = hclock::now();
      }

      auto elapsed()
      {
          using namespace std::chrono;

          return duration_cast< microseconds >( _end - _start ).count();
      }

      auto elapsed_from_start()
      {
          using namespace std::chrono;
          end();

          return duration_cast< microseconds >( _end - _start ).count();
      }

      bool is_elapsed()
      {
          return elapsed_from_start() > _period;
      }

  };


  Timer       _timer;
  sockaddr_in _server;
  sockaddr_in _client;
  char_ptr    _buffer{ new char[BUF_SIZE] };

  Socket      _data_sock;
  Socket      _connection_sock;

  fd_set      _all_set;

  short       _max_fd;
  short       _ready_connection_sockets;
  timeval     _select_timeout;

  socklen_t   _socklen;
  int         _result;

  bool        _is_data_for_sending;
  int         _read_size;

  void configuring();
  void set_reuse_addr();

  void perror       ( const std::string &msg   ) const;
  void inet_net_pton(       std::string &&addr );
  void set_maxfd    (       short       new_   );
  void rebuild_set  (       Socket      sock_  );

  void show_addr_port( const std::string  &msg,
                       const HR_addr_port &addr );

  void get_sock_name ( Socket sock_fd, sockaddr_in &addr );
  void inet_ntop( sockaddr_in &addr, char buf[ ADDR_LENGTH ] );

  void show_received_buf();
  void close_connection();
  void shutdown();
  void close();

public:

  enum PeerName
  {
    SERVER,
    CLIENT
  };

  Server( const in_addr_t& s_addr   = INADDR_ANY,
                in_port_t  sin_port = 28650       );

  Server( std::string  &&s_addr,
          in_port_t      sin_port = 28650 );

  sockaddr *get_sockaddr( PeerName     peer_name_ );
  sockaddr *get_sockaddr( sockaddr_in& addr       );

  sockaddr *initilize_connection_sockaddr( const in_addr_t& s_addr_,
                                                 in_port_t  sin_port_ );
  Socket    socket();
  void      bind();
  void      listen();
  Socket    accept();
  int       write();
  int       read();

  void      select( fd_set &read_set, fd_set &write_set );
  void      run();

  short     get_ready_connection_sockets() const;
  timeval   set_select_timeval( const timeval &tm = {0,0} );
  timeval   get_select_timeval() const;

  HR_addr_port get_hum_read_addr_port( Socket sock_fd );

};


#endif // SERVER_HPP
