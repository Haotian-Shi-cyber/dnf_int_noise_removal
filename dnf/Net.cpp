#include "Net.h"
#include "Layer.h"
#include "Neuron.h"

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <ctgmath>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iostream>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string>
#include <numeric>
#include <vector>

using namespace std;

//*************************************************************************************
//initialisation:
//*************************************************************************************

Net::Net(const int _nLayers, const int * const _nNeurons, const int _nInputs, const int _subject, const string _trial){
	nLayers = _nLayers; //no. of layers including inputs and outputs layers
	layers= new Layer*[nLayers];
	const int* nNeuronsp = _nNeurons; //number of neurons in each layer
	nInputs=_nInputs; // the no. of inputs to the network (i.e. the first layer)
	//cout << "nInputs: " << nInputs << endl;
	int nInput = 0; //temporary variable to use within the scope of for loop
	for (int i=0; i<nLayers; i++){
		int numNeurons= *nNeuronsp; //no. neurons in this layer
		if (i==0){nInput=nInputs;}
		/* no. inputs to the first layer is equal to no. inputs to the network */
		layers[i]= new Layer(numNeurons, nInput, _subject, _trial);
		nNeurons += numNeurons;
		nWeights += (numNeurons * nInput);
		nInput=numNeurons;
		/*no. inputs to the next layer is equal to the number of neurons
		 * in the current layer. */
		nNeuronsp++; //point to the no. of neurons in the next layer
	}
	nOutputs=layers[nLayers-1]->getnNeurons();
	errorGradient= new double[nLayers];
	//cout << "net" << endl;
}

Net::~Net(){
	for (int i=0; i<nLayers; i++){
		delete layers[i];
	}
	delete[] layers;
	delete[] errorGradient;
}

void Net::initNetwork(Neuron::weightInitMethod _wim, Neuron::biasInitMethod _bim, Neuron::actMethod _am){
	for (int i=0; i<nLayers; i++){
		layers[i]->initLayer(i, _wim, _bim, _am);
	}
}

void Net::setLearningRate(int _w_learningRate, int _b_learningRate){
	for (int i=0; i<nLayers; i++){
		layers[i]->setlearningRate(_w_learningRate, _b_learningRate);
	}
}

//*************************************************************************************
//forward propagation of inputs:
//*************************************************************************************

void Net::setInputs(const int* _inputs, const int scale, const unsigned int offset, const int n) {
	inputs=_inputs;
			// using std::chrono::high_resolution_clock;
    		// using std::chrono::duration_cast;
    		// using std::chrono::duration;
    		// using std::chrono::microseconds;
			// auto t1 = high_resolution_clock::now();
	layers[0]->setInputs(inputs, scale, offset, n); //sets the inputs to the first layer only
			// auto t2 = high_resolution_clock::now();
			// auto us_int = duration_cast<microseconds>(t2 - t1);
			// std::cout << us_int.count() << " us\n";	
}

void Net::propInputs(){
	for (int i=0; i<nLayers-1; i++){
		layers[i]->calcOutputs();
		for (int j=0; j<layers[i]->getnNeurons(); j++){
			int inputOuput = layers[i]->getOutput(j);
			layers[i+1]->propInputs(j, inputOuput);
		}
	}
	layers[nLayers-1]->calcOutputs();
	/* this calculates the final outoup of the network,
	 * i.e. the output of the final layer
	 * but this is not fed into any further layer*/
}

//*************************************************************************************
//back propagation of error
//*************************************************************************************

void Net::setError(int _leadError){
	/* this is only for the final layer */
	theLeadError = _leadError;
	//cout<< "lead Error: " << theLeadError << endl;
	layers[nLayers-1]->setError(theLeadError);
}

void Net::propErrorBackward(){
	int tempError = 0;
	int tempWeight = 0;
	for (int i = nLayers-1; i > 0 ; i--){
		for (int k = 0; k < layers[i-1]->getnNeurons(); k++){
			long sum = 0;
			for (int j = 0; j < layers[i]->getnNeurons(); j++){
				tempError = layers[i]->getNeuron(j)->getError();
				tempWeight = layers[i]->getWeights(j,k);
				sum += (tempError * tempWeight);
			}
			assert(std::isfinite(sum));
			layers[i-1]->getNeuron(k)->setBackpropError(sum);
		}
	}
	//cout << "--------------------------------------------------" << endl;
}

//*************************************************************************************
//exploding/vanishing gradient:
//*************************************************************************************

double Net::getGradient(Layer::whichGradient _whichGradient) {
	for (int i=0; i<nLayers; i++) {
		errorGradient[i] = layers[i]->getGradient(_whichGradient);
	}
	double gradientRatio = errorGradient[nLayers -1] / errorGradient[0] ; ///errorGradient[0];
	assert(std::isfinite(gradientRatio));
	return gradientRatio;
}

//*************************************************************************************
//learning:
//*************************************************************************************

void Net::updateWeights(){
	for (int i=nLayers-1; i>=0; i--){
		layers[i]->updateWeights();
	}
}

//*************************************************************************************
// getters:
//*************************************************************************************

int Net::getOutput(int _neuronIndex){
	return (layers[nLayers-1]->getOutput(_neuronIndex));
}

long Net::getSumOutput(int _neuronIndex){
	return (layers[nLayers-1]->getSumOutput(_neuronIndex));
}

int Net::getnLayers(){
	return (nLayers);
}

int Net::getnInputs(){
	return (nInputs);
}

Layer* Net::getLayer(int _layerIndex){
	assert(_layerIndex<nLayers);
	return (layers[_layerIndex]);
}

double Net::getWeightDistance(){
	double weightChange = 0 ;
	double weightDistance =0;
	for (int i=0; i<nLayers; i++){
		weightChange += layers[i]->getWeightChange();
	}
	weightDistance=sqrt(weightChange);
	// cout<< "Net: WeightDistance is: " << weightDistance << endl;
	return (weightDistance);
}

double Net::getLayerWeightDistance(int _layerIndex){
	return layers[_layerIndex]->getWeightDistance();
}

double Net::getWeights(int _layerIndex, int _neuronIndex, int _weightIndex){
	double weight=layers[_layerIndex]->getWeights(_neuronIndex, _weightIndex);
	return (weight);
}

int Net::getnNeurons(){
	return (nNeurons);
}

//*************************************************************************************
//saving and inspecting
//*************************************************************************************

void Net::saveWeights(){
	for (int i=0; i<nLayers; i++){
		layers[i]->saveWeights();
	}
}


void Net::snapWeights(string prefix, string _trial, int _subject){
	for (int i=0; i<nLayers; i++){
		layers[i]->snapWeights(prefix, _trial, _subject);
	}
}

void Net::snapWeightsMatrixFormat(string prefix){
	layers[0]->snapWeightsMatrixFormat(prefix);
}

void Net::printNetwork(){
	cout<< "This network has " << nLayers << " layers" <<endl;
	for (int i=0; i<nLayers; i++){
		cout<< "Layer number " << i << ":" <<endl;
		layers[i]->printLayer();
	}
	cout<< "The output(s) of the network is(are):";
	for (int i=0; i<nOutputs; i++){
		cout<< " " << this->getOutput(i);
	}
	cout<<endl;
}
