﻿#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <memory>
#include <FreeImage.h>

#include "Tools.h"
#include "Tensors/Tensor.h"

namespace Neuro
{
    Random g_Rng;
    bool g_ImageLibInitialized = false;

    //////////////////////////////////////////////////////////////////////////
    Random& GlobalRng()
    {
        return g_Rng;
    }

    //////////////////////////////////////////////////////////////////////////
    void GlobalRngSeed(unsigned int seed)
    {
        g_Rng = Random(seed);
    }

    //////////////////////////////////////////////////////////////////////////
	int AccNone(const Tensor& target, const Tensor& output)
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	int AccBinaryClassificationEquality(const Tensor& target, const Tensor& output)
	{
		int hits = 0;
		for (uint32_t n = 0; n < output.Batch(); ++n)
			hits += target(0, 0, 0, n) == roundf(output(0, 0, 0, n)) ? 1 : 0;
		return hits;
	}

	//////////////////////////////////////////////////////////////////////////
	int AccCategoricalClassificationEquality(const Tensor& target, const Tensor& output)
	{
        Tensor targetArgMax = target.ArgMax(EAxis::Sample);
        Tensor outputArgMax = output.ArgMax(EAxis::Sample);

		int hits = 0;
		for (uint32_t i = 0; i < targetArgMax.Length(); ++i)
			hits += targetArgMax(i) == outputArgMax(i) ? 1 : 0;
		return hits;
	}

    //////////////////////////////////////////////////////////////////////////
    void DeleteData(vector<tensor_ptr_vec_t>& data)
    {
        for (auto& v : data)
            DeleteContainer(v);
        data.clear();
    }

    //////////////////////////////////////////////////////////////////////////
	float Clip(float value, float min, float max)
	{
		return value < min ? min : (value > max ? max : value);
	}

	//////////////////////////////////////////////////////////////////////////
	int Sign(float value)
	{
		return value < 0 ? -1 : (value > 0 ? 1 : 0);
	}

    //////////////////////////////////////////////////////////////////////////
    vector<float> LinSpace(float start, float stop, uint32_t num, bool endPoint)
    {
        vector<float> result;
        float interval = (stop - start) / num;
        for (uint32_t i = 0; i < num; ++i)
        {
            result.push_back(start);
            start += interval;
        }

        if (endPoint)
            result.push_back(stop);

        return result;
    }

    //////////////////////////////////////////////////////////////////////////
	std::string ToLower(const string& str)
	{
		string result = str;

		for (uint32_t i = 0; i < str.length(); ++i)
			result[i] = tolower(str[i]);

		return result;
	}

    //////////////////////////////////////////////////////////////////////////
    string TrimStart(const string& str, const string& chars /*= "\t\n\v\f\r "*/)
    {
        string ret = str;
        ret.erase(0, str.find_first_not_of(chars));
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////
    string TrimEnd(const string& str, const string& chars /*= "\t\n\v\f\r "*/)
    {
        string ret = str;
        ret.erase(str.find_last_not_of(chars) + 1);
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////
    vector<string> Split(const string& str, const string& delimiter)
    {
        vector<string> result;
        size_t pos = 0;
        size_t lastPos = 0;

        while ((pos = str.find(delimiter, lastPos)) != string::npos)
        {
            result.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = pos + delimiter.length();
        }

        result.push_back(str.substr(lastPos, str.length() - lastPos));

        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    string Replace(const string& str, const string& pattern, const string& replacement)
    {
        string result = str;
        string::size_type n = 0;
        while ((n = result.find(pattern, n)) != string::npos)
        {
            result.replace(n, pattern.size(), replacement);
            n += replacement.size();
        }

        return result;
    }

    //////////////////////////////////////////////////////////////////////////
	string GetProgressString(int iteration, int maxIterations, const string& extraStr, int barLength, char blankSymbol, char fullSymbol)
	{
		int maxIterLen = (int)to_string(maxIterations).length();
		float step = maxIterations / (float)barLength;
		int currStep = min((int)ceil(iteration / step), barLength);

		stringstream ss;
		ss << setw(maxIterLen) << iteration << "/" << maxIterations << " [";
		for (int i = 0; i < currStep - 1; ++i)
			ss << fullSymbol;
		ss << ((iteration == maxIterations) ? "=" : ">");
		for (int i = 0; i < barLength - currStep; ++i)
			ss << blankSymbol;
		ss << "] " << extraStr;
		return ss.str();
	}

    //////////////////////////////////////////////////////////////////////////
    uint32_t EndianSwap(uint32_t x)
    {
        return (x >> 24) |
               ((x << 8) & 0x00FF0000) |
               ((x >> 8) & 0x0000FF00) |
               (x << 24);
    }

    unique_ptr<char[]> LoadBinFileContents(const string& file)
    {
        ifstream stream(file, ios::in | ios::binary | ios::ate);
        assert(stream);
        auto size = stream.tellg();
        unique_ptr<char[]> buffer(new char[size]);
        stream.seekg(0, std::ios::beg);
        stream.read(buffer.get(), size);
        stream.close();
        return buffer;
    }

    //////////////////////////////////////////////////////////////////////////
    void LoadMnistData(const string& imagesFile, const string& labelsFile, Tensor& input, Tensor& output, bool generateImage, int maxImages)
    {
        auto ReadBigInt32 = [](const unique_ptr<char[]>& buffer, size_t offset)
        {
            auto ptr = reinterpret_cast<uint32_t*>(buffer.get());
            auto value = *(ptr + offset);
            return EndianSwap(value);
        };

        auto imagesBuffer = LoadBinFileContents(imagesFile);
        auto labelsBuffer = LoadBinFileContents(labelsFile);

        uint32_t magic1 = ReadBigInt32(imagesBuffer, 0); // discard
        uint32_t numImages = ReadBigInt32(imagesBuffer, 1);
        uint32_t imgWidth = ReadBigInt32(imagesBuffer, 2);
        uint32_t imgHeight = ReadBigInt32(imagesBuffer, 3);

        int magic2 = ReadBigInt32(labelsBuffer, 0); // 2039 + number of outputs
        int numLabels = ReadBigInt32(labelsBuffer, 1);

        maxImages = maxImages < 0 ? numImages : min<int>(maxImages, numImages);

        int outputsNum = magic2 - 2039;

        FIBITMAP* image = nullptr;
        RGBQUAD imageColor;
        imageColor.rgbRed = imageColor.rgbGreen = imageColor.rgbBlue = 255;
        uint32_t imageRows = (uint32_t)ceil(sqrt((float)maxImages));
        uint32_t imageCols = (uint32_t)ceil(sqrt((float)maxImages));

        const uint32_t IMG_WIDTH = imageRows * imgWidth;
        const uint32_t IMG_HEIGHT = imageCols * imgHeight;
        if (generateImage)
        {
            ImageLibInit();
            image = FreeImage_Allocate(IMG_WIDTH, IMG_HEIGHT, 24);
            FreeImage_FillBackground(image, &imageColor);
        }

        input = Tensor(Shape(imgWidth, imgHeight, 1, maxImages));
        output = Tensor(Shape(1, outputsNum, 1, maxImages));

        uint8_t* pixelOffset = reinterpret_cast<uint8_t*>(imagesBuffer.get() + 16);
        uint8_t* labelOffset = reinterpret_cast<uint8_t*>(labelsBuffer.get() + 8);

        for (uint32_t i = 0; i < (uint32_t)maxImages; ++i)
        {
            for (uint32_t y = 0; y < imgWidth; ++y)
            for (uint32_t x = 0; x < imgHeight; ++x)
            {
                uint8_t color = *(pixelOffset++);
                input(x, y, 0, i) = color / 255.f;

                if (image)
                {
                    imageColor.rgbRed = imageColor.rgbGreen = imageColor.rgbBlue = color;
                    FreeImage_SetPixelColor(image, (i % imageCols) * imgWidth + x, IMG_HEIGHT - ((i / imageCols) * imgHeight + y) - 1, &imageColor);
                }
            }

            uint8_t label = *(labelOffset++);
            output(0, label, 0, i) = 1;
        }

        if (image)
        {
            FreeImage_Save(FIF_PNG, image, (imagesFile + ".png").c_str());
            FreeImage_Unload(image);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //void SaveMnistData(const Tensor& input, const Tensor& output, const string& imagesFile, const string& labelsFile)
    //{
    //    auto WriteBigInt32 = [](ostream& stream, int v)
    //    {
    //        stream << EndianSwap(v);
    //    };

    //    ofstream fsLabels(labelsFile, ios::binary);
    //    ofstream fsImages(imagesFile, ios::binary);
    //    
    //    uint32_t imgHeight = input.Height();
    //    uint32_t imgWidth = input.Width();
    //    uint32_t outputsNum = output.BatchLength();

    //    WriteBigInt32(fsImages, 1337); // discard
    //    WriteBigInt32(fsImages, input.Batch());
    //    WriteBigInt32(fsImages, imgHeight);
    //    WriteBigInt32(fsImages, imgWidth);

    //    WriteBigInt32(fsLabels, 2039 + outputsNum);
    //    WriteBigInt32(fsLabels, output.Batch());

    //    for (uint32_t i = 0; i < input.Batch(); ++i)
    //    {
    //        for (uint32_t y = 0; y < imgHeight; ++y)
    //        for (uint32_t x = 0; x < imgWidth; ++x)
    //            fsImages << (unsigned char)(input(x, y, 0, i) * 255);

    //        for (uint32_t j = 0; j < outputsNum; ++j)
    //        {
    //            if (output(0, j, 0, i) == 1)
    //            {
    //                fsLabels << (unsigned char)j;
    //            }
    //        }
    //    }
    //}

    //////////////////////////////////////////////////////////////////////////
    void LoadCSVData(const string& filename, int outputsNum, Tensor& input, Tensor& output, bool outputsOneHotEncoded, int maxLines)
    {
        ifstream infile(filename.c_str());
        string line;

        vector<float> inputValues;
        vector<float> outputValues;
        uint32_t batches = 0;
        uint32_t inputBatchSize = 0;

        while (getline(infile, line))
        {
            if (maxLines >= 0 && batches >= (uint32_t)maxLines)
                break;

            if (line.empty())
                continue;

            auto tmp = Split(line, ",");

            ++batches;
            inputBatchSize = (int)tmp.size() - (outputsOneHotEncoded ? 1 : outputsNum);

            for (int i = 0; i < (int)inputBatchSize; ++i)
                inputValues.push_back((float)atof(tmp[i].c_str()));

            for (int i = 0; i < (outputsOneHotEncoded ? 1 : outputsNum); ++i)
            {
                float v = (float)atof(tmp[inputBatchSize + i].c_str());

                if (outputsOneHotEncoded)
                {
                    for (int e = 0; e < outputsNum; ++e)
                        outputValues.push_back(e == (int)v ? 1.f : 0.f);
                }
                else
                    outputValues.push_back(v);
            }
        }

        input = Tensor(inputValues, Shape(1, inputBatchSize, 1, batches));
        output = Tensor(outputValues, Shape(1, outputsNum, 1, batches));
    }

    //////////////////////////////////////////////////////////////////////////
    void ImageLibInit()
    {
        if (!g_ImageLibInitialized)
        {
            FreeImage_Initialise();
            g_ImageLibInitialized = true;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    Tqdm::Tqdm(uint32_t maxIterations)
        : m_MaxIterations(maxIterations)
    {
        m_Timer.Start();
        NextStep();
    }

    //////////////////////////////////////////////////////////////////////////
    void Tqdm::NextStep()
    {        
        const int BAR_LENGTH = 30;
        const char blankSymbol = (char)176;
        const char fullSymbol = (char)219;
        const char activeSymbol = (char)178;

        ++m_Iteration;

        float pct = m_Iteration / (float)m_MaxIterations * 100;

        stringstream stream;
        stream << right << setw(4) << (to_string((int)pct) + "%") << '[';

        float step = m_MaxIterations / (float)BAR_LENGTH;
        int currStep = min((int)ceil(m_Iteration / step), BAR_LENGTH);

        for (int i = 0; i < currStep - 1; ++i)
            stream << fullSymbol;
        stream << ((m_Iteration == m_MaxIterations) ? fullSymbol : activeSymbol);
        for (int i = 0; i < BAR_LENGTH - currStep; ++i)
            stream << blankSymbol;

        stream << '|';

        if (m_Iteration > 0)
        {
            float averageTimePerStep = m_Timer.ElapsedMilliseconds() / (float)m_Iteration;
            stream << " eta: " << fixed << setprecision(3) << averageTimePerStep * (m_MaxIterations - m_Iteration) * 0.001f << "s [" << m_Timer.ElapsedMilliseconds() * 0.001f << "s]  ";
        }

        cout << stream.str();

        if (m_Iteration == m_MaxIterations)
            cout << endl;
        else
        {
            for (uint32_t i = 0; i < stream.str().length(); ++i)
                cout << '\b';
        }

        /*int maxIterLen = (int)to_string(m_MaxIterations).length();
        float step = maxIterations / (float)barLength;
        int currStep = min((int)ceil(iteration / step), barLength);

        stringstream ss;
        ss << setw(maxIterLen) << iteration << "/" << maxIterations << " [";
        for (int i = 0; i < currStep - 1; ++i)
            ss << fullSymbol;
        ss << ((iteration == maxIterations) ? "=" : ">");
        for (int i = 0; i < barLength - currStep; ++i)
            ss << blankSymbol;
        ss << "] " << extraStr;
        return ss.str();*/
        
    }
}