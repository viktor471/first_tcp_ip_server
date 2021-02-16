#include "server.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

sockaddr *Server::get_sockaddr( PeerName peer_name_ ) const
{
  if     ( peer_name_ == SERVER )
    return get_sockaddr( _server );
  else if( peer_name_ == CLIENT )
    return get_sockaddr( _client );
  else
  {
    std::cerr << "peer_name_ wasn't recognized in "
                 "get_sockaddr";
    return nullptr;
  }
}

sockaddr *Server::get_sockaddr( const sockaddr_in& addr ) const
{
  return (sockaddr*)&addr;
}

sockaddr *Server::initilize_connection_sockaddr( const in_addr_t& s_addr_,
                                                 in_port_t sin_port_ )
{
  _server.sin_addr.s_addr = htonl( s_addr_   );
  _server.sin_port        = htons( sin_port_ );
  _server.sin_family      = AF_INET;

  memset( _server.sin_zero, 0, sizeof( _server.sin_zero )  );
  _max_fd = 0;
  set_select_timeval( {0,0} );

  return get_sockaddr( SERVER );
}

Socket Server::socket()
{
  _connection_sock = ::socket( PF_INET, SOCK_STREAM, 0 );
  if( _connection_sock == -1 )
    perror("socket(): ");

  rebuild_set(_connection_sock, READ);

  return _connection_sock;
}

void Server::bind()
{
  sockaddr* addr = get_sockaddr(SERVER);
  addr->sa_family = AF_INET;
  if(   ::bind(  _connection_sock,
                 addr,
                 sizeof ( struct sockaddr)  )   != 0   )
    perror("bind(): ");
}

void Server::listen()
{
  if( ::listen(_connection_sock, PENDING_CONNECTIONS) == -1 )
    perror("listen(): ");
}

Socket Server::accept()
{
  _data_sock = ::accept( _connection_sock, get_sockaddr( CLIENT ),
                         &socklen );
  if( _data_sock == -1 )
    perror("accept(): ");

  ready_connection_sockets--;
  rebuild_set(_data_sock, READ );

  HumReadable_Addr_Port addr_and_port =
    getHumanReadable_Addr_and_Port( _client );

  std::cout << "connect "
            << addr_and_port._addr.toStdString()
            << ":"
            << addr_and_port._port
            << std::endl;

  return _data_sock;
}

int Server::read()
{
  int read_size = recv( _data_sock, _buffer.get(), BUF_SIZE, 0 );

  if( read_size <= 0 )
    perror("recv: ");
  else
  {
    HumReadable_Addr_Port addr_and_port =
      getHumanReadable_Addr_and_Port( _client );
    std::cout << "receive data from "
              << addr_and_port._addr.toStdString()
              << addr_and_port._port
              << std::endl;

    std::string received_string;
    received_string.append( _buffer.get(), read_size );

    std::cout << "received_string:" << received_string << std::endl;

    QTextStream stream(stdout);

    for( int i = 0; i < read_size; i++ )
    {
      stream << Qt::hex << (int)_buffer.get()[i];
      if( !( i % 8 ) )
        stream << " ";
    }

    stream << "\n";
    stream.flush();

  }

  return read_size;
}


int Server::write()
{
  quint16 sended_size = send(_data_sock, _buffer.get(), MSG_SIZE, 0 );
  if( sended_size <= 0 )
    perror("send: ");


  return sended_size;
}

void Server::rebuild_set( Socket sock_, Server::RW rw_type_ )
{
  if( rw_type_ == READ)
    FD_SET(sock_, _read_set.get() );
  _max_fd++;
}

void Server::run()
{

  while( FOREVER )
  {
    ready_connection_sockets = ::select(  _max_fd,
                                          _read_set.get(),
                                          _write_set.get(),
                                          NULL,
                                          &select_timeout   );

    std::cerr << "ready_con: " << ready_connection_sockets << std::endl;
    if( ready_connection_sockets > 0 )
    {
      if( FD_ISSET( _connection_sock, _read_set.get()  ) )
        accept();
      if( FD_ISSET( _data_sock,       _write_set.get() ) )
        write();
      if( FD_ISSET( _data_sock,       _read_set.get()  ) )
        read();
    }

  }

}

short Server::get_ready_connection_sockets() const
{
  return ready_connection_sockets;
}

timeval Server::set_select_timeval(const timeval &tm)
{
  return select_timeout = tm;
}

timeval Server::get_select_timeval() const
{
  return select_timeout;
}

Server::HumReadable_Addr_Port
Server::getHumanReadable_Addr_and_Port( const sockaddr_in& addr ) const
{
  HumReadable_Addr_Port addr_and_port;
  addr_and_port._addr = inet_ntoa(addr.sin_addr);
  addr_and_port._port = htons( addr.sin_port );
  return addr_and_port;
}

void Server::configuring()
{
  socket();
  setsockopt( _connection_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&ON, sizeof(int));
  bind();
  listen();
  rebuild_set(_connection_sock, READ );
}

Server::Server(const in_addr_t& s_addr, in_port_t sin_port) :
_read_set { new(fd_set) },
_write_set{ new(fd_set) }
{
  initilize_connection_sockaddr(s_addr, sin_port);
  configuring();
}

Server::Server( const std::string &s_addr, in_port_t sin_port) :
_read_set ( new fd_set ),
_write_set( new fd_set )
{
  std::string s_addr_copy = s_addr;
  if( s_addr == "" )
    s_addr_copy = "127.0.0.1";

  inet_net_pton(  AF_INET,
                  s_addr_copy.c_str() ,
                  (void*)&(_server.sin_addr),
                  (size_t)s_addr_copy.size()    );

  _server.sin_port = sin_port;

  configuring();
}

