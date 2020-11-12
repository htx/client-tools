// misc.cpp - written and placed in the public domain by Wei Dai

#include "FirstCrypto.h"
#include "misc.h"
#include "words.h"

NAMESPACE_BEGIN(CryptoPP)

byte OAEP_P_DEFAULT[1];

void xorbuf(byte *buf, const byte *mask, size_t count)
{
	size_t i=0;
	if (IsAligned<word32>(buf) && IsAligned<word32>(mask))
	{
		if (IsAligned<word64>(buf) && IsAligned<word64>(mask))
		{
			for (i=0; i<count/8; i++)
				((word64*)(void*)buf)[i] ^= ((word64*)(void*)mask)[i];
			count -= 8*i;
			if (!count)
				return;
			buf += 8*i;
			mask += 8*i;
		}

		for (i=0; i<count/4; i++)
			((word32*)(void*)buf)[i] ^= ((word32*)(void*)mask)[i];
		
		count -= 4*i;
		if (!count)
			return;
		buf += 4*i;
		mask += 4*i;
	}
	
	for (i=0; i<count; i++)
		buf[i] ^= mask[i];
}

void xorbuf(byte *output, const byte *input, const byte *mask, size_t count)
{
	size_t i=0;
	
	if (IsAligned<word32>(output) && IsAligned<word32>(input) && IsAligned<word32>(mask))
	{
		if (IsAligned<word64>(output) && IsAligned<word64>(input) && IsAligned<word64>(mask))
		{
			for (i=0; i<count/8; i++)
				((word64*)(void*)output)[i] = ((word64*)(void*)input)[i] ^ ((word64*)(void*)mask)[i];
			count -= 8*i;
			if (!count)
				return;
			output += 8*i;
			input += 8*i;
			mask += 8*i;
		}

		for (i=0; i<count/4; i++)
			((word32*)(void*)output)[i] = ((word32*)(void*)input)[i] ^ ((word32*)(void*)mask)[i];
		count -= 4*i;
		if (!count)
			return;
		output += 4*i;
		input += 4*i;
		mask += 4*i;
	}

	for (i=0; i<count; i++)
		output[i] = input[i] ^ mask[i];
}

unsigned int Parity(unsigned long value)
{
	for (unsigned int i=8*sizeof(value)/2; i>0; i/=2)
		value ^= value >> i;
	return (unsigned int)value&1;
}

unsigned int BytePrecision(unsigned long value)
{
	unsigned int i;
	for (i=sizeof(value); i; --i)
		if (value >> (i-1)*8)
			break;

	return i;
}

unsigned int BitPrecision(unsigned long value)
{
	if (!value)
		return 0;

	unsigned int l=0, h=8*sizeof(value);

	while (h-l > 1)
	{
		unsigned int t = (l+h)/2;
		if (value >> t)
			l = t;
		else
			h = t;
	}

	return h;
}

unsigned long Crop(unsigned long value, unsigned int size)
{
	if (size < 8*sizeof(value))
    	return (value & ((1L << size) - 1));
	else
		return value;
}

NAMESPACE_END
