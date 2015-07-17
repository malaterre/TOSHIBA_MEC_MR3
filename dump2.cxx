#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cstring>

#include <stdint.h>

enum {
  TYPE_STRING   = 0x2c00,
  TYPE_DATETIME = 0x0e00,
  TYPE_UNK1     = 0xd000,
  TYPE_UNK2     = 0x0800,
  TYPE_UNK3     = 0x0400,
  TYPE_UNK4     = 0x0200,
  TYPE_UNK5     = 0x2400,
  TYPE_UNK6     = 0x0300, // multi-byte string ?
  TYPE_UNK7     = 0x0100,
  TYPE_UNK8     = 0x2900,
  TYPE_UNK9     = 0x2800,
  TYPE_UNK10    = 0x3200,
  TYPE_UNK11    = 0x2300,
  TYPE_UNK12    = 0x2900,
  TYPE_UNK13    = 0x2200,
  TYPE_UNK14    = 0x3100,
  TYPE_UNK15    = 0x2a00,
  TYPE_UNK16    = 0x2500,
  TYPE_UNK17    = 0x0600,
};

struct S
{
  uint16_t key;
  uint16_t key2; // zero ?
  uint16_t type;
  uint16_t val16;
  unsigned char len;
  unsigned char dummy2[11];
  unsigned char twelve;
  unsigned char dummy3[11];
};

static void print2( const unsigned char * buffer )
{
  std::cout << std::dec;
  std::cout << " {";
  for( int i = 0; i < 23; ++i )
  {
    if( i ) std::cout << ",";
    std::cout << (int)buffer[i];
  }
  std::cout << "}";
}

template <typename T>
static void print_type( const unsigned char * buffer, unsigned int len, bool rev = false )
{
  const T * start = (const T*)buffer;
  T val1;
  const unsigned int n = len / sizeof(val1);
  assert( len % sizeof(val1) == 0 );
  std::cout << len << " [";
  for( unsigned int i = 0; i < n; ++i )
  {
    if( rev )
    {
      union { T val; char buf[ sizeof(T) ]; } u;
      const unsigned char * tmp = (unsigned char*)start;
      for( int k = 0; k < sizeof(T); ++k )
      {
        u.buf[ sizeof(T) - k - 1 ] = tmp[k];
      }
      val1 = u.val;
    }
    else
      memcpy(&val1, start, sizeof(val1) );
    if( i ) std::cout << ",";
    std::cout << val1;
    start += 1 /*sizeof(val1)*/;
  }
  std::cout << "]";
}
template <>
void print_type<uint8_t>( const unsigned char * buffer, unsigned int len, bool rev )
{
  const uint8_t * start = (const uint8_t*)buffer;
  uint8_t val1;
  const int n = len / sizeof(val1);
  std::cout << len << " [";
  for( int i = 0; i < n; ++i )
  {
    memcpy(&val1, start, sizeof(val1) );
    if( i ) std::cout << ",";
    std::cout << std::hex << (int)val1;
    start += sizeof(val1);
  }
  std::cout << "]";
}

void print_multistring( const unsigned char * buffer, unsigned int len )
{
  std::cout << len;
  std::cout << " [";
  if( len )
  {
  assert( buffer[0] == 0xdf );
  assert( buffer[1] == 0xff );
  assert( buffer[2] == 0x79 );
  //assert( buffer[3] == 0x15 );
  assert( buffer[4] == 0x1 );
  assert( buffer[5] == 0x9 );
  assert( buffer[6] == 0x0 );
  const char * iso = (char*)buffer + 7;
  std::string s1( iso, 9 );
  assert( s1 == "ISO8859-1" );
  const unsigned char * next = buffer + 7 + 9;
  assert( next[0] == 0x2 );
  //assert( next[1] == 0x6 );
  assert( next[2] == 0x0 );
  const unsigned int diff = len - (7 + 9 + 3);
  std::string s2( (char*)next + 3, diff );
  std::cout << s1 << ":" << s2;
  }
  std::cout << "]";
}

static void print( const S & s, const unsigned char buffer[] )
{
  static int didx = 0;
  if( s.twelve != 12 )
  {
    std::cout << "ERROR:";
  }
  std::cout << didx++ << ":(" << std::hex << std::setw(4) << std::setfill('0') << (unsigned int)s.key << "," << (unsigned int)s.key2 << ") ";
  std::cout << std::dec;
  switch( s.type )
  {
    case TYPE_STRING:
      //assert( s.key2 == 0 );
      assert( s.val16 == 0xff00 );
      assert( s.dummy2[0] == 0 );
      std::cout << " ST ";
      std::cout << (int)s.len << " [" << buffer << "]";
      print2( s.dummy2 );
      std::cout << std::endl;
      break;
    case TYPE_DATETIME:
      //assert( s.key2 == 0 );
      //assert( s.val16 == 0x0 );
      std::cout << " DT ";
      std::cout << (int)s.len << " [" << buffer << "]";
      print2( s.dummy2 );
      std::cout << std::endl;
      break;
    case TYPE_UNK1: //(Device Serial Number / Study ID)
      assert( s.key2 == 0 );
      //assert( s.val16 == 0xd );
      std::cout << " UID ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 8 );
      {
        uint32_t val1;
        uint32_t val2;
        memcpy(&val1, buffer, sizeof(val1) );
        memcpy(&val2, buffer + 4, sizeof(val2) );
        // TOSHIBA display UID `Device Serial Number`.`Study ID`
        std::cout << (int)s.len << " [" << val1 << "." << val2 << "]";
        print2( s.dummy2 ); std::cout << std::endl;
      }
      break;
    case TYPE_UNK2:
      std::cout << " FL "; // Patient Weight ?
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 4 || s.len == 32 || s.len == 0 );
      print_type<float>( buffer, s.len, false );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK3:
      std::cout << " U3 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 4 || s.len == 16 || s.len == 8 || s.len == 32 || s.len == 0 || s.len == 24 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK4:
      std::cout << " U4 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      //assert( s.len == 36 || s.len == 40 || s.len == 4 || s.len == 16 || s.len == 24 || s.len == 8 || s.len == 92 );
      print_type<uint32_t>( buffer, s.len, false );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK5:
      std::cout << " U5 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 4 || s.len == 40 || s.len == 32 || s.len == 12 || s.len == 24 || s.len == 16 || s.len == 8 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK6:
      std::cout << " WST ";
      //assert( s.val16 == 0 || s.val16 == 256 || s.val16 ==  );
      //std::cout << (int)s.len << " [ ?? ]" << std::endl;
      print_multistring( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK7:
      std::cout << " U7 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 0 || s.len == 4 );
      print_type<int32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
   case TYPE_UNK9:
      std::cout << " FL ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 4 || s.len == 32 || s.len == 12 || s.len == 0 || s.len == 48 || s.len == 8 );
      print_type<float>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
    case TYPE_UNK10:
      std::cout << " U8 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 12 || s.len == 4 || s.len == 8 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
     case TYPE_UNK11:
      std::cout << " U11 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      //assert( s.len == 16 || s.len == 132 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
      case TYPE_UNK12:
      std::cout << " U12 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      //assert( s.len == 8 );
      print_type<int16_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
       case TYPE_UNK13:
      std::cout << " U13 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 2 || s.len == 0 );
      print_type<uint16_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
        case TYPE_UNK14:
      std::cout << " U14 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 8 || s.len == 16 || s.len == 0 );
      print_type<uint16_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
         case TYPE_UNK15:
      std::cout << " U15 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      //assert( s.len == 4 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
         case TYPE_UNK16:
      std::cout << " U16 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      assert( s.len == 4 || s.len == 0 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
          case TYPE_UNK17:
      std::cout << " U17 ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      print_type<uint32_t>( buffer, s.len );
      print2( s.dummy2 ); std::cout << std::endl;
      break;
 
    default:
      std::cout << " ?? (" << std::hex << s.type << ") ";
      std::cout << "(" << std::hex << s.val16 << std::dec << ") ";
      std::cout << std::dec << (int)s.len << " [" << std::string((char*)buffer,s.len) << "]";
      print2( s.dummy2 );
      std::cout << std::endl;
  }

}


int main(int argc, char * argv[])
{
  const char * in = argv[1];
  std::ifstream is( in, std::ios::binary );

  unsigned char buffer[512];
  S s;
  //std::cout << sizeof(s) << std::endl;
  assert( sizeof(s) == 32 );
  uint32_t n;
  is.read( (char*)&n, sizeof(n) );
  std::cout << n << std::endl;
  //for( int i = 0; i < n + 850; ++i )
  for( int i = 0; i < n + 0; ++i )
  {

    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  is.read( (char*)&n, sizeof(n) );
  std::cout << std::dec << n << std::endl;
  n = 70;
  for( int i = 0; i < n + 0; ++i )
  {

    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  //is.read( (char*)&n, sizeof(n) );
  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
    n = 57; // skip some junk ???
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    //print( s , buffer );
  }

  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
  n = 20;
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
   n= 11;
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    //print( s , buffer );
  }
  is.seekg(1, std::ios::cur );

  n = 400 - 27;
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }


  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
   n = 10;
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

  std::cout << "n=" << std::dec << n << std::endl;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
  n = 260;
 
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    //print( s , buffer );
  }

  std::cout << "back" << std::endl;
  is.seekg( -4, std::ios::cur );
  is.seekg( -4, std::ios::cur );
  is.seekg( -32, std::ios::cur );
  is.seekg( -32, std::ios::cur );
  is.seekg( -44, std::ios::cur );
  n = 1;
  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
  is.read( (char*)&n, sizeof(n) );
  std::cout << "MAGIC: " << std::dec << n << std::endl;
  
  n = 9;
  for( int i = 0; i < n + 0; ++i )
  {
    is.read( (char*)&s, sizeof(s) );
    is.read( (char*)buffer, s.len );
    print( s , buffer );
  }

//  std::cout << "n=" << std::dec << n << std::endl;
//  std::cout << "tell:" << std::hex << is.tellg() << std::dec << std::endl;
//  is.read( (char*)&n, sizeof(n) );
//  std::cout << "MAGIC: " << std::dec << n << std::endl;
// 
//  for( int i = 0; i < n + 0; ++i )
//  {
//    is.read( (char*)&s, sizeof(s) );
//    is.read( (char*)buffer, s.len );
//    print( s , buffer );
//  }


 
  std::cout << std::hex << is.tellg() << std::endl;
  is.peek();
  assert( is.eof() );

  return 0;
}
