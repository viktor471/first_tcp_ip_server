#include "server.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iomanip>

sockaddr *Server::get_sockaddr( PeerName peer_name_ )
{
  if     ( peer_name_ == SERVER )
    return get_sockaddr( _server );
  else if( peer_name_ == CLIENT )
    return get_sockaddr( _client );
  else
  {
    std::cerr << "peer_name_ wasn't recognized in get_sockaddr";

    return nullptr;
  }
}


sockaddr *Server::get_sockaddr( sockaddr_in& addr )
{
  return reinterpret_cast<sockaddr*>(&addr);
}


sockaddr *Server::initilize_connection_sockaddr( const in_addr_t& s_addr_,
                                                 in_port_t sin_port_ )
{
  memset(&_server, 0, SOCKADDR_IN_LENGTH );
  memset(&_client, 0, SOCKADDR_IN_LENGTH );

  _server.sin_addr.s_addr = htonl( s_addr_   );
  _server.sin_port        = htons( sin_port_ );
  _server.sin_family      = AF_INET;

  _connection_sock = 0;
  _data_sock       = 0;
  _is_data_for_sending = false;
  memset( _server.sin_zero, 0, sizeof( _server.sin_zero )  );

  _max_fd = 0;
  _read_size    = 0;
  FD_ZERO(&_all_set);


  set_select_timeval( {0,0} );

  return get_sockaddr( SERVER );
}


Socket Server::socket()
{
  _connection_sock = ::socket( PF_INET, SOCK_STREAM, 0 );
  _result = _connection_sock;

  perror("socket()");

  rebuild_set( _connection_sock );

  return _connection_sock;
}


void Server::bind()
{
  sockaddr* addr = get_sockaddr(SERVER);
  addr->sa_family = AF_INET;

  _result = ::bind(  _connection_sock, addr, sizeof ( struct sockaddr)  );

  perror("bind()");
}


void Server::listen()
{
  _result = ::listen( _connection_sock, PENDING_CONNECTIONS);
  perror("listen()");
}


Socket Server::accept()
{
  _data_sock = ::accept( _connection_sock, get_sockaddr( CLIENT ), &_socklen );

  if( _data_sock == -1 )
    perror("accept(): ");

  _ready_connection_sockets--;
  rebuild_set( _data_sock );

  HR_addr_port addr_and_port = get_hum_read_addr_port( _data_sock );

  show_addr_port( "connect", addr_and_port );

  return _data_sock;
}


int Server::read()
{
  int read_size = recv( _data_sock, _buffer.get(), BUF_SIZE, 0 );

  if( read_size <= 0 )
    perror("recv: ");
  else
  {
    _is_data_for_sending = true;
    _read_size           = read_size;

    Hum_read_addr_port addr_and_port = get_hum_read_addr_port( _data_sock );

    show_addr_port( "read", addr_and_port );

    show_received_buf();
  }

  return read_size;
}


void Server::select( fd_set &read_set, fd_set &write_set )
{

    timeval tm = _select_timeout;

    _result = ::select( _max_fd + 1, &read_set, &write_set, NULL, &tm );

    _ready_connection_sockets = _result;

    perror("select()");
}


int Server::write()
{
  if( _is_data_for_sending )
  {
    _is_data_for_sending = false;

    int &size_for_sending = _read_size;
    uint16_t sended_size
            = send( _data_sock, _buffer.get(), size_for_sending, 0 );

    if( sended_size <= 0 )
      perror("send: ");

    size_for_sending -= sended_size;

    return sended_size;
  }
  else
    return 0;
}


void Server::rebuild_set( Socket sock_ )
{
  FD_SET( sock_, &_all_set );
  set_maxfd( sock_ );
}


void Server::show_addr_port( const std::string                  &msg,
                             const HR_addr_port &addr_and_port )
{
  std::cout << std::setw(10)
            << std::left
            << msg
            << " "
            << addr_and_port._addr
            << ":"
            << addr_and_port._port
            << std::endl;
}


void Server::get_sock_name( Socket sock_fd, sockaddr_in &addr_in )
{
  sockaddr* addr   = get_sockaddr( addr_in );
  socklen_t length = SOCKADDR_IN_LENGTH;

  _result = ::getsockname( sock_fd, addr, &length );

  perror( "getsockname()" );
}


void Server::inet_ntop( sockaddr_in &addr, char buf[ADDR_LENGTH] )
{
   if( ::inet_ntop( AF_INET, &addr.sin_addr, buf, SOCKADDR_IN_LENGTH ) == NULL )
       _result = -1;

   perror( "inet_ntop" );
}


void Server::show_received_buf()
{
    std::string received_string;

    received_string.append( _buffer.get(), _read_size );

    std::cout << "received_string: "
              <<  received_string
              << std::endl;

    for( int i = 1; i < _read_size; i++ )
    {
      std::cout << " ";

      std::cout << std::hex << std::setw(2)
                << (int)_buffer.get()[i];
    }

    std::cout << std::endl;

}

void Server::close_connection()
{
    shutdown();
    close();
}

void Server::shutdown()
{
    _result = ::shutdown( _data_sock, SHUT_RDWR );

    perror( "shutdown" );
}

void Server::close()
{
   _result = ::close( _data_sock );

   perror( "close" );
}


void Server::run()
{
  std::cout << "run" << std::endl;

//  _timer.set_period( 100'000'000 );
//  _timer.start();

//  bool first_shutdown = true;

  while( FOREVER )
  {
//    if( first_shutdown )
//        if( _timer.is_elapsed() )
//        {
//          first_shutdown = false;
          //close_connection();
//        }

    fd_set read_set  = _all_set,
           write_set = _all_set;

    select( read_set, write_set );

    if( _ready_connection_sockets > 0 )
    {
      if( FD_ISSET( _connection_sock, &read_set  ) )
        accept();
      if( FD_ISSET( _data_sock,       &read_set  ) )
        read();
      if( FD_ISSET( _data_sock,       &write_set ) )
        write();
    }

  }

  std::cout << "end" << std::endl;

}


void Server::configuring()
{
  socket();
  set_reuse_addr();
  bind();
  listen();
  rebuild_set( _connection_sock );
}



Server::Server( const in_addr_t& s_addr, in_port_t sin_port )
{
  initilize_connection_sockaddr( s_addr, sin_port );
  configuring();
}


Server::Server( std::string&& s_addr, in_port_t sin_port ) :
  _timer(0),
  _server(),
  _client(),
  _data_sock(0),
  _connection_sock(0),
  _max_fd(0),
  _ready_connection_sockets(0),
  _select_timeout(),
  _socklen(0),
  _result(0),
  _is_data_for_sending(false),
  _read_size(0)
{
  FD_ZERO(&_all_set);
  inet_net_pton(  std::move( s_addr )  );

  _server.sin_port    = htons( sin_port );

  _server.sin_family  = AF_INET;

  memset( _server.sin_zero, 0, sizeof( _server.sin_zero )  );

  configuring();

}

