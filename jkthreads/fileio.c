/***************************************************************************
*                          JKThreads Reenterant FileIO
*
*   File    : fileio.c
*   Purpose : JKThreads reenterant fileio
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
*   $Id: fileio.c,v 1.2 2000/08/21 19:58:28 mdipper Exp $
****************************************************************************
*
*   This file replaces the standard libc file input/output functions
*   with a set of thread-safe reenterant wrappers.  At the present time
*   the functions provided use a semaphore to ensure that only one process
*   is accessing a given file at any time.  Upon creation of a file
*   descriptor, by one of the file open functions, a semaphore is
*   allocated.  When the file is closed, the semaphore is killed.
*
*   Much of the code in this module has been taken directly from
*   Linux-Threads version 0.71 and ported to run using jkthread semaphores
*   instead of pthread mutexes.
*
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

#ifdef __JKFILEIO

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include "jkcthread.h"

/***************************************************************************
*                            IMPORTED VARIABLES
***************************************************************************/
extern jkthreadinfo *_jkthread_kludge[32768];


/***************************************************************************
*                             GLOBAL VARIABLES
***************************************************************************/
jksem _jk_file_sems[JKMAX_OPEN];

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
extern int _IO_fclose(_IO_FILE*);
extern _IO_FILE *_IO_fdopen(int, const char*);
extern int _IO_fflush(_IO_FILE*);
extern int _IO_fgetpos(_IO_FILE*, _IO_fpos_t*);
extern char* _IO_fgets(char*, int, _IO_FILE*);
extern _IO_FILE *_IO_fopen(const char*, const char*);
extern int _IO_fprintf(_IO_FILE*, const char*, ...);
extern int _IO_fputs(const char*, _IO_FILE*);
extern int _IO_fsetpos(_IO_FILE*, const _IO_fpos_t *);
extern long int _IO_ftell(_IO_FILE*);
extern _IO_size_t _IO_fread(void*, _IO_size_t, _IO_size_t, _IO_FILE*);
extern _IO_size_t _IO_fwrite(const void*,
                             _IO_size_t, _IO_size_t, _IO_FILE*);
extern char* _IO_gets(char*);
extern int _IO_printf(const char*, ...);
extern int _IO_puts(const char*);
extern int _IO_scanf(const char*, ...);
extern void _IO_setbuffer(_IO_FILE *, char*, _IO_size_t);
extern int _IO_setvbuf(_IO_FILE*, char*, int, _IO_size_t);
extern int _IO_sscanf(const char*, const char*, ...);
extern int _IO_sprintf(char *, const char*, ...);
extern int _IO_ungetc(int, _IO_FILE*);
extern int _IO_vsscanf(const char *, const char *, _IO_va_list);
extern int _IO_vsprintf(char*, const char*, _IO_va_list);

extern int _IO_file_close_it(_IO_FILE*);
extern _IO_FILE* _IO_file_fopen(_IO_FILE*, const char*, const char*);
extern _IO_ssize_t __getdelim (char**, _IO_size_t*, int, _IO_FILE*);
extern int _IO_flush_all(void);

#ifndef _IO_pos_BAD
#define _IO_pos_BAD ((_IO_fpos_t)(-1))
#endif
#define _IO_clearerr(FP) ((FP)->_flags &= ~(_IO_ERR_SEEN|_IO_EOF_SEEN))
#define _IO_fseek(__fp, __offset, __whence) \
  (_IO_seekoff(__fp, __offset, __whence, _IOS_INPUT|_IOS_OUTPUT) == _IO_pos_BAD ? EOF : 0)
#define _IO_rewind(FILE) (void)_IO_seekoff(FILE, 0, 0, _IOS_INPUT|_IOS_OUTPUT)
#define _IO_vprintf(FORMAT, ARGS) _IO_vfprintf(_IO_stdout, FORMAT, ARGS)
#define _IO_freopen(FILENAME, MODE, FP) \
  (_IO_file_close_it(FP), _IO_file_fopen(FP, FILENAME, MODE))
#define _IO_fileno(FP) ((FP)->_fileno)
extern _IO_FILE* _IO_popen(const char*, const char*);
#define _IO_pclose _IO_fclose
#define _IO_setbuf(_FP, _BUF) _IO_setbuffer(_FP, _BUF, _IO_BUFSIZ)
#define _IO_setlinebuf(_FP) _IO_setvbuf(_FP, NULL, 1, 0)

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : jksem_lockfile
*   Description: This function will acquire a lock for a file stream.
*   Parameters : stream - pointer to file stream to be locked
*   Effects    : Recursive lock associated with a file is acquired
*   Returned   : None
****************************************************************************/
void jksem_lockfile(FILE *stream)
{
    int fd = fileno(stream);

    if (fd >= 0 && fd < JKMAX_OPEN) 
    {
        /* acquire lock */
        jkrsem_get(&_jk_file_sems[fd]);
    }
}


/****************************************************************************
*   Function   : jksem_unlockfile
*   Description: This function will release a lock for a file stream.
*   Parameters : stream - pointer to file stream to be unlocked
*   Effects    : Recursive lock associated with a file is released
*   Returned   : None
****************************************************************************/
void jksem_unlockfile(FILE *stream)
{
    int fd = fileno(stream);

    if (fd >= 0 && fd < JKMAX_OPEN) 
    {
        /* release lock */
        jkrsem_release(&_jk_file_sems[fd]);
    }
}


/****************************************************************************
*   Function   : jksem_trylockfile
*   Description: This function will make a non-blocking attempt to acquire a
*                lock for a file stream.
*   Parameters : stream - pointer to file stream to be locked
*   Effects    : Recursive lock associated with a file may be acquired
*   Returned   : non-zero for success.  0 implies the lock was not acquired.
****************************************************************************/
int jksem_trylockfile(FILE *stream)
{
    int fd = fileno(stream);

    if (fd >= 0 && fd < JKMAX_OPEN) 
    {
        /* try to  acquire lock */
        return (jksem_tryget(&_jk_file_sems[fd]));
    }

    return 0;
}


/****************************************************************************
*   Function   : jksem_initfilelock
*   Description: This function initializes a file lock associated with a
*                given file stream provided the lock does not already exist.
*   Parameters : stream - pointer to file stream that lock is being created
*                         for.
*   Effects    : Lock associated with a file stream is initialized for use.
*   Returned   : none
****************************************************************************/
void jksem_initfilelock(FILE *stream)
{
    int fd = fileno(stream);

    if (fd >= 0 && fd < JKMAX_OPEN) 
    {
        if (_jk_file_sems[fd].sem_id == 0)
        {
            /* initialize lock */
            jksem_init(&_jk_file_sems[fd]);
        }
    }
}


/****************************************************************************
*   Function   : jksem_killfilelock
*   Description: This function kills a file lock associated with a given
*                file stream.
*   Parameters : stream - pointer to file stream lock being killed
*   Effects    : Lock associated with a file stream is killed.  Semaphore
*                released back into pool.
*   Returned   : none
****************************************************************************/
void jksem_killfilelock(FILE *stream)
{
    int fd = fileno(stream);

    if (fd >= 0 && fd < JKMAX_OPEN) 
    {
        if (_jk_file_sems[fd].sem_id != 0)
        {
            /* free lock */
            jksem_kill(&_jk_file_sems[fd]);
        }
    }
}


/****************************************************************************
*   Function   : fdopen
*   Description: This function replaces the libc fdopen function; it opens
*                a file stream for a file descriptor.
*   Parameters : fd - file descriptor
*                mode - mode to open file "r", "w", ...
*   Effects    : file lock is created for newly opened file stream
*   Returned   : New stream for the file descriptor fd.
****************************************************************************/
FILE *fdopen (int fd, const char *mode)
{
    FILE *fp;

    if (!jkthread_initcheck())
    {
        jkthread_init();
    }

    jksem_get(&_jk_fileio_sem);
    fp = _IO_fdopen (fd, mode);
    jksem_release(&_jk_fileio_sem);

    if (fp != NULL)
    {
        /* initialize the file lock */
        jksem_initfilelock(fp);
    }
    
    return fp;
}


/****************************************************************************
*   Function   : fopen
*   Description: This function replaces the libc fopen function; it opens a
*                file stream for the named file.
*   Parameters : filename - name of file to be opened
*                mode - mode to open file "r", "w", ...
*   Effects    : file lock is created for newly opened file stream
*   Returned   : New stream for the file filename.
****************************************************************************/
FILE *fopen (const char *filename, const char *mode)
{
    FILE *fp;

    if (!jkthread_initcheck())
    {
        jkthread_init();
    }

    jksem_get(&_jk_fileio_sem);
    fp = _IO_fopen (filename, mode);
    jksem_release(&_jk_fileio_sem);

    if (fp != NULL)
    {
        /* initialize the file lock */
        jksem_initfilelock(fp);
    }

    return fp;
}


/****************************************************************************
*   Function   : freopen
*   Description: This function replaces the libc freopen function; it closes
*                a file and reopens it.
*   Parameters : filename - name of file to be opened
*                mode - mode to open file "r", "w", ...
*                fp - stream for file being reopened
*   Effects    : filename is closed and reopened.
*   Returned   : fp, or NULL if reopen fails.
****************************************************************************/
FILE* freopen(const char* filename, const char* mode, FILE* fp)
{
    FILE *ret;

    if (fp == NULL)
    {
        return NULL;
    }

    jksem_lockfile(fp);

    if (!(fp->_flags & _IO_IS_FILEBUF))
    {
        jksem_unlockfile(fp);
        return NULL;
    }

    jksem_get(&_jk_fileio_sem);
    ret = _IO_freopen(filename, mode, fp);
    jksem_release(&_jk_fileio_sem);

    jksem_unlockfile(fp);
    return ret;
}


/****************************************************************************
*   Function   : popen
*   Description: This function replaces the libc popen function; it opens a
*                file stream for a pipe to the system command command.
*   Parameters : command - commmand to run at other end of pipe
*                mode - mode to open pipe "r", "w", ...
*   Effects    : file lock is created for newly opened pipe
*   Returned   : New stream pointer for pipe
****************************************************************************/
FILE *popen(const char *command, const char *mode)
{
    FILE *fp;

    if (!jkthread_initcheck())
    {
        jkthread_init();
    }

    jksem_get(&_jk_fileio_sem);
    fp = _IO_popen (command, mode);
    jksem_release(&_jk_fileio_sem);

    if (fp != NULL)
    {
        /* initialize the file lock */
        jksem_initfilelock(fp);
    }

    return fp;
}


/****************************************************************************
*   Function   : fgetpos
*   Description: This function replaces the libc fgetpos function; it
*                provides the file position indicator for a file stream.
*   Parameters : fp - file stream
*                posp - position indicator for fp (upon return)
*   Effects    : none
*   Returned   : 0, or errno if failure
****************************************************************************/
int fgetpos(FILE *fp, fpos_t *posp)
{
    int ret;
 
    jksem_lockfile(fp);
    ret = _IO_fgetpos(fp, posp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : fsetpos
*   Description: This function replaces the libc fsetpos function; it sets
*                the file position indicator for a file stream to the
*                specified position.
*   Parameters : fp - file stream
*                posp - position indicator for fp
*   Effects    : File position indicator is updated.
*   Returned   : 0, or errno if failure
****************************************************************************/
int fsetpos(FILE *fp, const fpos_t *posp)
{
    int ret;

    jksem_lockfile(fp);
    ret = _IO_fsetpos (fp, posp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : fgets
*   Description: This function replaces the libc fgets function; it reads a
*                terminated stream of up to n-1 characters and stores it in
*                buf.
*   Parameters : buf - buffer to store the results of the get
*                n - maximum number of characters to get
*                fp - stream to get characters from
*   Effects    : position in fp advances a full string, up to n charcaters
*   Returned   : buf, or NULL if error occurred
****************************************************************************/
char *fgets (char *buf, int n, FILE *fp)
{
    jksem_lockfile(fp);
    buf = _IO_fgets (buf, n, fp);
    jksem_unlockfile(fp);

    return buf;
}


/****************************************************************************
*   Function   : fputs
*   Description: This function replaces the libc fputs function; it writes a
*                null terminated string to a file stream.
*   Parameters : buf - string to write
*                fp - stream to put characters in
*   Effects    : position in fp advances and string is stored in stream
*   Returned   : EOF if there is an error
****************************************************************************/
int fputs (const char *buf, FILE *fp)
{
    int ret;

    jksem_lockfile(fp);
    ret = _IO_fputs (buf, fp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : fgetc
*   Description: This function replaces the libc fgetc function; it reads a
*                character from a stream.
*   Parameters : fp - stream to get character from
*   Effects    : position in fp advances one character
*   Returned   : unsigned character, or NULL if error occurred
****************************************************************************/
int fgetc(FILE *fp)
{
    int ret;

    jksem_lockfile(fp);
    ret = _IO_getc(fp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : fputc
*   Description: This function replaces the libc fputc function; it writes a
*                character to a file stream.
*   Parameters : c - character to put
*                fp - stream to put characters in
*   Effects    : position in fp advances and string is stored in stream
*   Returned   : character written, or EOF if there is an error
****************************************************************************/
int fputc(int c, FILE *fp)
{
    int ret;

    jksem_lockfile(fp);
    ret = _IO_putc(c, fp);
    jksem_unlockfile(fp);

    return ret;
}

/****************************************************************************
*   Function   : getc
*   Description: This function replaces the libc getc function; it reads a
*                character from a stream.
*   Parameters : fp - stream to get character from
*   Effects    : position in fp advances one character
*   Returned   : unsigned character, or NULL if error occurred
****************************************************************************/
#undef getc
int getc(FILE *fp)
{
    int ret;

    jksem_lockfile(fp);
    ret = _IO_getc(fp);
    jksem_unlockfile(fp);

    return ret;
}

/****************************************************************************
*   Function   : getchar
*   Description: This function replaces the libc getchar function; it reads a
*                character from stdin.
*   Parameters : none
*   Effects    : position in stdin advances one character
*   Returned   : unsigned character, or NULL if error occurred
****************************************************************************/
#undef getchar
int getchar()
{
    int ret;

    jksem_lockfile(stdin);
    ret = _IO_getc(stdin);
    jksem_unlockfile(stdin);

    return ret;
}


/****************************************************************************
*   Function   : getw
*   Description: This function replaces the libc getw function; it reads a
*                word (int) from a stream.
*   Parameters : fp - stream to get word from
*   Effects    : position in fp advances one word
*   Returned   : word read, or EOF if error occurred
****************************************************************************/
int getw(FILE *fp)
{
    int w;
    _IO_size_t bytes_read;

    jksem_lockfile(fp);
    bytes_read = _IO_sgetn (fp, (char*)&w, sizeof(w));
    jksem_unlockfile(fp);

    return sizeof(w) == bytes_read ? w : EOF;
}


/****************************************************************************
*   Function   : putc
*   Description: This function replaces the libc putc function; it writes a
*                character to a file stream.
*   Parameters : c - character to put
*                stream - stream to put characters in
*   Effects    : position in stream advances and string is stored in stream
*   Returned   : character written, or EOF if there is an error
****************************************************************************/
#undef putc
int putc(int c, FILE *stream)
{
    int ret;

    jksem_lockfile(stream);
    ret = _IO_putc(c, stream);
    jksem_unlockfile(stream);

    return ret;
}


/****************************************************************************
*   Function   : putchar
*   Description: This function replaces the libc putchar function; it writes
*                a character to a stdout.
*   Parameters : c - character to put
*   Effects    : position in stdout advances and string is stored in stdout
*   Returned   : character written, or EOF if there is an error
****************************************************************************/
#undef putchar
int putchar(int c)
{
    int ret;

    jksem_lockfile(stdout);
    ret = _IO_putc(c, stdout);
    jksem_unlockfile(stdout);

    return ret;
}


/****************************************************************************
*   Function   : putw
*   Description: This function replaces the libc putw function; it puts a
*                word (int) on a stream.
*   Parameters : w - word to write
*                fp - stream to put word to
*   Effects    : position in fp advances one word
*   Returned   : 0, or EOF if error occurred
****************************************************************************/
int putw(int w, FILE *fp)
{
    _IO_size_t written;

    written = fwrite((const char *)&w, sizeof(w), 1, fp);

    return written != EOF  ? 0 : EOF;
}


/****************************************************************************
*   Function   : fread
*   Description: This function replaces the libc fread function; it reads at
*                most count objects of size size and stores them in buf.
*   Parameters : buf - pointer to location to store results
*                size - size of each object to be read
*                count - number of objects to be read
*                fp - stream to read from
*   Effects    : position in fp advances and buf contains objects read
*   Returned   : Number of objects read, EOF or error on failure
****************************************************************************/
size_t fread (void *buf, size_t size, size_t count, FILE *fp)
{
    jksem_lockfile(fp);
    count = _IO_fread(buf, size, count, fp);
    jksem_unlockfile(fp);

    return count;
}


/****************************************************************************
*   Function   : ftell
*   Description: This function replaces the libc ftell function; it reports
*                the current file position in fp.
*   Parameters : fp - stream to being interrogated
*   Effects    : none
*   Returned   : Current position or -1 on error.
****************************************************************************/
long int ftell (FILE *fp)
{
    long int ret;

    jksem_lockfile(fp);
    ret = _IO_ftell(fp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : fseek
*   Description: This function replaces the libc fseek function; it changes
*                the current position in fp.
*   Parameters : fp - stream to changed
*                offset - how far to move new position
*                whence - starting position to move offset from
*   Effects    : Position in file stream, fp, is adjusted
*   Returned   : Non-zero on error.
****************************************************************************/
int fseek(FILE *fp, long int offset, int whence)
{
    jksem_lockfile(fp);
    whence = _IO_fseek(fp, offset, whence);
    jksem_unlockfile(fp);

    return whence;
}


/****************************************************************************
*   Function   : rewind
*   Description: This function replaces the libc rewind function; it changes
*                the current position in fp to the beginning of the file.
*   Parameters : fp - stream to changed
*   Effects    : Position in file stream, fp, is changed to the begining.
*   Returned   : None
****************************************************************************/
void rewind(FILE* fp)
{
    jksem_lockfile(fp);
    _IO_rewind(fp);
    jksem_unlockfile(fp);
}


/****************************************************************************
*   Function   : fwrite
*   Description: This function replaces the libc fwrite function; it writes
*                most count objects of size size and from buf into fp.
*   Parameters : buf - pointer to location to of data to write
*                size - size of each object to be written
*                count - number of objects to be written
*                fp - stream to be written to
*   Effects    : position in fp advances and as buf is appended to it.
*   Returned   : Number of objects written.
****************************************************************************/
size_t fwrite(const void *buf, size_t size, size_t count, FILE *fp)
{
    jksem_lockfile(fp);
    count = _IO_fwrite(buf, size, count, fp);
    jksem_unlockfile(fp);

    return count;
}


/****************************************************************************
*   Function   : getdelim
*   Description: This function replaces the libc getdelim function; it reads
*                most n characters from fp, stopping at the delimiter
*                character.
*   Parameters : lineptr - location to store results
*                n - maximum number of characters to read
*                delimiter - delimiter to read to
*                fp - stream to be read
*   Effects    : Position in fp advances and lineptr will store the results
*                of the read.  lineptr will be expanded if it is too small
*                to store the results of the read.
*   Returned   : Number of characters read
****************************************************************************/
ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *fp)
{
    ssize_t ret;

    jksem_lockfile(fp);
    ret = __getdelim (lineptr, n, delimiter, fp);
    jksem_unlockfile(fp);

    return ret;
}


/****************************************************************************
*   Function   : gets
*   Description: This function replaces the libc gets function; it reads
*                from stdin, until it reaches a '\n'.
*   Parameters : buf - location to store results of read
*   Effects    : position in stdin advances until '\n' as read data is
*                stored in buf.
*   Returned   : buf, or NULL if error occurred
****************************************************************************/
char *gets(char *buf)
{
    jksem_lockfile(_IO_stdin);
    buf = _IO_gets(buf);
    jksem_unlockfile(_IO_stdin);

    return buf;
}


/****************************************************************************
*   Function   : puts
*   Description: This function replaces the libc puts function; it writes
*                a null terminated string to stdout, replacing the null with
*                a newline.
*   Parameters : buf - string to write
*   Effects    : Contents of buf are written to stdout.
*   Returned   : Non-negative value, or EOF on error.
****************************************************************************/
int puts(const char *buf)
{
    int ret;

    jksem_lockfile(_IO_stdout);
    ret = _IO_puts(buf);
    jksem_unlockfile(_IO_stdout);

    return ret;
}


/****************************************************************************
*   Function   : setbuffer
*   Description: This function replaces the libc setbuffer function; it
*                makes fp a buffered stream, using buf as it's buffer.  If
*                buf is NULL, fp will be unbuffered.
*   Parameters : fp - stream to buffer
*                buf - buffer to use
*                size - size of buffer
*   Effects    : fp becomes buffered by buf.
*   Returned   : None
****************************************************************************/
void setbuffer(FILE *fp, char *buf, size_t size)
{
    jksem_lockfile(fp);
    _IO_setbuffer(fp, buf, size);
    jksem_unlockfile(fp);
}


/****************************************************************************
*   Function   : setvbuf
*   Description: This function replaces the libc setvbuf function; it
*                makes fp a buffered stream, using buf as it's buffer.
*                mode specifies the buffering mode.
*   Parameters : fp - stream to buffer
*                buf - buffer to use
*                mode - the mode of the buffer (full, line, or none)
*                size - size of buffer
*   Effects    : fp becomes buffered by buf using mode mode.
*   Returned   : Non-zero on success.
****************************************************************************/
int setvbuf (FILE *fp, char *buf, int mode, size_t size)
{
    jksem_lockfile(fp);
    mode = _IO_setvbuf (fp, buf, mode, size);
    jksem_unlockfile(fp);

    return mode;
}


/****************************************************************************
*   Function   : setlinebuf
*   Description: This function replaces the libc setlinebuf function; it
*                makes steram a line buffered stream.
*   Parameters : stream - stream to buffer
*   Effects    : steam becomes line buffered and buffer is allocated.
*   Returned   : None
****************************************************************************/
void setlinebuf(FILE *stream)
{
    jksem_lockfile(stream);
    _IO_setvbuf(stream, NULL, 1, 0);
    jksem_unlockfile(stream);
}


/****************************************************************************
*   Function   : setbuf
*   Description: This function replaces the libc setbuf function; if buf is
*                not NULL, fp is buffered by buf using defined size BUFSIZE.
*                If buf is NULL, fp will be unbuffered
*   Parameters : fp - stream to be buffered
*                buf - buffer to use
*   Effects    : Either fp is fully buffered by buf, or made unbuffered if
*                buf is NULL.
*   Returned   : None
****************************************************************************/
void setbuf(FILE *fp, char *buf)
{
    jksem_lockfile(fp);
    _IO_setbuffer(fp, buf, _IO_BUFSIZ);
    jksem_unlockfile(fp);
}


/****************************************************************************
*   Function   : ungetc
*   Description: This function replaces the libc ungetc function; it pushes
*                c back onto fp, so that it will be retrieved next read.
*   Parameters : c - unsigned character pushed back on stream
*                fp - stream to push character on to
*   Effects    : c is pushed back onto fp
*   Returned   : The character pushed, or EOF on error.
****************************************************************************/
int ungetc (int c, FILE *fp)
{
    jksem_lockfile(fp);
    c = _IO_ungetc(c, fp);
    jksem_unlockfile(fp);

    return c;
}


#include <stdarg.h>

/****************************************************************************
*   Function   : vfprintf
*   Description: This function replaces the libc vfprintf function; it
*                writes a formatted string to file stream s.
*   Parameters : s - The file stream to write to
*                format - formatting string for the output
*                ap - argument list used by format
*   Effects    : Formatted output is appended to file stream s.
*   Returned   : Number of characters printed, or negative on error.
****************************************************************************/
int vfprintf(FILE *s, const char *format, va_list ap)
{
    int ret;

    jksem_lockfile(s);
    ret = _IO_vfprintf(s, format, ap);
    jksem_unlockfile(s);

    return ret;
}


/****************************************************************************
*   Function   : vprintf
*   Description: This function replaces the libc vprintf function; it
*                writes a formatted string to stdout.
*   Parameters : format - formatting string for the output
*                ap - argument list used by format
*   Effects    : Formatted output is appended to stdout.
*   Returned   : Number of characters printed, or negative on error.
****************************************************************************/

int vprintf(const char *format, va_list ap)
{
    return vfprintf(stdout, format, ap);
}


/****************************************************************************
*   Function   : fprintf
*   Description: This function replaces the libc fprintf function; it
*                writes a formatted string to file stream s.
*   Parameters : s - The file stream to write to
*                format - formatting string for the output
*                ... - arguments used by format string
*   Effects    : Formatted output is appended to file stream s.
*   Returned   : Number of characters printed, or negative on error.
****************************************************************************/
int fprintf(FILE *s, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfprintf(s, format, args);
    va_end(args);

    return ret;
}


/****************************************************************************
*   Function   : printf
*   Description: This function replaces the libc printf function; it
*                writes a formatted string to stdout.
*   Parameters : format - formatting string for the output
*                ... - arguments used by format string
*   Effects    : Formatted output is appended to stdout.
*   Returned   : Number of characters printed, or negative on error.
****************************************************************************/
int printf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfprintf(stdout, format, args);
    va_end(args);

    return ret;
}


/****************************************************************************
*   Function   : perror
*   Description: This function replaces the libc perror function; it writes
*                s : predefined error string to stderr
*   Parameters : s - String to proceed error message.
*   Effects    : s : predefined error string is appended to stderr.
*   Returned   : None
****************************************************************************/
void perror(const char *s)
{
    int errno;

    errno = _jkthread_kludge[getpid()]->thr_errno;
    fprintf(stderr, "%s : %s\n", s, sys_errlist[errno]);
}
 

/****************************************************************************
*   Function   : vfscanf
*   Description: This function replaces the libc vfscanf function; it reads
*                formatted input from stream s.
*   Parameters : s - The file stream to read from
*                format - formatting string for input
*                ap - argument list used assign read values to
*   Effects    : Formatted input is read from stream s.
*   Returned   : The number of successful assignments, or EOF if EOF was
*                reached before all assignments were made
****************************************************************************/
int vfscanf(FILE *s, const char *format, va_list ap)
{
    int ret;

    jksem_lockfile(s);
    ret = _IO_vfscanf(s, format, ap, NULL);
    jksem_unlockfile(s);

    return ret;
}


/****************************************************************************
*   Function   : vscanf
*   Description: This function replaces the libc vscanf function; it reads
*                formatted input from stdin.
*   Parameters : format - formatting string for input
*                ap - argument list used assign read values to
*   Effects    : Formatted input is read from stdin.
*   Returned   : The number of successful assignments, or EOF if EOF was
*                reached before all assignments were made
****************************************************************************/
int vscanf(const char *format, va_list ap)
{
    return vfscanf(stdin, format, ap);
}


/****************************************************************************
*   Function   : fscanf
*   Description: This function replaces the libc fscanf function; it reads
*                formatted input from stream s.
*   Parameters : s - The file stream to read from
*                format - formatting string for input
*                ... - variable list used assign read values to
*   Effects    : Formatted input is read from stream s.
*   Returned   : The number of successful assignments, or EOF if EOF was
*                reached before all assignments were made
****************************************************************************/
int fscanf(FILE *s, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfscanf(s, format, args);
    va_end(args);

    return ret;
}


/****************************************************************************
*   Function   : scanf
*   Description: This function replaces the libc scanf function; it reads
*                formatted input from stdin.
*   Parameters : s - The file stream to read from
*                format - formatting string for input
*                ... - variable list used assign read values to
*   Effects    : Formatted input is read from stdin.
*   Returned   : The number of successful assignments, or EOF if EOF was
*                reached before all assignments were made
****************************************************************************/
int scanf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfscanf(stdin, format, args);
    va_end(args);

    return ret;
}


/****************************************************************************
*   Function   : fclose
*   Description: This function replaces the libc fclose function; it closes
*                stream fp, flushing it's buffer output, discarding it's
*                buffered input.  Then the buffer and stream are
*                deallocated. 
*   Parameters : fp - stream to close.
*   Effects    : fp is closed.  It and it's buffer are deallocated.  When
*                the stream is closed the lock associated with it is
*                deallocated.
*   Returned   : 0, or EOF on error.
****************************************************************************/
int fclose(FILE *fp)
{
    int status, fd;

    /* Wait for other threads to finish. */
    jksem_lockfile(fp);
    fd = fileno(fp);

    jksem_get(&_jk_fileio_sem);
    if(fp->_IO_file_flags & _IO_IS_FILEBUF)
    {
        status = _IO_file_close_it(fp);
    }
    else
    {
        status = fp->_flags & _IO_ERR_SEEN ? -1 : 0;
    }

    jksem_unlockfile(fp);
    jksem_killfilelock(fp);
    //%%%need to do whatever it takes to finish off FP here.
    jksem_release(&_jk_fileio_sem);

    if (fp != _IO_stdin && fp != _IO_stdout && fp != _IO_stderr)
    {
       free(fp);
    }

    return status;
}


/****************************************************************************
*   Function   : pclose
*   Description: This function replaces the libc pclose function; it is
*                identical to fclose.
*   Parameters : fp - steam to close
*   Effects    : See fclose
*   Returned   : 0, or EOF on error.
****************************************************************************/
int pclose(FILE *fp)
{
    return fclose(fp);
}


/****************************************************************************
*   Function   : getline
*   Description: This function replaces the libc getline function; it reads
*                most linelen characters from fp, stopping at the '\n'.
*   Parameters : lineptr - location to store results
*                linelen - maximum number of characters to read
*                fp - stream to be read
*   Effects    : Position in fp advances and lineptr will store the results
*                of the read.  lineptr will be expanded if it is too small
*                to store the results of the read.
*   Returned   : Number of characters read
****************************************************************************/
ssize_t getline(char **lineptr, size_t *linelen, FILE *fp)
{
    _IO_ssize_t retval;

    jksem_lockfile(fp);
    retval = __getdelim(lineptr, linelen, '\n', fp);
    jksem_unlockfile(fp);

    return retval;
}


/****************************************************************************
*   Function   : fflush
*   Description: This function replaces the libc fflush function; if forces
*                the contents of fp's buffer to be written out.  If fp is
*                NULL, all buffers will be flushed.
*   Parameters : fp - stream to flush.
*   Effects    : The contents of fp's buffer are written out.
*   Returned   : 0, or EOF on error
****************************************************************************/
int fflush(FILE *fp)
{
    int result;

    if (fp == NULL)
    {
        jksem_get(&_jk_fileio_sem);
        result = _IO_flush_all();
        jksem_release(&_jk_fileio_sem);
    }
    else
    {
        jksem_lockfile(fp);
        result = _IO_fflush(fp);
        jksem_unlockfile(fp);
    }

    return result;
}


/****************************************************************************
*   Function   : setfileno
*   Description: This function replaces the libc setfileno function; it sets
*                the file number of stream fp to the descriptor value fd.
*   Parameters : fp - file stream being modified
*                fd - file descriptor value to use
*   Effects    : Sets fp's file number to the descriptor value fd.
*   Returned   : None
*
*   %%% I think this will have problems, because the descriptor of the file
*       being locked might not be the same as the one being unlocked.
****************************************************************************/
void setfileno(FILE* fp, int fd)
{
    jksem_lockfile(fp);

    if ((fp->_flags & _IO_IS_FILEBUF) != 0)
    {
        fp->_fileno = fd;
    }
    jksem_unlockfile(fp);
}


/****************************************************************************
*   Function   : clearerr
*   Description: This function replaces the libc clearerr function; it
*                clears the error stsus of file fp.
*   Parameters : fp - file stream being cleared.
*   Effects    : Error status of fp is cleared
*   Returned   : None
****************************************************************************/
void clearerr(FILE* fp)
{
    jksem_lockfile(fp);
    _IO_clearerr(fp);
    jksem_unlockfile(fp);
}

#endif /* __JKFILEIO */
