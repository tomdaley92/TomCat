/*

Thomas Daley
11/13/2016
WinCat.h

*/
#ifndef WINCAT_H
#define WINCAT_H

#include <windows.h>
#include <stdio.h>
#include "AsyncStreamReader.h"
#include "pipes.h"

class WinCat {
    private:
        AsyncStreamReader *input = NULL;
        FILE *output;
        struct PipeHandles pipes;
        int launched;
        char *filename;

        int Launch();

    public:
        WinCat(char *filename);
        ~WinCat();
        int Process(SOCKET ClientSocket);
        
};

#endif /* WINCAT_H */
