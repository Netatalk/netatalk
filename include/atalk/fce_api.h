/* 
 * File:   fce_api.h
 * Author: mw
 *
 * Created on 1. Oktober 2010, 21:35
 *
 * API calls for file change event api
 */

#ifndef _FCE_API_H
#define	_FCE_API_H

struct path;
struct ofork;
int fce_register_delete_file( struct path *path );
int fce_register_delete_dir( char *name );
int fce_register_new_dir( struct path *path );
int fce_register_new_file( struct path *path );
int fce_register_file_modification( struct ofork *ofork );

int fce_add_udp_socket(const char *target );  // IP or IP:Port
int fce_set_coalesce( char *coalesce_opt ); // all|delete|create

#define FCE_DEFAULT_PORT 12250
#define FCE_DEFAULT_PORT_STRING "12250"

#endif	/* _FCE_API_H */

