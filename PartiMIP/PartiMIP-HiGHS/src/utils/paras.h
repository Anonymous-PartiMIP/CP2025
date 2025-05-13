#ifndef _paras_hpp_INCLUDED
#define _paras_hpp_INCLUDED

#include "header.h"

//        name,               type,  short-name, must-need, default, low, high, comments
#define PARAS \
    PARA( cutoff            ,   double   , 'c'  ,  false , 300   , 0  , 1e8     , "Cutoff time") \
    PARA( threadNum         ,   int      , 't'  ,  false , 8     , 2  , 192     , "Thread number") \
    PARA( DecreaseVar       ,   int      , 'D'  ,  false , 1     , 1  , 50      , "Decrease var branch")\
    PARA( Obstacle          ,   double   , 'X'  ,  false , 0.5   , 0  , 1       , "obstacle of branch")\
    PARA( initNodes         ,   double   , 'I'  ,  false , 0.5   , 0  , 10000000, "init nodes")\
    PARA( defualtPrecision  ,   int      , '\0' ,  false , 1     , 0  , 1       , "defualt precision")\
    PARA( MIPGap            ,   double   , '\0' ,  false , 0     , 0  , 1       , "MIPGap")\
    PARA( AbsMIPGap         ,   double   , '\0' ,  false , 0     , 0  , 1       , "MIPGapAbs")\
//            name,   short-name, must-need, default, comments
#define STR_PARAS \
    STR_PARA( instance   , 'i'   ,  true    , "" , ".mps format instance")\
    STR_PARA( logPath    , 'l'   ,  false    , "log/" , "")
    
struct paras 
{
#define PARA(N, T, S, M, D, L, H, C) \
    T N = D;
    PARAS 
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    std::string N = D;
    STR_PARAS
#undef STR_PARA

void parse_args(int argc, char *argv[]);
void print_change();
void set_paras();
};

#define INIT_ARGS __global_paras.parse_args(argc, argv);

extern paras __global_paras;

#define OPT(N) (__global_paras.N)

#endif