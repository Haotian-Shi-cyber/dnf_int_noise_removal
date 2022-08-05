#include <iostream>
#include <fstream>
#include <stdio.h>
#include <boost/circular_buffer.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <Iir.h>
#include <Fir1.h>
#include <memory>
#include "dynamicPlots.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <numeric>
#include <string>
#include <functional>
#include <assert.h>
#include <ctgmath>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <math.h>

#include "cvui.h"

#include <initializer_list>

//#define CVUI_IMPLEMENTATION
//#include "cvui.h"


using namespace std;
namespace cv {
    class Mat;
}

dynaPlots::dynaPlots(cv::Mat &_frame, int _plotW, int _plotH) {
    plotH = _plotH;
    plotW = _plotW;
    graphDX = plotW/2 - gapX * 2;
    frame = _frame;
    graphY = graphDY + gapY;
}
dynaPlots::~dynaPlots() = default;

void dynaPlots::plotMainSignals(std::vector<double> &outer,
				std::vector<double> &inner,
                                std::vector<double> &remover,
				std::vector<double> &fnn)
                                {

    int graphX = gapX;

    const double outer_min = *min_element(outer.begin(), outer.end());
    const double outer_max = *max_element(outer.begin(), outer.end());
    const double inner_min = *min_element(inner.begin(), inner.end());
    const double inner_max = *max_element(inner.begin(), inner.end());
    const double remover_min = *min_element(remover.begin(), remover.end());
    const double remover_max = *max_element(remover.begin(), remover.end());
    const double fnn_min = *min_element(fnn.begin(), fnn.end());
    const double fnn_max = *max_element(fnn.begin(), fnn.end());

    int step = 0;
    cvui::sparkline(frame, outer,     graphX, graphY * step + topOffset, graphDX, graphDY, 0xffffff); //white
    cvui::text(     frame,            graphX, graphY * step + topOffset + graphDY, "Outer: raw(b) & filtered(w) & end(gray)");
    cvui::printf(   frame,            graphX, graphY * step + topOffset + graphDY + lineEnter, "min: %+.5lf max: %+.5lf", outer_min, outer_max);
    step ++;
    cvui::sparkline(frame, inner,     graphX, graphY * step + topOffset, graphDX, graphDY, 0xffffff); //white
    cvui::text(     frame,            graphX, graphY * step + topOffset + graphDY, "inner: raw(b) & filtered(w)");
    cvui::printf(   frame,            graphX, graphY * step + topOffset + graphDY + lineEnter, "min: %+.5lf max: %+.5lf", inner_min, inner_max);
    step ++;
    cvui::sparkline(frame, remover,   graphX, graphY * step + topOffset, graphDX, graphDY, 0xffffff); //white
    cvui::text(     frame,            graphX, graphY * step + topOffset + graphDY, "remover");
    cvui::printf(   frame,            graphX, graphY * step + topOffset + graphDY + lineEnter, "min: %+.5lf max: %+.5lf", remover_min, remover_max);
    step ++;
    cvui::sparkline(frame, fnn,       graphX, graphY * step + topOffset, graphDX, graphDY, 0xffffff); //white
    cvui::text(     frame,            graphX, graphY * step + topOffset + graphDY, "DNF output");
    cvui::printf(   frame,            graphX, graphY * step + topOffset + graphDY + lineEnter, "min: %+.5lf max: %+.5lf", fnn_min, fnn_max);
    step = 2;
    graphX += plotW/2;
}

void dynaPlots::plotTitle(std::string title, long count, double duration,const char* fullPath){
    cvui::printf(frame, gapX,           titleY, "Subject %s -- Sample number: %ld , Duration: %2.3f [s], %s",
                 title.c_str(),count, duration, fullPath);
}
