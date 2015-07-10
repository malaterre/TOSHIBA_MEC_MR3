#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cstring>

#include <stdint.h>

struct S
{
  unsigned char a;
  unsigned char b;
};

enum {
  TYPE_STRING   = 0x2c000000,
  TYPE_DATETIME = 0x0e000000,
  TYPE_UNK1     = 0xd0000000,
  TYPE_UNK2     = 0x08000000,
  TYPE_UNK3     = 0x04000000,
  TYPE_UNK4     = 0x02000000,
  TYPE_UNK5     = 0x24000000,
  TYPE_UNK6     = 0x03000000,
  TYPE_UNK7     = 0x01000000,
  TYPE_UNK8     = 0x29000000,
  TYPE_UNK9     = 0x28000000,
  TYPE_UNK10    = 0x32000000,
};

static void print( const char * buffer )
{
  std::cerr << "{";
  for( int i = 0; i < 23; ++i )
  {
    if( i ) std::cerr << ",";
    std::cerr << (int)buffer[i];
  }
  std::cerr << "}\n";
}

int main(int argc, char * argv[])
{
  const char * in = argv[1];
  std::ifstream is( in, std::ios::binary );

  char buffer[512];
  S s;
  uint32_t n;
  is.read( (char*)&n, sizeof(n) );
  std::cout << n << std::endl;
  //for( int i = 0; i < n + 850; ++i )
  for( int i = 0; i < n + 0; ++i )
  {
    uint32_t type;
    uint16_t val16;
    uint8_t len;

    is.read( (char*)&s, 2 );
    //std::cout << "key: " << (unsigned int)s.a << "," << (unsigned int)s.b << std::endl;
    is.read( (char*)&type, sizeof(type));
    //std::cout << "type: " << std::hex << val32 << std::endl;
    //assert( val == 0x2c000000 );
    is.read( (char*)&val16, sizeof(val16));
    //std::cout << "??: " << std::hex << val16 << std::endl;
    if( val16 == 0xff00 || val16 == 7 || val16 == 0 
        || val16 == 0xfff0 || val16 == 0x3000
        || val16 == 0x1000
        || val16 == 0x30
        )
    {
    }
    else
    {
      //std::cerr << std::hex << "val16: " << val16 << std::endl;
    }
    is.read( (char*)&len, sizeof(len));
    //std::cout << "len: " << std::dec << (int)val8 << std::endl;
    is.read( buffer, 23 ); // skip me ??
    char zero[23] = {};
    char case1[23] = {0,0,0,0,0,0,0,0,0,0,0,12};
    char case2[23] = {36,0,-1,8,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0};
    char case3[23] = {0,0,0,-27,-81,0,0,0,40,0,-1,4,0,0,0,0,0,0,0,0,0,0,0};
    char case4[23] = {0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    char case5[23] = {2,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0};
    char case6[23] = {18,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0};
    char case7[23] = {-112,0,-112,40,-128,0,48,0,48,0,48,0,16,0,16,41,0,0,48,0,48,0,48};
    if( memcmp(buffer, zero, 23) == 0 )
    {
      //std::cerr << "zero" << std::endl;
    }
    else if( memcmp(buffer, case1, 23) == 0
        || memcmp(buffer, case3, 23) == 0
        || memcmp(buffer, case4, 23) == 0
        || memcmp(buffer, case5, 23) == 0
        || memcmp(buffer, case6, 23) == 0
        || memcmp(buffer, case7, 23) == 0
        || memcmp(buffer, case2, 23) == 0 )
    {
      //std::cerr << "case" << std::endl;
    }
    else
    {
      print( buffer );
      //assert(0);
    }
    is.read( buffer, len );
    //std::cout << is.tellg() << "->[" << buffer << "]" << std::endl;

    std::cout << "(" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)s.a << "," << (unsigned int)s.b << ") ";
    std::cout << std::dec;
    switch( type )
    {
      case TYPE_STRING:
        std::cout << " ST ";
        std::cout << (int)len << " [" << buffer << "]" << std::endl;
        assert( val16 == 0xff00 );
        break;
      case TYPE_DATETIME:
        std::cout << " DT ";
        std::cout << (int)len << " [" << buffer << "]" << std::endl;
        assert( val16 == 0x0 );
        break;
      case TYPE_UNK1:
        std::cout << " U1 ";
        assert( len == 8 );
        {
          uint32_t val1;
          uint32_t val2;
          memcpy(&val1, buffer, sizeof(val1) );
          memcpy(&val2, buffer + 4, sizeof(val2) );
          std::cout << (int)len << " [" << val1 << "," << val2 << "]" << std::endl;
        }
        assert( val16 == 0x7 );
        break;
      case TYPE_UNK2:
        std::cout << " U2 ";
        assert( len == 4 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 );
        break;
      case TYPE_UNK3:
        std::cout << " U3 ";
        assert( len == 4 || len == 16 || len == 8 || len == 32 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 || val16 == 0x0 );
        break;
      case TYPE_UNK4:
        std::cout << " U4 ";
        assert( len == 36 || len == 40 || len == 4 || len == 16 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0x0 || val16 == 0xff00 || val16 == 0xfff0 );
        break;
       case TYPE_UNK5:
        std::cout << " U5 ";
        assert( len == 4 || len == 40 || len == 32 || len == 12 || len == 24 || len == 16 || len == 8 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 );
        break;
       case TYPE_UNK6:
        std::cout << " U6 ";
        assert( len == 25 || len == 23 || len == 36 || len == 19 || len == 26 || len == 34 );
        {
          std::cout << (int)len << " [ ?? ]" << std::endl;
        }
        assert( val16 == 0x0 );
        break;
       case TYPE_UNK7:
        std::cout << " U7 ";
        assert( len == 0 || len == 4 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0x0 );
        break;
       case TYPE_UNK8:
        std::cout << " U8 ";
        assert( len == 8 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 );
        break;
        case TYPE_UNK9:
        std::cout << " U9 ";
        assert( len == 4 || len == 32 || len == 12 || len == 0 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 );
        break;
         case TYPE_UNK10:
        std::cout << " U10 ";
        assert( len == 12 || len == 4 || len == 8 );
        {
          uint32_t val1;
          memcpy(&val1, buffer, sizeof(val1) );
          std::cout << (int)len << " [" << val1 << "]" << std::endl;
        }
        assert( val16 == 0xff00 );
        break;
       default:
        std::cout << " ?? " << std::hex << type << " ";
        std::cout << std::dec << (int)len << " [" << buffer << "]" << std::endl;
    }
  }
  std::cout << std::hex << is.tellg() << std::endl;

  return 0;
}
