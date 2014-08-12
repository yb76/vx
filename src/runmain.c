
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <svc.h>
#include <svc_sec.h>

const char *RIS_DEFAULT_RUNFILE= "I:AURIS.OUT";
const char *RIS_DEFAULT_RUNFILE_SIG= "I:AURIS.OUT.P7S";


void DebugDisp (const char *template, ...)
{
    char stmp[128];
    char key;
    va_list ap;
	static int conHandle = -1;
     
    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);
    va_end (ap);

    //DispClearScreen();
	if (conHandle == -1)
		conHandle = open(DEV_CONSOLE, 0);
	clrscr();

	setfont("");
    write_at(stmp, strlen(stmp), 1, 0);
    while(read(STDIN, &key, 1) != 1);
}

int rename_file(char *oldname,char *newname,int flashflag)
{
	if( !flashflag ) {
		// RAM file	
		int iret = 0;
		_remove( newname );
		iret = _rename( oldname, newname );
		return(iret);
	} else {
		// FLASH file
		int rhandle = -1;

		// Get the file size
		rhandle = open(oldname, O_RDONLY );

		if( rhandle != -1 ) {
			int len = dir_get_file_sz(oldname);
			int whandle = -1;
			char *data = NULL;

			data = malloc(len);
			read(rhandle, (char *) data, len);
			close(rhandle);
			_remove( oldname );

			//dir_flash_coalesce();

			whandle = open(newname, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND | O_CODEFILE);
			if( whandle != -1 ) {
				int iret = write(whandle, (char *) data, len);
				close(whandle);
			} else {
			}

			free(data);
		}
	}

	return(0);

}


main(int argc, char * argv[])
{
	char sRunfile[128];
	char sRunfile_sig[128];
	char sUpdate[128];
	int envret = 0;
		
	envret = get_env("RIS_RUNFILE",sRunfile,128);
	if(envret<=0) strcpy( sRunfile,RIS_DEFAULT_RUNFILE);
	else sRunfile[envret] = '\0';

	envret = get_env("RIS_RUNFILE_SIG",sRunfile_sig,128);
	if(envret<=0) strcpy( sRunfile_sig,RIS_DEFAULT_RUNFILE_SIG);
	else sRunfile_sig[envret] = '\0';

	envret = get_env("RIS_RUNFILE_UPDATE",sUpdate,128);
	if(envret>0) {
		sUpdate[envret] = '\0';
		if(strcmp(sUpdate,"ON") == 0) {
			char newname[256];
			char newname_sig[256];
			char oldname[256];
			char oldname_sig[256];
			sprintf(oldname,"%s.OLD", sRunfile);
			sprintf(oldname_sig,"%s.OLD", sRunfile_sig);
			sprintf(newname,"%s.NEW", sRunfile);
			sprintf(newname_sig,"%s.NEW", sRunfile_sig);
			if(dir_get_file_sz( newname)>0 && dir_get_file_sz(newname_sig)>0) {
				long space;
				int iret = 0;
				if(iret==0) iret = rename_file(sRunfile,oldname,0);
				if(iret==0) iret = rename_file(sRunfile_sig,oldname_sig,0);
				if(iret==0) iret = rename_file(newname,sRunfile,0);
				if(iret==0) iret = rename_file(newname_sig,sRunfile_sig,0);
				strcpy(sRunfile,newname);
				put_env("RIS_RUNFILE_UPDATE","OFF",3);
				SVC_WAIT(500);
				SVC_RESTART("");
				//if (dir_flash_coalesce_size(&space) == 0 && space > 0) { dir_flash_coalesce(); }
			}
		}
	}

	run(sRunfile,"");
}

