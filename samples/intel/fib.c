#include <stdio.h>

/** 
 * Shamelessly stolen:
 * 	http://en.literateprograms.org/Fibonacci_numbers_(C)
 */

unsigned int fib(unsigned int n);
unsigned int fastfib(unsigned int n);
unsigned int fastfib_v2(unsigned int n);


int main()
{
	unsigned int n = 0;
    if (n == 0)
    {
        if (n == 1) {
            printf("Hello\n!");
        }
    }
	for(n=0; n<35; ++n) 
		printf("fib(%u)=%u\n", n, fib(n));
	for(n=0; n<35; ++n) 
		printf("fastfib(%u)=%u\n", n, fastfib(n));
	for(n=0; n<35; ++n) 
		printf("fastfib_v2(%u)=%u\n", n, fastfib(n));
	return 0;
}

unsigned int fib(unsigned int n)
{
	return n < 2 ? n : fib(n-1) + fib(n-2);
}

unsigned int fastfib(unsigned int n)
{
	unsigned int a[3];
	unsigned int *p=a;
	unsigned int i;

	for(i=0; i<=n; ++i) {
		if(i<2) *p=i;
		else {
			if(p==a) *p=*(a+1)+*(a+2);
			else if(p==a+1) *p=*a+*(a+2);
			else *p=*a+*(a+1);
		}
		if(++p>a+2) p=a;
	}

	return p==a?*(p+2):*(p-1);
}

unsigned int fastfib_v2 (unsigned int n)
{
	unsigned int n0 = 0;
	unsigned int n1 = 1;
	unsigned int naux;
	unsigned int i;
	if (n == 0)
		return 0;
	for (i=0; i < n-1; i++) {
		naux = n1;
		n1 = n0 + n1;
		n0 = naux;
	}
	return n1;
}

