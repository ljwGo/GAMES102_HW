#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>
#include <vector>
#include <random>
#include <iostream>

using namespace Eigen;
using namespace std;

struct NN;
struct NNOutput;
struct UserData {};

// One Sample Cost Derivative Function(One predict output how to influence one cost)
typedef float (*OSCDFn)(float oneOutPredict, float realOut, UserData* userData);
// Multiple Sample Cost Derivative Function(One sample cost how to influence multiple sample cost)
typedef float (*MSCDFn)(float oneCost, size_t sampleCout, UserData* userData);
typedef float (*MSCFn)(std::vector<float>& oneSampleCosts, size_t sampleCout, UserData* userData);
typedef float (*OSCFn)(const VectorXf& predictOut, const VectorXf& realOut, UserData* userData);


struct NNLayer {
	size_t len = 0;
	size_t count = 0;  // count = len + 1. Because per layer need a constraint threshold.

	NN* nn;
	NNLayer* preNNLayer = nullptr;
	NNLayer* nextNNLayer = nullptr;

	float (*activeFn)(const float& input) = nullptr;
	float (*activeDerivativeFn)(const float& input) = nullptr;

	MatrixXf w;
	std::vector<float> chainDerivative;

	MatrixXf outputs;
	MatrixXf inputs;

	NNLayer(int len, NN* nn);

	virtual void CalcChainDerivative();

	virtual void BPUpdateWeights();

	virtual void SetNextLayer(NNLayer* nextLayer);

	virtual void SetPreLayer(NNLayer* preLayer);

	virtual void CalcOutput();

	virtual void CalcInput();

	virtual void PrintInputs() {
		cout << inputs << endl;
	}

	virtual void PrintOutputs() {
		cout << outputs << endl;
	}

	virtual void PrintWeight() {
		cout << w << endl;
	}
};

// BatchSize can only one.
class NN {
public:
	NN(size_t size) {
		layers.reserve(size);
	}

	void AddLayer(NNLayer* layer) noexcept;
	void Train();
	VectorXf Predict(VectorXf&& input);

	~NN() {
		for (NNLayer* layer : layers) {
			if (layer != nullptr) delete layer;
		}
	}

private:
	void TrainOnce();

public:
	float learnRatio;
	size_t batchSize;
	size_t maxIterCount;

	MatrixXf realOuts;
	MatrixXf originInput;
	OSCDFn oscdFn = nullptr;
	MSCDFn mscdFn = nullptr;
	MSCFn mscFn = nullptr;
	OSCFn oscFn = nullptr;

	float costThreshold;
	float mulSampleCost;
	std::vector<float> oneSampleCosts;

	std::vector<NNLayer*> layers;
	MatrixXf ys;
	MatrixXf xs;
};

struct NNInput : public NNLayer
{
	NNInput(int len, NN* nn) : NNLayer(len, nn) {}

	virtual void SetPreLayer(NNLayer* preLayer);

	virtual void CalcOutput();

	virtual void CalcInput();

	virtual void CalcChainDerivative();

	virtual void BPUpdateWeights();
};

struct NNOutput : public NNLayer
{
	NNOutput(int len, NN* nn) : NNLayer(len, nn) {}

	virtual void SetNextLayer();

	virtual void CalcOutput();

	virtual void CalcCost();

	virtual void CalcChainDerivative();
};