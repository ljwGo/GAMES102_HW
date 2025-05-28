// 激活函数
// 层数(纵向和横向)
// 损失函数
// 样本
// 标记
// batchsize
// update method
// 正则项

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include "main.h"
#include "rasterizer.hpp"

using namespace cv;

float GetRandomFloat() {
	static std::random_device rd;
	static std::mt19937 rng(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(rng);
}

float SimpleDESM(float oneOutPredict, float realOut, UserData* userData) {
	return oneOutPredict - realOut;
}

float SimpleESM(const VectorXf& predictOut, const VectorXf& realOut, UserData* userData) {
	return 0.5 * (predictOut - realOut).dot(predictOut - realOut);
}

float GaussFn(const float& x) {
	return exp(-0.5 * x * x);
}

float GaussDFn(const float& x) {
	return -exp(-0.5 * x * x) * x;
}

float LUFn(const float& x) {
	return x > 0 ? x : 0;
}

float LUDFn(const float& x) {
	return x > 0 ? 1 : 0;
}

float SelfFn(const float& x) {
	return x;
}

float SelfDFn(const float& x) {
	return 1;
}

NNLayer::NNLayer(int len, NN* nn)
{
	this->len = len;
	this->count = len + 1;
	this->nn = nn;

	chainDerivative.resize(len);
}

void NNLayer::CalcChainDerivative()
{
	assert(nextNNLayer != nullptr);
	if (nn->batchSize == 1) {
		for (int i = 0; i < len; ++i) {
			chainDerivative[i] = 0;
			for (int j = 0; j < nextNNLayer->chainDerivative.size(); ++j) {
				chainDerivative[i] +=
					nextNNLayer->chainDerivative[j] *
					nextNNLayer->w(i, j);
			}
			chainDerivative[i] *= activeDerivativeFn(inputs(0, i));
		}
	}
	else {
		assert(false);
	}
}

// 当激活函数导致输出为0时, dp / dw恒为0. 常量项恒为1.
void NNLayer::BPUpdateWeights()
{
	if (nn->batchSize == 1) {
		for (int i = 0; i < w.rows(); ++i) {
			for (int j = 0; j < w.cols(); ++j) {
				float dw = nn->learnRatio * chainDerivative[j] * preNNLayer->outputs(0, i);
				//assert(abs(dw) > (1e-6));
				w(i, j) -= dw;
			}
		}
	}
}

void NNLayer::SetNextLayer(NNLayer* nextLayer)
{
	assert(nextLayer != nullptr);
	this->nextNNLayer = nextLayer;
}

void NNLayer::SetPreLayer(NNLayer* preLayer)
{
	assert(preLayer != nullptr);

	this->preNNLayer = preLayer;
	preLayer->SetNextLayer(this);

	// Init w matrix
	w = MatrixXf(preLayer->count, len);
	for (int i = 0; i < w.rows(); ++i) {
		for (int j = 0; j < w.cols(); ++j) {
			w(i, j) = GetRandomFloat();
		}
	}
}

void NNLayer::CalcOutput()
{
	outputs = MatrixXf::Ones(inputs.rows(), inputs.cols() + 1);
	for (int i = 0; i < inputs.rows(); ++i) {
		for (int j = 0; j < inputs.cols(); ++j) {
			outputs(i, j) = activeFn(inputs(i, j));
		}
	}
}

void NNLayer::CalcInput()
{
	assert(preNNLayer != nullptr);
	// 上一层输出 * 本层权重 = 本层输入(不包含阈值结点)
	inputs = preNNLayer->outputs * w;
}

void NNOutput::SetNextLayer()
{
		assert(false);
}

void NNOutput::CalcOutput()
{
	outputs = MatrixXf::Ones(inputs.rows(), inputs.cols());
	for (int i = 0; i < inputs.rows(); ++i) {
		for (int j = 0; j < inputs.cols(); ++j) {
			outputs(i, j) = activeFn(inputs(i, j));
		}
	}
}

void NNOutput::CalcCost()
{
	if (nn->batchSize == 1) {
		nn->oneSampleCosts.clear();
		float cost = nn->oscFn(outputs.row(0), nn->realOuts.row(0), nullptr);
		nn->mulSampleCost = cost;
		nn->oneSampleCosts.push_back(cost);
	}
	else {
		nn->oneSampleCosts.clear();
		for (int i = 0; i < outputs.rows(); ++i) {
			nn->oneSampleCosts.push_back(nn->oscFn(outputs.row(i), nn->realOuts.row(i), nullptr));
		}
		nn->mulSampleCost = nn->mscFn(nn->oneSampleCosts, outputs.rows(), nullptr);
	}
}

void NNOutput::CalcChainDerivative()
{
	// p represent partial: p(E) / p(E_k) * p(E_k) / p(y_predict) * p(y_predict) / p(input) 
	if (nn->batchSize == 1) {
		for (int i = 0; i < len; ++i) {
			chainDerivative[i] =
				nn->oscdFn(outputs(0, i), nn->realOuts(0, i), nullptr) *
				activeDerivativeFn(inputs(0, i));
		}
	}
	else {
		for (int i = 0; i < len; ++i) {
			chainDerivative[i] = 0;
			for (int j = 0; j < outputs.rows(); ++j) {
				chainDerivative[i] +=
					nn->mscdFn(nn->oneSampleCosts[j], outputs.rows(), nullptr) *
					nn->oscdFn(outputs(j, i), nn->realOuts(j, i), nullptr) *
					activeDerivativeFn(inputs(j, i));
			}
		}
	}
}

void NNInput::SetPreLayer(NNLayer* preLayer)
{
	assert(false);
}

void NNInput::CalcOutput()
{
	outputs = MatrixXf::Ones(nn->originInput.rows(), nn->originInput.cols() + 1);
	for (int i = 0; i < nn->originInput.rows(); ++i) {
		for (int j = 0; j < nn->originInput.cols(); ++j) {
			outputs(i, j) = nn->originInput(i, j);
		}
	}
}

void NNInput::CalcInput()
{
	assert(false);
}

void NNInput::CalcChainDerivative()
{
	assert(false);
}

void NNInput::BPUpdateWeights()
{
	assert(false);
}

void NN::AddLayer(NNLayer* layer) noexcept
{
	if (layers.size() >= 1) {
		layer->SetPreLayer(layers.back());
	}
	layers.emplace_back(std::move(layer));
}

void NN::Train()
{
	for (int i = 0; i < maxIterCount; ++i) {
		TrainOnce();
		if (mulSampleCost < costThreshold) return;
	}
}

VectorXf NN::Predict(VectorXf&& input)
{
	originInput = std::move(input);
	for (int i = 0; i < layers.size(); ++i) {
		if (i == 0) {
			layers[i]->CalcOutput();
			//layers[i]->PrintOutputs();
			continue;
		}

		layers[i]->CalcInput();
		//layers[i]->PrintInputs();
		layers[i]->CalcOutput();
		//layers[i]->PrintOutputs();
	}
	return layers[layers.size() - 1]->outputs.row(0);
}

void NN::TrainOnce()
{
	for (size_t startIx = 0; startIx < xs.rows(); startIx += batchSize) {
		size_t count = startIx + batchSize > xs.rows() ? xs.rows() - startIx : batchSize;

		originInput = xs.block(startIx, 0, count, xs.cols());
		realOuts = ys.block(startIx, 0, count, ys.cols());

		// 计算网络输出
		for (int i = 0; i < layers.size(); ++i) {
			if (i == 0) {
				layers[i]->CalcOutput();
				continue;
			}

			layers[i]->CalcInput();
			layers[i]->CalcOutput();
		}

		// 计算链式
		for (int i = layers.size() - 1; i > 0; --i) {
			if (i == layers.size() - 1) {
				NNOutput* output = (NNOutput*)(layers[i]);
				output->CalcCost();
				output->CalcChainDerivative();
				continue;
			}

			layers[i]->CalcChainDerivative();
		}

		// 误差逆向传播
		for (int i = layers.size() - 1; i > 0; --i) {
			layers[i]->BPUpdateWeights();
		}
	}
}

// 窗口
const size_t row = 600;
const size_t column = 800;
const float scale = 200.f;
const float invScale = 1 / scale;

// 网络
NN nn(3);

// 渲染器
std::vector<Vector2f> points;
rst::rasterizer r(column, row);

void onMouseCallBack(int e, int x, int y, int flag, void* userdata) {
	if (e == EVENT_LBUTTONUP) {
		points.push_back(Vector2f(x, y));
		r.clear();

		for (const Vector2f& p : points) {
			r.drawSquare(p);
		}
	}
}

int main(void) {

	NNLayer* input = new NNInput(1, &nn);
	NNLayer* hiddenLayer = new NNLayer(10, &nn);
	NNLayer* output = new NNOutput(1, &nn);

	// 设置激活函数
	hiddenLayer->activeDerivativeFn = GaussDFn;
	hiddenLayer->activeFn = GaussFn;
	output->activeDerivativeFn = SelfDFn;
	output->activeFn = SelfFn;

	nn.AddLayer(input);
	nn.AddLayer(hiddenLayer);
	nn.AddLayer(output);

	// 网络参数设置
	nn.batchSize = 1;
	nn.costThreshold = 0.001;
	nn.learnRatio = 0.005;
	nn.oscdFn = SimpleDESM;
	nn.oscFn = SimpleESM;
	nn.maxIterCount = 100;

	// UI
	namedWindow("curve");
	resizeWindow("curve", Size(row, column));
	setMouseCallback("curve", onMouseCallBack);

	int key = 0;

	while (key != 27) {
		// 特别注意32是Mat中每个元素的内存空间, F是类型, C3是3通道的意思
		// 所以有时一个像素(rgba, 32位), 需要使用Mat的四个位置来存储(选择8U, 即8bit)
		switch (key)
		{
		case 'j':
			nn.xs = MatrixXf(points.size(), 1);
			nn.ys = MatrixXf(points.size(), 1);

			for (int i = 0; i < points.size(); ++i) {
				nn.xs(i, 0) = points[i][0] * invScale;
				nn.ys(i, 0) = points[i][1] * invScale;
			}

			nn.Train();

			for (int i = 0; i < column; ++i) {
				VectorXf tmp(1);
				tmp << (i * invScale);
				int y = nn.Predict(std::move(tmp))(0) * scale;
				r.set_pixel(Vector2f(i, y), Vector3f(255, 0, 255));
			}
			break;
		}

		Mat image(row, column, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);
		cv::imshow("curve", image);

		key = waitKey(10);
	}

	return 0;
}