#include <sys/types.h>
#include <string.h>

#include <iostream>
using namespace  std;

#define DLOG(x)  cout << x << endl;


int main(int argc, char** argv)
{
    const char*  extns = argc == 1 ? ".jpg;.jpeg;.nef;.tiff;.tif;.png;.gif" : argv[1];

    char**  extn = NULL;

    char*  e = (char*)extns;
    uint_t  n = 0;

    while (*e) {
	if (*e++ == '.') {
	    ++n;
	}
    }

    if (n)
    {
	DLOG(extns << "  n=" << n << " tokens");

	e = strcpy(new char[strlen(extns)+1], extns);

	extn = new char*[n+1];
	memset(extn, 0, n+1);
	char**  eptr = extn;

	char*  pc = NULL;
	char*  tok = NULL;
	while ( (tok = strtok_r(pc == NULL ? e : NULL, ".;", &pc)) )
	{
	    DLOG(tok);
	    const uint_t  n = strlen(tok);
	    *eptr++ = strcpy(new char[n+1], tok);
	}
	delete [] e;
    }

    char**  p = extn;
    while (*p) {
	DLOG("  '" << *p++ << "'");
    }
    p = extn;
    while (*p) {
	delete [] *p++;
    }
    delete []  extn;

    return 0;
}
