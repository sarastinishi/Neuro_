﻿#include <sstream>
#include <iomanip>

#include "Optimizers/Adam.h"
#include "Tensors/Tensor.h"
#include "Tensors/TensorOpCpu.h"
#include "ComputationalGraph/Variable.h"
#include "ComputationalGraph/Constant.h"
#include "ComputationalGraph/Graph.h"
#include "Tools.h"

namespace Neuro
{    
	//////////////////////////////////////////////////////////////////////////
    Adam::Adam(float lr, float beta1, float beta2, float epsilon)
        :Adam(new Constant(lr), beta1, beta2, epsilon)
	{
	}

    //////////////////////////////////////////////////////////////////////////
    Adam::Adam(TensorLike* lr, float beta1, float beta2, float epsilon)
        : m_LearningRate(lr), m_Beta1(beta1), m_Beta2(beta2), m_Epsilon(epsilon)
    {
        // we will be using it's host value so I want to offload it rather than wait for memcpy
        m_LearningRate->SetAlwaysOffload(true);
    }

    //////////////////////////////////////////////////////////////////////////
    OptimizerBase* Adam::Clone() const
    {
        return new Adam(*this);
    }

    //////////////////////////////////////////////////////////////////////////
	string Adam::ToString()
	{
		stringstream ss;
		ss << setprecision(5) << "Adam(lr=" << m_LearningRate << ", beta1=" << m_Beta1 << ", beta2=" << m_Beta2 << ")";
		return ss.str();
	}

	//////////////////////////////////////////////////////////////////////////
	const char* Adam::ClassName() const
	{
		return "Adam";
	}

    //////////////////////////////////////////////////////////////////////////
    Adam::MinimizationOperation::MinimizationOperation(const vector<TensorLike*>& losses, const vector<Variable*>& vars, Variable* globalStep, TensorLike* lr, float beta1, float beta2, float epsilon)
        : Operation(MergeVectors({ losses, vector<TensorLike*>{ lr } }), "adam_minimize"), m_Vars(vars), m_GlobalStep(globalStep), m_LearningRate(lr), m_Beta1(beta1), m_Beta2(beta2), m_Epsilon(epsilon)
    {
        m_Order = Graph::Default()->BuildBackwardOrder(losses, m_NodesAffectingLosses, vars);
    }

    //////////////////////////////////////////////////////////////////////////
    void Adam::MinimizationOperation::Reset()
    {
        m_MGradients.clear();
        m_VGradients.clear();
        m_Iteration = 0;
        if (m_GlobalStep)
            m_GlobalStep->Output()(0) = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void Adam::MinimizationOperation::ComputeInternal()
    {
        m_InputsManuallyConsumed = true;
        auto vars = Graph::Default()->ComputeGradientsInOrder(m_Order, m_InputNodes, m_NodesAffectingLosses, m_Vars);
        ++m_Iteration;

        if (m_MGradients.size() != vars.size())
        {
            assert(m_MGradients.empty() && m_VGradients.empty());
            m_MGradients.reserve(vars.size());
            m_VGradients.reserve(vars.size());

            for (uint32_t i = 0; i < vars.size(); ++i)
            {
                m_MGradients.push_back(zeros(vars[i]->Output().GetShape()));
                m_MGradients.back().Name(vars[i]->Name() + "/adam_m_grad");
                //m_MGradients.back().SetStorageType(ST_Offloadable);

                m_VGradients.push_back(zeros(vars[i]->Output().GetShape()));
                m_VGradients.back().Name(vars[i]->Name() + "/adam_v_grad");
                //m_VGradients.back().SetStorageType(ST_Offloadable);
            }
        }

        float learningRate = m_LearningRate->Output()(0) * (float)::sqrt(1.0 - ::pow(m_Beta2, m_Iteration)) / (1.0f - (float)::pow(m_Beta1, m_Iteration));

        for (auto i = 0; i < vars.size(); ++i)
        {
            auto& value = vars[i]->Output();
            auto& gradient = vars[i]->OutputGrad();
            /*value.SyncToHost();
            gradient.SyncToHost();*/
            auto& mGrad = m_MGradients[i];
            //mGrad.SyncToHost();
            auto& vGrad = m_VGradients[i];
            //vGrad.SyncToHost();

            Tensor::ActiveOp()->AdamStep(value, gradient, mGrad, vGrad, learningRate, m_Beta1, m_Beta2, m_Epsilon);

            //value.SyncToHost();
            //gradient.SyncToHost();
        }

        if (m_GlobalStep)
            m_GlobalStep->Output()(0) += 1;
    }
}
