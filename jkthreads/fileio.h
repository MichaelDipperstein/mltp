/***************************************************************************
*                          JKThreads Reenterant FileIO
*
*   File    : fileio.h
*   Purpose : Headers for JKThreads reenterant fileio
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
*   $Id: fileio.h,v 1.2 2000/08/21 20:00:07 mdipper Exp $
****************************************************************************
*
* Original work Copyright 1996 by J.D. Koftinoff Software Ltd,
* under the KPL (Koftinoff Public License)
*
*   Jeff Koftinoff
*   J.D. Koftinoff Software, Ltd.
*   #5 - 1131 Burnaby St.
*   Vancouver, B.C.
*   Canada
*   V6E-1P3
*
*   email: jeffk@jdkoftinoff.com
*
* This Work has been released for use in and modified to support
* the Multi-Layer Thread Package for SMP Linux
*
* MLTP: Multi-Layer Thread Package for SMP Linux
* Copyright (C) 2000 by Michael Dipperstein (mdipper@cs.ucsb.edu)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
***************************************************************************/

/* avoid multiple inclusions */
#if (!defined(__FILEIO_H) && defined(__JKFILEIO))
#define __FILEIO_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
FILE *fdopen (int fd, const char *mode);
FILE *fopen (const char *filename, const char *mode);
FILE* freopen(const char* filename, const char* mode, FILE* fp);
FILE *popen(const char *command, const char *mode);

int fgetpos(FILE *fp, fpos_t *posp);
int fsetpos(FILE *fp, const fpos_t *posp);

int fgetc(FILE *fp);
char *fgets (char *buf, int n, FILE *fp);

int fputc(int c, FILE *fp);
int fputs (const char *buf, FILE *fp);

#undef getc
int getc(FILE *fp);
#undef getchar
int getchar();
int getw(FILE *fp);

#undef putc
int putc(int c, FILE *stream);
#undef putchar
int putchar(int c);
int putw(int w, FILE *fp);

long int ftell (FILE *fp);
int fseek(FILE *fp, long int offset, int whence);
void rewind(FILE* fp);

size_t fread (void *buf, size_t size, size_t count, FILE *fp);
size_t fwrite(const void *buf, size_t size, size_t count, FILE *fp);

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *fp);
ssize_t getline(char **lineptr, size_t *linelen, FILE *fp);
char *gets(char *buf);
int puts(const char *buf);

void setbuffer(FILE *fp, char *buf, size_t size);
int setvbuf (FILE *fp, char *buf, int mode, size_t size);
void setlinebuf(FILE *stream);
void setbuf(FILE *fp, char *buf);
int ungetc (int c, FILE *fp);

int vfprintf(FILE *s, const char *format, va_list ap);
int vprintf(const char *format, va_list ap);
int fprintf(FILE *s, const char *format, ...);
int printf(const char *format, ...);
void perror(const char *s);
int vfscanf(FILE *s, const char *format, va_list ap);
int vscanf(const char *format, va_list ap);
int fscanf(FILE *s, const char *format, ...);
int scanf(const char *format, ...);

int fclose(FILE *fp);
int pclose(FILE *fp);

int fflush(FILE *fp);
void setfileno(FILE* fp, int fd);
void clearerr(FILE* fp);

#ifdef __cplusplus
}
#endif

#endif  /* define FILEIO_H */