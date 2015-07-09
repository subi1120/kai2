/* File : kai.i */
%module kai
%{
/* Put headers and other declarations here */
#define SWIG_FILE_WITH_INIT
#include "kai.h"
%}

%include "kaiDefinition.h"
%include "kaiMsg.h"
%include "kaiServer.h"
%include "kaiSocket.h"
%include "kai.h"