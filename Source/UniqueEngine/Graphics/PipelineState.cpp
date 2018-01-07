#include "UniquePCH.h"
#include "PipelineState.h"
#include "Shader.h"

using namespace Diligent;

namespace Unique
{

	PipelineState::PipelineState(Shader& shader, Pass& shaderPass, unsigned defs)
	{
		shaderProgram_ = new ShaderProgram(shaderPass);

		if (shaderPass.shaderStage_[5])
		{
			shaderProgram_->isComputePipeline_ = true;
			SPtr<ShaderVariation> sv = shaderPass.GetShaderVariation(shader, shaderPass.shaderStage_[5], defs);
			shaderProgram_->shaders_.push_back(sv);
		}
		else
		{
			shaderProgram_->isComputePipeline_ = false;

			for (int i = 0; i < 5; i++)
			{
				if (shaderPass.shaderStage_[i])
				{
					SPtr<ShaderVariation> sv = shaderPass.GetShaderVariation(shader, shaderPass.shaderStage_[i], defs);
					shaderProgram_->shaders_.push_back(sv);
				}
			}

		}

		Init();
	}

	PipelineState::PipelineState(ShaderProgram* shaderProgram) : shaderProgram_(shaderProgram)
	{
		Init();
	}

	void PipelineState::Init()
	{
		auto& graphics = GetSubsystem<Graphics>();

		psoDesc_.GraphicsPipeline.DepthStencilDesc = shaderProgram_->shaderPass_.depthState_;
		psoDesc_.GraphicsPipeline.RasterizerDesc = shaderProgram_->shaderPass_.rasterizerState_;
		psoDesc_.GraphicsPipeline.BlendDesc = shaderProgram_->shaderPass_.blendState_;
		psoDesc_.GraphicsPipeline.InputLayout.LayoutElements = shaderProgram_->shaderPass_.inputLayout_.layoutElements_.data();
		psoDesc_.GraphicsPipeline.InputLayout.NumElements = (uint)shaderProgram_->shaderPass_.inputLayout_.layoutElements_.size();

		psoDesc_.IsComputePipeline = shaderProgram_->isComputePipeline_;

		psoDesc_.GraphicsPipeline.NumRenderTargets = 1;
		psoDesc_.GraphicsPipeline.RTVFormats[0] = graphics.IsSRgb() ? TEX_FORMAT_RGBA8_UNORM_SRGB : TEX_FORMAT_RGBA8_UNORM;
		psoDesc_.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
		psoDesc_.GraphicsPipeline.PrimitiveTopologyType = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	bool PipelineState::CreateImpl()
	{
		if (shaderDirty_)
		{
			for (auto& shader : shaderProgram_->shaders_)
			{
				if (!shader->CreateImpl())
				{
					return false;
				}

			}

			shaderDirty_ = false;
		}

		shaderProgram_->shaderVariables_.clear();

		if (shaderProgram_->isComputePipeline_)
		{
			for (auto shader : shaderProgram_->shaders_)
			{
				if (shader->GetShaderType() == SHADER_TYPE_COMPUTE)
				{
					psoDesc_.ComputePipeline.pCS = *shader;
					break;
				}
			}
		}
		else
		{
			for (auto shader : shaderProgram_->shaders_)
			{
				switch (shader->GetShaderType())
				{
				case SHADER_TYPE_VERTEX:
					psoDesc_.GraphicsPipeline.pVS = *shader;
					break;
				case SHADER_TYPE_PIXEL:
					psoDesc_.GraphicsPipeline.pPS = *shader;
					break;
				case SHADER_TYPE_GEOMETRY:
					psoDesc_.GraphicsPipeline.pGS = *shader;
					break;
				case SHADER_TYPE_HULL:
					psoDesc_.GraphicsPipeline.pHS = *shader;
					break;
				case SHADER_TYPE_DOMAIN:
					psoDesc_.GraphicsPipeline.pDS = *shader;
					break;
				default:
					break;
				}

			}
		}

		auto& graphics = GetSubsystem<Graphics>();
		graphics.CreatePipelineState(psoDesc_, *this);
		graphics.BindShaderResources(*this, BIND_SHADER_RESOURCES_ALL_RESOLVED);
		((IPipelineState*)deviceObject_)->CreateShaderResourceBinding(&shaderResourceBinding_);
		dirty_ = false;
		return true;
	}

	IShaderVariable* PipelineState::GetShaderVariable(const StringID& name)
	{
		if (!shaderResourceBinding_)
		{	
			GetPipeline();
		}
	
		if (!shaderResourceBinding_)
		{	
			return nullptr;
		}

		return shaderResourceBinding_->GetVariable(SHADER_TYPE_PIXEL, name.c_str());
	}

	IPipelineState* PipelineState::GetPipeline()
	{
		if (shaderDirty_ || dirty_ || !IsValid())
		{
			if (!CreateImpl())
			{
				return nullptr;
			}
		}

		return (IPipelineState*)deviceObject_;
	}

	void PipelineState::SetDepthStencilState(const DepthStencilStateDesc& dss)
	{
		psoDesc_.GraphicsPipeline.DepthStencilDesc = dss;
		dirty_ = true;
	}

	void PipelineState::SetCullMode(CullMode cull)
	{
		psoDesc_.GraphicsPipeline.RasterizerDesc.CullMode = cull;
	}

	void PipelineState::SetLineAntialiased(bool val)
	{
		psoDesc_.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = val;
	}

	void PipelineState::Reload()
	{
		dirty_ = true;

		for (auto& shd : shaderProgram_->shaders_)
		{
			shd->Reload();
		}

	}


}