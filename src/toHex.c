/*
 * toHex.c
 *
 * Created: 6.10.2023 14.57.10
 * Author : Jessé Forest
 */
#include "toHex.h"
char* itohexa (char dest[5], uint16_t x)
{
  uint8_t remove_zeroes = 1;
  uint8_t f =0;
  char* ptr = dest;

  for(f=0; f<4; f++)
  {
    // mask out the nibble by shifting 4 times byte number:
    uint8_t nibble = (x >> (3-f)*4) & 0xF;

    // binary to ASCII hex conversion:
    char hex_digit = "0123456789ABCDEF" [nibble];

    if(!ZERO_PAD && remove_zeroes && hex_digit == '0')
    {
      ; // do nothing
    }
    else
    {
      remove_zeroes = 0;
      *ptr = hex_digit;
      ptr++;
    }
  }

  if(remove_zeroes) // was it all zeroes?
  {
    *ptr = '0';
    ptr++;
  }
  *ptr = '\0';

  return dest;
}
