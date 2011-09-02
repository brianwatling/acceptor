#ifndef _ACCEPTOR_H_INCLUDED_
#define _ACCEPTOR_H_INCLUDED_

extern "C" int acceptor_connect(const char* path);

extern "C" int acceptor_accept(int acceptor);

#endif

