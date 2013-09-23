#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "deviceCGI.h"

int main()
{
    char **postvars = NULL; 	// POST request data repository
    char **getvars = NULL; 		// GET request data repository
    int form_method; 			// POST = 1, GET = 0
	
    form_method = getRequestMethod();

    if(form_method == POST) 
    {
        getvars = getGETvars();
        postvars = getPOSTvars();
    } 
    else if(form_method == GET) 
    {
        getvars = getGETvars();
    }

    htmlHeader("Demo Web Page");
    htmlBody();
		
    template_page(postvars, form_method);

    htmlFooter();
    cleanUp(form_method, getvars, postvars);

	fflush(stdout);
    exit(0);
}
