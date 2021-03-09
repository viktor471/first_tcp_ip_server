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
#include "end_point.hpp"

const int    PENDING_CONNECTIONS = 4;

typedef struct in_addr in_addr;

class Server : public End_point
{

private:

  sockaddr_in _client;

  Socket      _connection_sock;

  bool        _is_data_for_sending;
  int         _read_size;

  void configuring();

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
