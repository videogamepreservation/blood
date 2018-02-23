/**********************************************************************
   module: USRHOOKS.C

   author: James R. Dos‚
   phone:  (214)-271-1365 Ext #221
   date:   July 26, 1994

   This module contains cover functions for operations the library
   needs that may be restricted by the calling program.  This code
   is left public for you to modify.
**********************************************************************/
#include "debug4g.h"
#include "usrhooks.h"
#include "resource.h"

#include <memcheck.h>

/*---------------------------------------------------------------------
   Function: USRHOOKS_GetMem

   Allocates the requested amount of memory and returns a pointer to
   its location, or NULL if an error occurs.  NOTE: pointer is assumed
   to be dword aligned.
---------------------------------------------------------------------*/

int USRHOOKS_GetMem( void **ptr, unsigned long size )
{
	void *memory;

	memory = Resource::Alloc( size );
	if ( memory == NULL )
		return( USRHOOKS_Error );

	*ptr = memory;

	dprintf("USRHOOKS_GetMem allocated %d bytes at %p\n", size, memory);

	return( USRHOOKS_Ok );
}


/*---------------------------------------------------------------------
   Function: USRHOOKS_FreeMem

   Deallocates the memory associated with the specified pointer.
---------------------------------------------------------------------*/

int USRHOOKS_FreeMem( void *ptr )
{
	if ( ptr == NULL )
		return( USRHOOKS_Error );

 	dprintf("USRHOOKS_FreeMem at %p\n",ptr);

	Resource::Free( ptr );

	return( USRHOOKS_Ok );
}
