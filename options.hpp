/* options.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _OPTIONS_H
#define _OPTIONS_H
// 2023-05-27
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <string>
#include <iostream>

namespace ismt
{
    // 2023-05-27
    struct SolverOptions{
        std::string File = "";
        int Time = 1200;
        // std::string Combined_Solver_Name = "none";
        bool GA = true;
        bool MA = true;
        bool VO = true;
        int Alpha = 1536;
        int Beta = 7;
        int K = 16;
        double Gamma = 0.5;
        bool DG = true;
        bool CM = true;
        int MaxBW = 128;

        bool parse(int argc, char* argv[]){
            for(int i=1;i<argc;i++){
                if(argv[i][0]=='-'){
                    std::string param = std::string(argv[i]);
                    if(param.size() > 3){
                        if(param[1]=='T' && param[2]==':'){
                            // time
                            Time = std::atoi(param.substr(3).c_str());
                        }
                        // else if(param[1]=='C' && param[2]=='S' && param[3]=='N' && param[4]==':'){
                        //     Combined_Solver_Name = param.substr(5);
                        // }
                        else if(param[1]=='G' && param[2]=='A' && param[3]==':'){
                            if(param.substr(4) == "true") GA = true;
                            else GA = false;
                        }
                        else if(param[1]=='M' && param[2]=='A' && param[3]==':'){
                            if(param.substr(4) == "true") MA = true;
                            else if(param.substr(4) == "false") MA = false;
                        }
                        else if(param[1]=='V' && param[2]=='O' && param[3]==':'){
                            if(param.substr(4) == "true") VO = true;
                            else if(param.substr(4) == "false") VO = false;
                        }
                        else if(param[1]=='A' && param[2]=='l' && param[3]=='p' && param[4]=='h' && param[5]=='a' && param[6]==':'){
                            Alpha = std::atoi(param.substr(7).c_str());
                        }
                        else if(param[1]=='B' && param[2]=='e' && param[3]=='t' && param[4]=='a' && param[5]==':'){
                            Beta = std::atoi(param.substr(6).c_str());
                        }
                        else if(param[1]=='K' && param[2]==':'){
                            K = std::atoi(param.substr(3).c_str());
                        }
                        else if(param[1]=='G' && param[2]=='a' && param[3]=='m' && param[4]=='m' && param[5]=='a' && param[6]==':'){
                            Gamma = std::atof(param.substr(7).c_str());
                        }
                        else if(param[1]=='D' && param[2]=='G' && param[3]==':'){
                            if(param.substr(4) == "true") DG = true;
                            else if(param.substr(4) == "false") DG = false;
                        }
                        else if(param[1]=='C' && param[2]=='M' && param[3]==':'){
                            if(param.substr(4) == "true") CM = true;
                            else if(param.substr(4) == "false") CM = false;
                        }
                        else if(param[1]=='M' && param[2]=='a' && param[3]=='x' && param[4]=='B' && param[5]=='W' && param[6]==':'){
                            MaxBW = std::atoi(param.substr(7).c_str());
                        }
                    }
                    else if(param[1]=='h'){
                        print_help();
                    }
                    else assert(false);
                }
                else{
                    File = std::string(argv[i]);
                    // if multiple file, the final file is valid.
                }
            }
            if(File=="") return false;
            else return true;
        }

        void print(){
            std::cout<<"Params: {"<<std::endl;
            std::cout<<"\tfile: "<<File<<std::endl;
            std::cout<<"\tTime: "<<Time<<std::endl;
            // std::cout<<"\tCombined Solver Name: "<<Combined_Solver_Name<<std::endl;
            std::cout<<"\tGA: "<<GA<<std::endl;
            std::cout<<"\tMA: "<<MA<<std::endl;
            std::cout<<"\tVO: "<<VO<<std::endl;
            std::cout<<"\tAlpha: "<<Alpha<<std::endl;
            std::cout<<"\tBeta: "<<Beta<<std::endl;
            std::cout<<"\tK: "<<K<<std::endl;
            std::cout<<"\tGamma: "<<Gamma<<std::endl;
            std::cout<<"\tDG: "<<DG<<std::endl;
            std::cout<<"\tCM: "<<CM<<std::endl;
            std::cout<<"\tMaxBW: "<<MaxBW<<std::endl;
            std::cout<<"}"<<std::endl;
        }
        void print_help(){
            std::cout<<"BLAN 1.smt2 (the smt-lib file to solve)"<<std::endl;
            std::cout<<"\t-T:1200 (time limit (s), default 1200)"<<std::endl;
            std::cout<<"\t-GA:true (Greedy Addtion, default true)"<<std::endl;
            std::cout<<"\t-MA:true (Multiplication Adaptation, default true)"<<std::endl;
            std::cout<<"\t-VO:true (Vote, default true)"<<std::endl;
            std::cout<<"\t-Alpha:1536 (Alpha of Multiplication Adaptation, default 1536)"<<std::endl;
            std::cout<<"\t-Beta:7 (Beta of Multiplication Adaptation, default 7)"<<std::endl;
            std::cout<<"\t-K:16 (K of Clip, default 16)"<<std::endl;
            std::cout<<"\t-Gamma:0.5 (Gamma of Vote, default 0.5)"<<std::endl;
            std::cout<<"\t-DG:true (Distinct Graph, default true)"<<std::endl;
            std::cout<<"\t-CM:true (Coefficient Matching, default true)"<<CM<<std::endl;
            std::cout<<"\t-MaxBW:128 (Maximum Bit-Width, default 128)"<<MaxBW<<std::endl;
        }
    };

    // static SolverOptions* StaticOptions = nullptr;
   
} // namespace ismt



#endif