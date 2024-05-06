#include <cassert>
#include "infrastructure/utility.h"
int mylog(long long x)
{
	for(int i = 0; i < 64; ++i)
	{
		if((1LL << i) == x)
			return i;
	}
	assert(false);
}
int min(int x, int y)
{
    return x > y ? y : x;
}
int max(int x, int y)
{
	return x > y ? x : y;
}