#ifndef CONFIG_H
#define CONFIG_H

struct ConfigType {
    struct {
        int activeObjectId = -1;
        bool realTimeDiff = false;
        bool realTimeGrad = false;
    } Interactivity;

    struct {
        bool useLogScale = false;
        float differentialScale = 1.0f;
        float targetMaxLogScale = 1.0f;
        bool showObstacles = false;
    } Display;

    struct {
        double q = 6.0;
        double p = 12.0;
        double theta = 10.0;
        double intersection_theta = 10000000000.0;
        double farFieldSeparation = 0.25;
        double nearFieldSeparation = 10.0;
        double nearFieldIntersection = 10000000000.0;
        int maxRefinement = 30;
        int clusterSplitThreshold = 2;
        int parallelPercolationDepth = 5;
        int threadCount = 1;
    } TPE;

    struct {
        int nLoopIterations = 1;
    } Opt;

    struct {
        int verbosity = 1;  // 0: no output, 1: some output, 2: detailed output
    } Debug;
};

#endif  // CONFIG_H