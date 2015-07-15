#ifndef __CSS_CMD_ROUTE_H__
#define __CSS_CMD_ROUTE_H__

#include "css_stream.h"

int css_cmd_dispatch(css_stream_t* stream, char* package, ssize_t length);
#endif