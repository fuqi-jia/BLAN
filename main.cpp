/* main.cpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#include "qfnia/qfnia.hpp"
#include "options.hpp"
#include <csignal>
#include <unistd.h>

using namespace ismt;

void sigalarm_handler(int){std::cout<<"unknown"<<std::endl; exit(0);}
int main(int argc, char* argv[]){
    if(argc >= 1){
        SolverOptions* StaticOptions = new SolverOptions();
        if(!StaticOptions->parse(argc, argv)){
            delete StaticOptions;
            return 0;
        }
        // StaticOptions->print();

        signal(SIGALRM, sigalarm_handler);
        signal(SIGINT, sigalarm_handler);

        // signal(SIGABRT, sigalarm_handler);
        // signal(SIGFPE, sigalarm_handler);
        // signal(SIGILL, sigalarm_handler);
        // signal(SIGSEGV, sigalarm_handler);
        // signal(SIGTERM, sigalarm_handler);
        alarm(StaticOptions->Time);


        qfnia_solver solver;
        solver.solve(StaticOptions);

        delete StaticOptions;
    }
}