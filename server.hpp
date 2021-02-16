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
#include <QString>
#include <QTextStream>
#include <iomanip>
#include <QDebug>
#include <string>


const int    ON = 1;
const int    PENDING_CONNECTIONS = 4;
const int    FOREVER = 1;
const int    BUF_SIZE = 1024;
const size_t MSG_SIZE = 10;

typedef struct in_addr In_addr;
typedef int Socket;


class Server
{

  typedef std::unique_ptr< fd_set > fd_set_ptr ;
  typedef std::unique_ptr< char >   char_ptr ;
private:

  sockaddr_in _server;
  sockaddr_in _client;
  char_ptr    _buffer{ new char[3] };

  Socket      _data_sock;
  Socket      _connection_sock;

  fd_set_ptr _read_set;
  fd_set_ptr _write_set;

  short       _max_fd;
  short       ready_connection_sockets;
  timeval     select_timeout;

  socklen_t   socklen;

  void configuring();

public:

  enum PeerName
  {
    SERVER,
    CLIENT
  };

  enum RW
  {
    READ,
    WRITE
  };

  struct HumReadable_Addr_Port
  {

    QString _addr;
    quint32 _port;

    HumReadable_Addr_Port( const QString& addr_ = 0, quint32 port_ = 0 ) :
      _addr(addr_) , _port(port_)  {}

  };

  Server( const in_addr_t& s_addr   = INADDR_ANY,
                in_port_t  sin_port = 30000       );

  Server( const std::string&  s_addr,
          in_port_t sin_port = 30000       );

  sockaddr *get_sockaddr(       PeerName     peer_name_ ) const;
  sockaddr *get_sockaddr( const sockaddr_in& addr       ) const;

  sockaddr *initilize_connection_sockaddr( const in_addr_t& s_addr_,
                                                 in_port_t  sin_port_ );
  Socket    socket();
  void      bind();
  void      listen();
  Socket    accept();
  int       write();
  int       read();

  void      rebuild_set( Socket sock_, RW rw_type_ );
  void      run();

  short     get_ready_connection_sockets() const;
  timeval   set_select_timeval( const timeval &tm = {0,0} );
  timeval   get_select_timeval() const;

  HumReadable_Addr_Port
  getHumanReadable_Addr_and_Port( const sockaddr_in& addr ) const;

  HumReadable_Addr_Port
  getHumanReadable_Addr_and_Port( const sockaddr&    addr ) const;

};

#endif // SERVER_HPP
