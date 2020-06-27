/*
 * Copyright (c) 2020 Sergey Zolotarev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <errno.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#ifdef _WIN32
  #include <windows.h>
#endif

#define MAX_ERROR_LENGTH 1024

static THREAD_LOCAL char static_error_buf[MAX_ERROR_LENGTH];

const char *xstrerror(int error)
{
  char *buf = static_error_buf;
  size_t size = sizeof(static_error_buf);

#ifdef _WIN32

  if (error < WSABASEERR) {
    /* C/POSIX error code */
    strerror_s(buf, size, error);
    return buf;
  } else {
    /* Windows Sockets error code */
    DWORD count;
    count = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error,
        0,
        buf,
        (DWORD)size,
        NULL);
    if (count == 0) {
      return "Unknown error";
    }
    /* Remove trailing newline and '.' characters (if any) */
    while (buf[count - 1] == '\r'
        || buf[count - 1] == '\n'
        || buf[count - 1] == '.') {
      buf[--count] = '\0';
    }
    return buf;
  }

#else /* _WIN32 */

  strerror_r(error, buf, size);
  return static_error_buf;

#endif /* !_WIN32 */
}

int xerrno_get(void)
{
#ifdef _WIN32
  int error = GetLastError();
  /* Mapping of WSA error codes to POSIX errors codes */
  switch (error) {
    case WSAEINTR:
      return EINTR;
    case WSAEBADF:
      return EBADF;
    case WSAEACCES:
      return EACCES;
    case WSAEFAULT:
      return EFAULT;
    case WSAEINVAL:
      return EINVAL;
    case WSAEMFILE:
      return EMFILE;
    case WSAEWOULDBLOCK:
      return EWOULDBLOCK;
    case WSAEINPROGRESS:
      return EINPROGRESS;
    case WSAEALREADY:
      return EALREADY;
    case WSAENOTSOCK:
      return ENOTSOCK;
    case WSAEDESTADDRREQ:
      return EDESTADDRREQ;
    case WSAEMSGSIZE:
      return EMSGSIZE;
    case WSAEPROTOTYPE:
      return EPROTOTYPE;
    case WSAENOPROTOOPT:
      return ENOPROTOOPT;
    case WSAEPROTONOSUPPORT:
      return EPROTONOSUPPORT;
    /*
    case WSAESOCKTNOSUPPORT:
      return ESOCKTNOSUPPORT;
    */
    case WSAEOPNOTSUPP:
      return EOPNOTSUPP;
    /*
    case WSAEPFNOSUPPORT:
      return EPFNOSUPPORT;
    */
    case WSAEAFNOSUPPORT:
      return EAFNOSUPPORT;
    case WSAEADDRINUSE:
      return EADDRINUSE;
    case WSAEADDRNOTAVAIL:
      return EADDRNOTAVAIL;
    case WSAENETDOWN:
      return ENETDOWN;
    case WSAENETUNREACH:
      return ENETUNREACH;
    case WSAENETRESET:
      return ENETRESET;
    case WSAECONNABORTED:
      return ECONNABORTED;
    case WSAECONNRESET:
      return ECONNRESET;
    case WSAENOBUFS:
      return ENOBUFS;
    case WSAEISCONN:
      return EISCONN;
    case WSAENOTCONN:
      return ENOTCONN;
    /*
    case WSAESHUTDOWN:
      return ESHUTDOWN;
    case WSAETOOMANYREFS:
      return ETOOMANYREFS;
    */
    case WSAETIMEDOUT:
      return ETIMEDOUT;
    case WSAECONNREFUSED:
      return ECONNREFUSED;
    case WSAELOOP:
      return ELOOP;
    case WSAENAMETOOLONG:
      return ENAMETOOLONG;
    /*
    case WSAEHOSTDOWN:
      return EHOSTDOWN;
    */
    case WSAEHOSTUNREACH:
      return EHOSTUNREACH;
    case WSAENOTEMPTY:
      return ENOTEMPTY;
    /*
    case WSAEPROCLIM:
      return EPROCLIM;
    case WSAEUSERS:
      return EUSERS;
    case WSAEDQUOT:
      return EDQUOT;
    case WSAESTALE:
      return ESTALE;
    case WSAEREMOTE:
      return EREMOTE;
    */
    case WSAVERNOTSUPPORTED:
      return WSAVERNOTSUPPORTED;
    /*
    case WSAEDISCON:
      return EDISCON;
    case WSAENOMORE:
      return ENOMORE;
    case WSAECANCELLED:
      return ECANCELLED;
    */
    default:
      return error;
  }
#else
  return errno;
#endif
}
