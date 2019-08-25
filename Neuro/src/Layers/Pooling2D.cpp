﻿#include "Layers/Pooling2D.h"

namespace Neuro
{
    //////////////////////////////////////////////////////////////////////////
    Pooling2D::Pooling2D(LayerBase* inputLayer, int filterSize, int stride, int padding, EPoolingMode mode, const string& name)
        : LayerBase(__FUNCTION__, inputLayer, Tensor::GetPooling2DOutputShape(inputLayer->OutputShape(), filterSize, filterSize, stride, padding, padding), nullptr, name)
    {
        m_Mode = mode;
        m_FilterSize = filterSize;
        m_Stride = stride;
        m_Padding = padding;
    }

    //////////////////////////////////////////////////////////////////////////
    Pooling2D::Pooling2D(Shape inputShape, int filterSize, int stride, int padding, EPoolingMode mode, const string& name)
        : LayerBase(__FUNCTION__, inputShape, Tensor::GetPooling2DOutputShape(inputShape, filterSize, filterSize, stride, padding, padding), nullptr, name)
    {
        m_Mode = mode;
        m_FilterSize = filterSize;
        m_Stride = stride;
        m_Padding = padding;
    }

    //////////////////////////////////////////////////////////////////////////
    Pooling2D::Pooling2D()
    {

    }

    //////////////////////////////////////////////////////////////////////////
    LayerBase* Pooling2D::GetCloneInstance() const
    {
        return new Pooling2D();
    }

    //////////////////////////////////////////////////////////////////////////
    void Pooling2D::OnClone(const LayerBase& source)
    {
        __super::OnClone(source);

        auto sourcePool = static_cast<const Pooling2D&>(source);
        m_Mode = sourcePool.m_Mode;
        m_FilterSize = sourcePool.m_FilterSize;
        m_Stride = sourcePool.m_Stride;
    }

    //////////////////////////////////////////////////////////////////////////
    void Pooling2D::FeedForwardInternal(bool training)
    {
        m_Inputs[0]->Pool2D(m_FilterSize, m_Stride, m_Mode, m_Padding, m_Output);
    }

    //////////////////////////////////////////////////////////////////////////
    void Pooling2D::BackPropInternal(Tensor& outputGradient)
    {
        m_Inputs[0]->Pool2DGradient(m_Output, *m_Inputs[0], outputGradient, m_FilterSize, m_Stride, m_Mode, m_Padding, m_InputsGradient[0]);
    }

    //////////////////////////////////////////////////////////////////////////
    MaxPooling2D::MaxPooling2D(LayerBase* inputLayer, int filterSize, int stride, int padding, const string& name)
        : Pooling2D(inputLayer, filterSize, stride, padding, EPoolingMode::Max, name)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    MaxPooling2D::MaxPooling2D(Shape inputShape, int filterSize, int stride, int padding, const string& name)
        : Pooling2D(inputShape, filterSize, stride, padding, EPoolingMode::Max, name)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    AvgPooling2D::AvgPooling2D(LayerBase* inputLayer, int filterSize, int stride, int padding, const string& name)
        : Pooling2D(inputLayer, filterSize, stride, padding, EPoolingMode::Avg, name)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    AvgPooling2D::AvgPooling2D(Shape inputShape, int filterSize, int stride, int padding, const string& name)
        : Pooling2D(inputShape, filterSize, stride, padding, EPoolingMode::Avg, name)
    {
    }
}