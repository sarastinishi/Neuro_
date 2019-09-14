﻿#pragma once

#include <string>
#include <vector>

#include "Random.h"
#include "Data.h"
#include "Stopwatch.h"

namespace Neuro
{
    using namespace std;

	class Tensor;

    const float _EPSILON = 10e-7f;
    
    Random& GlobalRng();
    void GlobalRngSeed(unsigned int seed);

    int AccNone(const Tensor& target, const Tensor& output);
    int AccBinaryClassificationEquality(const Tensor& target, const Tensor& output);
    int AccCategoricalClassificationEquality(const Tensor& target, const Tensor& output);

	template<typename C> void DeleteContainer(C& container);
    void DeleteData(vector<tensor_ptr_vec_t>& data);

    float Clip(float value, float min, float max);
	int Sign(float value);

    vector<float> LinSpace(float start, float stop, uint32_t num = 50, bool endPoint = true);

	string ToLower(const string& str);
    string TrimStart(const string& str, const string& chars = "\t\n\v\f\r ");
    string TrimEnd(const string& str, const string& chars = "\t\n\v\f\r ");
    vector<string> Split(const string& str, const string& delimiter);
    string Replace(const string& str, const string& pattern, const string& replacement);

    string GetProgressString(int iteration, int maxIterations, const string& extraStr = "", int barLength = 30, char blankSymbol = '.', char fullSymbol = '=');

    void LoadMnistData(const string& imagesFile, const string& labelsFile, Tensor& input, Tensor& output, bool normalize, bool generateImage = false, int maxImages = -1);
    //void SaveMnistData(const Tensor& input, const Tensor& output, const string& imagesFile, const string& labelsFile);
    void LoadCifar10Data(const string& imagesBatchFile, Tensor& input, Tensor& output, bool normalize, bool generateImage = false, int maxImages = -1);
    void LoadCSVData(const string& filename, int outputsNum, Tensor& inputs, Tensor& outputs, bool outputsOneHotEncoded = false, int maxLines = -1);

    void ImageLibInit();
    extern bool g_ImageLibInitialized;

    class Tqdm
    {
    public:
        Tqdm(uint32_t maxIterations);
        void NextStep(uint32_t iterations = 1);
        string Str() const { return m_Stream.str(); }
        __int64 ElapsedMilliseconds() const { return m_Timer.ElapsedMilliseconds(); }

    private:
        int m_Iteration = -1;
        uint32_t m_MaxIterations;
        Stopwatch m_Timer;
        stringstream m_Stream;
    };

    //////////////////////////////////////////////////////////////////////////
    template<typename C>
    void DeleteContainer(C& container)
    {
        for (auto elem : container)
            delete elem;
        container.clear();
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    void Shuffle(vector<T>& list)
    {
        uint32_t n = (int)list.size();
        while (n-- > 1)
        {
            int k = GlobalRng().Next(n + 1);
            swap(list[k], list[n]);
        }
    }
}