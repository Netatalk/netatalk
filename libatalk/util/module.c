#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <atalk/util.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>

#ifdef DLSYM_PREPEND_UNDERSCORE
void *mod_symbol(void *module, const char *name)
{
   void *symbol;
   char *underscore;

   if (!module)
     return NULL;

   if ((underscore = (char *) malloc(strlen(name) + 2)) == NULL)
     return NULL;

   strcpy(underscore, "_");
   strcat(underscore, name);
   symbol = dlsym(module, underscore);
   free(underscore);

   return symbol;
}
#endif /* DLSYM_PREPEND_UNDERSCORE */
#endif /* HAVE_DLFCN_H */
