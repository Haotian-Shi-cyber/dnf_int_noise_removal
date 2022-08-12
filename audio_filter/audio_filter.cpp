#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdio.h>
#include <boost/circular_buffer.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/lexical_cast.hpp>
#include <Iir.h>
#include <chrono>
#include <string>
#include <ctime>
#include <thread>         // std::thread
#include <memory>
#include <numeric>
#include "dnf.h"
#include "parameters.h"
#include "dynamicPlots.h"
#include "wavRead/wavread.h"

#define CVUI_IMPLEMENTATION
#include "cvui.h"

using namespace std;
constexpr int ESC_key = 27;

void addSOXheader(fstream &f) {
	f << "; Sample Rate " << fs << endl;
	f << "; Channels 1" << endl;
}

void processOneExperiment(const int expIndex, const bool showPlots = true) {
	std::srand(1);

	// file path prefix for the results
	std::string outpPrefix = "results";

	std::filesystem::create_directory(outpPrefix);

	fprintf(stderr,"Starting DNF on experiment %d, filename = %s.\n",expIndex, outpPrefix.c_str());

	const int samplesNoLearning = 3 * fs / signalWithNoiseHighpassCutOff;
	
	fprintf(stderr,"nTapsDNF = %d\n",nTapsDNF);
	
	boost::circular_buffer<int> oo_buf(bufferLength);
	boost::circular_buffer<int> io_buf(bufferLength);
	boost::circular_buffer<int> ro_buf(bufferLength);
	boost::circular_buffer<int> f_nno_buf(bufferLength);

	WAVread wavread;

	long count = 0;
	
	//setting up the interactive window and the dynamic plot class
	auto frame = cv::Mat(cv::Size(plotW, plotH), CV_8UC3);
	dynaPlots* plots = NULL;
	if (showPlots) {
		cvui::init(WINDOW, 1);
		plots = new dynaPlots(frame, plotW, plotH);
	}

	//create files for saving the data and parameters
	const std::string expDir = "/exp";
	const std::string sd = outpPrefix + expDir + std::to_string(expIndex);
	std::filesystem::create_directory(sd);
	
	DNF dnf(NLAYERS,nTapsDNF,fs,ACTIVATION);

// FILES
	fstream signalWithNoise_file;
	signalWithNoise_file.open(sd + "/signalWithNoise.dat", fstream::out);
	addSOXheader(signalWithNoise_file);

	fstream noiseref_file;
	noiseref_file.open(sd + "/noiseref.dat", fstream::out);
	addSOXheader(noiseref_file);

	fstream dnfOut_file;
	dnfOut_file.open(sd + "/dnf_out.dat", fstream::out);
	addSOXheader(dnfOut_file);

	fstream dnfRemover_file;
	dnfRemover_file.open(sd + "/dnf_remover.dat", fstream::out);

	fstream wdistance_file;
	wdistance_file.open(sd + "/weight_distance.tsv", fstream::out);
	
	char fullpath2data[256];

	sprintf(fullpath2data,audioPath,expIndex);
	char* buffer = wavread.open(fullpath2data);



	// if (r < 0) {
	// 	cout << "Unable to open file: " << fullpath2data << endl;
	// 	exit(1); // terminate with error
	// }
	wavread.printHeaderInfo();
	
	//setting up all the filters required
	Iir::Butterworth::HighPass<filterorder> noiseref_filterHP;
	noiseref_filterHP.setup(fs,noiserefHighpassCutOff);
	Iir::Butterworth::HighPass<filterorder> signalWithNoise_filterHP;
	signalWithNoise_filterHP.setup(fs,signalWithNoiseHighpassCutOff);
	
	fprintf(stderr,"signalWithNoise_gain = %f, noiseref_gain = %f, remover_gain = %f\n",signalWithNoise_gain,noiseref_gain,remover_gain);

	// main loop processsing sample by sample
	while (buffer) {
		int l = (buffer[0]) + (buffer[1] << 8) + (buffer[2] << 16);
		int left = (l * 256);
		left = left >> 8;
		buffer = buffer + 3;

		int r = (buffer[0]) + (buffer[1] << 8) + (buffer[2] << 16);
		int right = (r * 256);
		right = right >> 8;
		buffer = buffer + 3;

		int signalWithNoise_raw_data = left; // signal + noise
		int noiseref_raw_data = right; // noise ref
		
		//A) SIGNALWITHNOISE ELECTRODE:
		//1) ADJUST & AMPLIFY
		const double signalWithNoise_raw = signalWithNoise_gain * signalWithNoise_raw_data;
		int signalWithNoise_filtered = int(signalWithNoise_filterHP.filter(signalWithNoise_raw));// 20Hz

		//B) NOISEREF ELECTRODE:
		//1) ADJUST & AMPLIFY
		const double noiseref_raw = noiseref_gain * noiseref_raw_data;
		int noiserefhp = int(noiseref_filterHP.filter(noiseref_raw));

		/* testing speed now begin */
		    //  using std::chrono::high_resolution_clock;
    		//  using std::chrono::duration_cast;
    		//  using std::chrono::duration;
    		//  using std::chrono::microseconds;
			//  auto t1 = high_resolution_clock::now();
		int f_nn = dnf.filter(signalWithNoise_filtered,noiserefhp);
			//  auto t2 = high_resolution_clock::now();
			//  auto us_int = duration_cast<microseconds>(t2 - t1);
			//  std::cout << us_int.count() << " us\n";
		/* testing speed now end */

		// when sample reaches 7500, start to learn
		if (count > (samplesNoLearning+nTapsDNF)){
			dnf.getNet().setLearningRate(dnf_learning_rate, 0);
		} else {
			dnf.getNet().setLearningRate(0, 0);
		}

		double t = (double)count / fs;
		
		wdistance_file << dnf.getNet().getWeightDistance();
		for(int i=0; i < NLAYERS; i++ ) {
			wdistance_file << "\t" << dnf.getNet().getLayerWeightDistance(i);
		}
		wdistance_file << endl;

		// SAVE SIGNALS INTO FILES
		// undo the gain so that the signal is again in volt
		signalWithNoise_file << t << " " << dnf.getDelayedSignal()/signalWithNoise_gain << endl;
		noiseref_file << t << " " << noiseref_raw_data/noiseref_gain << " " << endl;
		dnfOut_file << t << " " << dnf.getOutput()/signalWithNoise_gain/8388608.0f << endl;
		dnfRemover_file << dnf.getRemover()/signalWithNoise_gain << endl;
		
		// PUT VARIABLES IN BUFFERS
		// 1) MAIN SIGNALS
		oo_buf.push_back(noiseref_raw_data); // outer is noise
		io_buf.push_back(dnf.getDelayedSignal());// inner is signalwithnoise
		ro_buf.push_back(dnf.getRemover());// ref is remover
		f_nno_buf.push_back(f_nn);
		
		// PUTTING BUFFERS IN VECTORS FOR PLOTS
		// MAIN SIGNALS
		std::vector<double> oo_plot(oo_buf.begin(), oo_buf.end());
		std::vector<double> io_plot(io_buf.begin(), io_buf.end());
		std::vector<double> ro_plot(ro_buf.begin(), ro_buf.end());
		std::vector<double> f_nno_plot(f_nno_buf.begin(), f_nno_buf.end());
		
		if (plots) {
			frame = cv::Scalar(60, 60, 60);
			if (0 == (count % 10)) {
				plots->plotMainSignals(oo_plot,
						       io_plot,
						       ro_plot,
						       f_nno_plot);
				plots->plotTitle(sd, count, count / fs,fullpath2data);
				cvui::update();
				cv::imshow(WINDOW, frame);
				
				if (cv::waitKey(1) == ESC_key) {
					break;
				}
			}
		}
		count++;
	}
	wavread.close();
	dnfOut_file.close();
	dnfRemover_file.close();
	signalWithNoise_file.close();
	noiseref_file.close();
	wdistance_file.close();
	if (plots) delete plots;
	cout << "The program has reached the end of the input file" << endl;
}



int main(int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr,"Usage: %s [-a]\n",argv[0]);
		fprintf(stderr,"       -a calculates all experiments one by one without screen output.\n");
		fprintf(stderr,"       -b calculates all experiments multi-threaded without screen output.\n");
		fprintf(stderr,"       Press ESC in the plot window to cancel the program.\n");
		return 0;
	}
	
	if (strcmp(argv[1],"-a") == 0) {
		for(int i = 0; i < nExp; i++) {
			processOneExperiment(i+1, false);
		}
		return 0;
	}
	
	// if (strcmp(argv[1],"-b") == 0) {
	// 	std::thread* workers[nExp];
	// 	for(int i = 0; i < nExp; i++) {
	// 		workers[i] = new std::thread(processOneExperiment,i+1);
	// 	}
	// 	for(int i = 0; i < nExp; i++) {
	// 		workers[i]->join();
	// 		delete workers[i];
	// 	}
	// 	return 0;
	// }
	
	const int experiment = atoi(argv[1]);
	if ( (experiment < 1) || (experiment > nExp) ) {
		fprintf(stderr,"Exp number of out range.\n");
		return -1;
	}
	processOneExperiment(experiment);
	return 0;
}
