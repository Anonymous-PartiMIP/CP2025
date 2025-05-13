#include "../Scheduler/Scheduler.h"

std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();

int main(int argc, char **argv)
{
    // setbuf(stdout, NULL);
    INIT_ARGS

    // __global_paras.print_change();
    // RecreateDirectory(OPT(logPath));
    Scheduler scheduler;
    scheduler.Optimize();
    return 0;
}