#include <iostream>
#include <chrono>


class Neuron {
    public:
    Neuron(int _nInputs);

       	void setInput(int _index, long _value);
    private:
        int *inputs = 0;
        int nInputs = 0;
};

Neuron::Neuron(int _nInputs){
    nInputs = _nInputs;
    inputs = new int[nInputs];
}

void Neuron::setInput(int _index,  long _value) {
	inputs[_index] = _value;
}

void setInputs(const int* const _inputs) {  
    Neuron **neurons = 0;
    neurons = new Neuron*[300];
    for(int i = 0; i < 300; i++){
        neurons[i] = new Neuron(300);
    }
	/*this is only for the first layer*/
    int count = 0;
	const int* inputs = _inputs;    
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::nanoseconds;
    auto t1 = high_resolution_clock::now();


	for (int i = 0; i<300 ; i++){
		Neuron** neuronsp = neurons;
        Neuron* sp = *neuronsp;
        for(int j = 0; j < 300; j++){
            sp->setInput(j,inputs[j]);
        }
        neuronsp++;
	}
    auto t2 = high_resolution_clock::now();
	auto us_int = duration_cast<nanoseconds>(t2 - t1);
	std::cout << us_int.count() << " ns\n";	

}


int main(){
    
    int* inputs = new int[300];
 /* 
            Neuron **neurons = 0;
            neurons = new Neuron*[300];

 			using std::chrono::high_resolution_clock;
    		using std::chrono::duration_cast;
    		using std::chrono::duration;
    		using std::chrono::nanoseconds;
			auto t1 = high_resolution_clock::now();

            for(int i = 0; i < 300; i++){
                neurons[i] = new Neuron(300);
            }
             auto t2 = high_resolution_clock::now();
		    auto us_int = duration_cast<nanoseconds>(t2 - t1);
		    std::cout << us_int.count() << " ns\n";	
	           
     
            Neuron** neuronsp = neurons;
            for(int i = 0; i < 300; i++){
			    (*neuronsp)->setInput(0,i);
			    neuronsp++; //point to the next neuron
            }
   */

    setInputs(inputs);
}
