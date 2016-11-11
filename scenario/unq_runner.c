#include <unqlite.h>
#include <stdio.h>
#include <stdlib.h>

int unq_consumer( const void* out, unsigned int len, void* dmy )
{
    fwrite( out, len, 1, stdout );
}

int main( int argc, char** argv )
{
    int rc;
    unqlite* db;
    unqlite_vm* vm;
    
    if( argc < 3 )
    {
        fputs( "Usage: uql <database> <script>", stderr );
        exit(1);
    }
    
    rc = unqlite_open( &db, argv[1], UNQLITE_OPEN_CREATE );
    if( rc != UNQLITE_OK )
    {
        const char *buf;
        int len;
        unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
        fprintf( stderr, "Couldn't open database file: %s\n", buf );
        exit(1);
    }
    
    rc = unqlite_compile_file( db, argv[2], &vm );
    if( rc != UNQLITE_OK )
    {
        const char *buf;
        int len;
        unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
        fprintf( stderr, "Couldn't open compile script: %s\n", buf );
        exit(1);
    }
    
    rc = unqlite_vm_config( vm, UNQLITE_VM_CONFIG_OUTPUT,
                           unq_consumer, NULL );
    if( rc != UNQLITE_OK )
    {
        const char *buf;
        int len;
        unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
        fprintf( stderr, "Couldn't configure UnQ output: %s\n", buf );
        exit(1);
    }
    
    rc = unqlite_vm_exec( vm );
    if( rc != UNQLITE_OK )
    {
        const char *buf;
        int len;
        unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
        fprintf( stderr, "Script execution failed: %s\n", buf );
        exit(1);
    }
    
    unqlite_vm_release( vm );
    unqlite_close( db );
}

