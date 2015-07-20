#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cstring>
#include <vector>
#include <sstream>

#include <stdint.h>
/*
 * 560b / 55f1 -> PatientID
 * 560c / 55f2 -> PatientName
 * 560d / 55f3 -> PatientName
 */

enum {
  TYPE_STRING   = 0x2c00, // ASCII STRING
  TYPE_STRING2  = 0xc100, // another string ?
  TYPE_STRING3  = 0xc300, // another string ?
  TYPE_DATETIME = 0x0e00, // Date/Time stored as ASCII
  TYPE_UID      = 0xd000, // UID: pair 32bits integer
  TYPE_DBL1     = 0x2900, // double (64bits)
  TYPE_DBL2     = 0xb900, // double (64bits) or not ???
  TYPE_FL1      = 0x0800, // float 32bits
  TYPE_FL2      = 0x2800, // another float32bits
  TYPE_FL3      = 0x0600, // another float32bits
  TYPE_FL4      = 0x0500, // a807 looks like ImageMatrix
  TYPE_FL5      = 0x0100, // another float32bits
  TYPE_FL6      = 0xb800, // another float32bits
  TYPE_WST      = 0x0300, // multi-byte string ?
  TYPE_STRU1    = 0x5e00, // weird struct ?
  TYPE_STRU2    = 0x5f00, // weird struct ?
  TYPE_STRU3    = 0x2000, // weird struct ?
  TYPE_INT1     = 0x2400, // looks like an int32 (because of -1 used...)
  TYPE_BOOL     = 0x2a00, // bool stored on 32bits
  TYPE_UINT1    = 0x3200, // uint32_t
  TYPE_UINT2    = 0x0400, // another uint32_t ?
  TYPE_UINT3    = 0x3100, // uint64_t
  TYPE_UINT4    = 0x0200, // another uint32_t ?
  TYPE_UINT5    = 0x0b00, // another uint32_t ?
  TYPE_UINT6    = 0x2500, // another uint32_t ? But only O/1 another bool type ?
  TYPE_UINT7    = 0xbb00, // another uint32_t ?
  TYPE_UINT8    = 0x7000, // another uint32_t ?
  TYPE_UINT9    = 0x2100, // another uint32_t ?
  TYPE_UINT10   = 0xba00, // another uint32_t ?
  TYPE_UINT11   = 0xc200, // uint16_t
  TYPE_UINT12   = 0x2200, // another uint16_t ?
  TYPE_UINT13   = 0x7200, // another uint16_t ?
};

struct __attribute__ ((__packed__)) S
{
#if 1
  uint16_t key1; // somewhat increasing
  uint16_t key2; // [0,1,2,3]
#else
  uint32_t key;
#endif
  uint16_t type;
  uint16_t flag;
  uint16_t len;
  unsigned char separator[22];
};

static void print2( const unsigned char * buffer )
{
  // digital trash ? or actual signature ?
  static const unsigned char magic[] = {0,0,0,0,0,0,0,0,0,0,0xc,0,0,0,0,0,0,0,0,0,0,0};
  int b = memcmp(buffer, magic, sizeof(magic) );
  assert( b == 0 );
  std::cout << "<magic12>";
}

template <typename T>
static void print_type( const unsigned char * buffer, unsigned int len, bool rev = false )
{
  const T * start = (const T*)buffer;
  T val1;
  const unsigned int n = len / sizeof(val1);
  if( len % sizeof(val1) == 0 )
  {
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
  std::cout << "]" << " (" << n << ")";
  }
  else
  {
  std::cout << "[INVALID]";
  }
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
  std::cout << "]" << std::dec;
}
template <>
void print_type<int8_t>( const unsigned char * buffer, unsigned int len, bool rev )
{
  const int8_t * start = (const int8_t*)buffer;
  int8_t val1;
  const int n = len / sizeof(val1);
  std::cout << len << " [";
  for( int i = 0; i < n; ++i )
  {
    memcpy(&val1, start, sizeof(val1) );
    if( i ) std::cout << ",";
    std::cout << std::hex << (int)val1;
    start += sizeof(val1);
  }
  std::cout << "]" << std::dec;
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
void print_multistring2( const unsigned char * buffer, unsigned int len )
{
  static int i = 0;
  assert( len == 48 || len == 60 || len == 68 );
  const unsigned char sig4[] = "USAN";
  int b = memcmp( buffer, sig4, sizeof(sig4) );
  assert(b == 0 );
  const unsigned char * p = buffer + 4;
  std::ostringstream ss;
  ss << "ms2_";
  ss << i++;
  ss << ".raw";
  std::ofstream os( ss.str().c_str(), std::ios::binary );
  os.write( (char*)buffer, len );
  os.close();
  std::cout << "[USAN??]";
}

static void print( const S & s, const unsigned char buffer[] )
{
#if 1
  std::cout << "(" << std::hex << std::setw(4) << std::setfill('0') << (unsigned int)s.key1 << "," << (unsigned int)s.key2 << ") ";
#else
  std::cout << "(" << std::hex << std::setw(4) << std::setfill('0') << (unsigned int)s.key << ") ";
#endif
  assert( s.key2 == 0 || s.key2 == 1 || s.key2 == 2 || s.key2 == 3 );
  std::cout << std::dec;
  std::cout << "(" << std::hex << std::setw(4) << std::setfill('0') << s.flag << std::dec << ") ";
  switch( s.type )
  {
    case TYPE_STRING:
      assert( s.flag == 0xff00 );
      std::cout << " ST ";
      std::cout << s.len << " [" << buffer << "]";
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_STRING2:
      assert( s.flag == 11 );
      std::cout << " ST2 ";
      //std::cout << s.len << " [" << std::string((char*)buffer,s.len) << "]";
      std::cout << s.len << " [" << buffer << "]";
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_STRING3:
      assert( s.flag == 11 );
      std::cout << " ST3 ";
      //std::cout << s.len << " [" << std::string((char*)buffer,s.len) << "]";
      std::cout << s.len << " [" << buffer << "]";
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_DATETIME:
      assert( s.flag == 0x0 );
      std::cout << " DT ";
      std::cout << s.len << " [" << buffer << "]";
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UID: //(Device Serial Number / Study ID)
      assert( s.flag == 0x7 );
      std::cout << " UID ";
      assert( s.len == 8 );
      {
        uint32_t val1;
        uint32_t val2;
        memcpy(&val1, buffer, sizeof(val1) );
        memcpy(&val2, buffer + 4, sizeof(val2) );
        // TOSHIBA display UID `Device Serial Number`.`Study ID`
        std::cout << s.len << " [" << val1 << "." << val2 << "]";
        print2( s.separator );
        std::cout << std::endl;
      }
      break;
    case TYPE_DBL1:
      assert( s.flag == 0xff00 );
      std::cout << " DBL1 "; // 13ec/afc9: Imaging Frequency
      assert( s.len % 8 == 0 );
      print_type<double>( buffer, s.len, false );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_DBL2:
      assert( s.flag == 0xb );
      std::cout << " DBL2 "; // always zero always zero 
      assert( s.len == 24 );
      print_type<double>( buffer, s.len, false );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL1:
      assert( s.flag == 0xff00 );
      std::cout << " FL1 "; // 55f9 Patient Weight ?
      assert( s.len % 4 == 0 );
      print_type<float>( buffer, s.len, false );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL2:
      assert( s.flag == 0xff00 );
      std::cout << " FL2 ";
      assert( s.len % 4 == 0 );
      print_type<float>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL3:
      assert( s.flag == 0x0 );
      std::cout << " FL3 ";
      assert( s.len % 4 == 0 );
      print_type<float>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL4:
      assert( s.flag == 0x0 );
      std::cout << " FL4 ";
      assert( s.len % 4 == 0 );
      print_type<float>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL5:
      assert( s.flag == 0x0 );
      std::cout << " FL5 ";
      assert( s.len % 4 == 0 );
      print_type<float>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_FL6:
      assert( s.flag == 0xb );
      std::cout << " FL6 ";
      assert( s.len % 4 == 0 ); // 36
      print_type<float>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_INT1:
      std::cout << " INT1 ";
      assert( s.flag == 0xff00 );
      print_type<int32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT1:
      std::cout << " UINT1 ";
      assert( s.flag == 0xff00 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT2:
      std::cout << " UINT2 ";
      assert( s.flag == 0xff00 || s.flag == 0x0 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT4:
      std::cout << " UINT4 ";
      assert( s.flag == 0xfff0 || s.flag == 0x0 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT3:
      std::cout << " UINT3 ";
      assert( s.flag == 0xff00 );
      print_type<uint64_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT5:
      std::cout << " UINT5 ";
      assert( s.flag == 0x0 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT6:
      std::cout << " UINT6 ";
      assert( s.flag == 0xff00 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT7:
      std::cout << " UINT7 ";
      assert( s.flag == 0xb );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT8:
      std::cout << " UINT8 ";
      assert( s.flag == 0x17 );
      assert( s.len == 24 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT9:
      std::cout << " UINT9 ";
      assert( s.flag == 0xff00 );
      assert( s.len == 16 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT10:
      std::cout << " UINT10 ";
      assert( s.flag == 0xb );
      assert( s.len == 8 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT11:
      std::cout << " UINT11 ";
      assert( s.flag == 0xb );
      assert( s.len % 2 == 0 );
      print_type<uint16_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT12:
      std::cout << " UINT12 ";
      assert( s.flag == 0xff00 );
      assert( s.len % 2 == 0 );
      print_type<uint16_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_UINT13:
      std::cout << " UINT13 ";
      assert( s.flag == 0x17 );
      assert( s.len % 2 == 0 );
      print_type<uint16_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;
    case TYPE_BOOL:
      std::cout << " BOOL ";
      assert( s.flag == 0xff00 );
      print_type<uint32_t>( buffer, s.len );
      print2( s.separator );
      std::cout << std::endl;
      break;

    case TYPE_WST:
      std::cout << " WST ";
      assert( s.flag == 0 );
      print_multistring( buffer, s.len );
      print2( s.separator ); std::cout << std::endl;
      break;
    case TYPE_STRU1:
      std::cout << " STRU1 ";
      assert( s.flag == 27 ); // ??
      print_multistring2( buffer, s.len );
      print2( s.separator ); std::cout << std::endl;
      break;
    case TYPE_STRU2:
      std::cout << " STRU2 ";
      assert( s.flag == 27 );
      print_multistring2( buffer, s.len );
      print2( s.separator ); std::cout << std::endl;
      break;
    case TYPE_STRU3:
      std::cout << " STRU3 ";
      assert( s.flag == 65280 );
      print_multistring2( buffer, s.len );
      print2( s.separator ); std::cout << std::endl;
      break;
    default:
      std::cout << " ?? (" << std::hex << s.type << ") ";
      std::cout << "(" << std::hex << s.flag << std::dec << ") ";
      std::cout << std::dec << (int)s.len << " [" << std::string((char*)buffer,s.len) << "]";
      std::cout << ",  DL  "; print_type<double>( buffer, s.len );
      std::cout << ",  FL  "; print_type<float>( buffer, s.len );
      std::cout << ",  U64  "; print_type<uint64_t>( buffer, s.len );
      std::cout << ",  U32  "; print_type<uint32_t>( buffer, s.len );
      std::cout << ",  U16  "; print_type<uint16_t>( buffer, s.len );
      std::cout << ",  U8  "; print_type<uint8_t>( buffer, s.len );
      std::cout << ",  I64  "; print_type<int64_t>( buffer, s.len );
      std::cout << ",  I32  "; print_type<int32_t>( buffer, s.len );
      std::cout << ",  I16  "; print_type<int16_t>( buffer, s.len );
      std::cout << ",  I8  "; print_type<int8_t>( buffer, s.len );
      //print2( s.separator );
      std::cout << std::endl;
  }
}


int main(int argc, char * argv[])
{
  const char * in = argv[1];
  std::ifstream is( in, std::ios::binary );

  unsigned char buffer[5000*2];
  S s;
  assert( sizeof(s) == 32 );

  for( int r = 0; r < 6; ++r )
  {
    uint32_t n;
    is.read( (char*)&n, sizeof(n) );
    std::cout << "#Items:" << std::dec << n << std::endl;
    std::cout << std::hex << is.tellg() << std::dec << std::endl;

    if( n == 1 )
    {
      uint32_t u32;
      is.read( (char*)&u32, sizeof(u32) );
      std::cout << "debug:" << u32 << "\n";
      n = u32;
    }
    for( int i = 0; i < n + 0; ++i )
    {
      is.read( (char*)&s, sizeof(s) );
      assert( s.len < sizeof(buffer) );
      if( s.len <= 512 )
      {
        is.read( (char*)buffer, s.len );
        print( s , buffer );
      }
      else
      {
        std::streampos pos = is.tellg();
        is.read( (char*)buffer, s.len );
        std::streamsize count = is.gcount();
        assert( is.good() );
        // std::cout << "skip: " << s.len << std::endl;
        std::ofstream os( "debug.raw", std::ios::binary );
        os.write( (char*)buffer, s.len );
        std::cout << "skip:(" << std::hex << std::setw(4) << std::setfill('0') << (unsigned int)s.key1 << "," << (unsigned int)s.key2 << ") ";
        std::cout << " ?? (" << std::hex << s.type << ") ";
        assert( s.flag == 0x0 || s.flag == 0xff00 );
        std::cout << s.len << " [" << "]";
        print2( s.separator );
        //os.write( (char*)s.separator, 22 );
        os.close();
        std::cout << std::endl;
      }
    }
  }

  std::cout << std::hex << is.tellg() << std::endl;
  is.peek();
  assert( is.eof() );

  return 0;
}
