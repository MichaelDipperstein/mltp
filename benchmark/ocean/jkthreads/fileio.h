/***************************************************************************
*                          JKThreads Reenterant FileIO
*
*   File    : fileio.h
*   Purpose : Headers for JKThreads reenterant fileio
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
***************************************************************************/

/*
 * $Id: fileio.h,v 1.2 2000/05/17 01:10:02 mdipper Exp mdipper $
 *
 * $Log: fileio.h,v $
 * Revision 1.2  2000/05/17 01:10:02  mdipper
 * Clean Up.
 *
 */

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