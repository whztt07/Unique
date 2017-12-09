#include "Precompiled.h"
#include "Shader.h"
#include "Graphics/Graphics.h"

namespace Unique
{	
	uClassTraits
	(
		StencilOpDesc, 
		"StencilFailOp", self.StencilFailOp, 
		"StencilDepthFailOp", self.StencilDepthFailOp, 
		"StencilPassOp", self.StencilPassOp,
		"StencilFunc", self.StencilFunc
	)
	
	uClassTraits
	(
		DepthStencilStateDesc, 
		"DepthEnable", self.DepthEnable, 
		"DepthWriteEnable", self.DepthWriteEnable, 
		"ComparisonFunction", self.DepthFunc,
		"StencilEnable", self.StencilEnable, 
		"StencilReadMask", self.StencilReadMask,
		"StencilWriteMask", self.StencilWriteMask,
		"FrontFace", self.FrontFace,
		"BackFace", self.BackFace
	)

	uClassTraits
	(
		RasterizerStateDesc, 
		"FillMode", self.FillMode, 
		"CullMode", self.CullMode, 
		"FrontCounterClockwise", self.FrontCounterClockwise,
		"DepthBias", self.DepthBias, 
		"DepthBiasClamp", self.DepthBiasClamp,
		"SlopeScaledDepthBias", self.SlopeScaledDepthBias,
		"DepthClipEnable", self.DepthClipEnable,
		"ScissorEnable", self.ScissorEnable,
		"AntialiasedLineEnable", self.AntialiasedLineEnable
	)

	uClassTraits
	(
		RenderTargetBlendDesc, 
		"BlendEnable", self.BlendEnable, 
		"LogicOperationEnable", self.LogicOperationEnable, 
		"SrcBlend", self.SrcBlend,
		"DestBlend", self.DestBlend, 
		"BlendOp", self.BlendOp,
		"SrcBlendAlpha", self.SrcBlendAlpha,
		"DestBlendAlpha", self.DestBlendAlpha,
		"BlendOpAlpha", self.BlendOpAlpha,
		"LogicOp", self.LogicOp,
		"RenderTargetWriteMask", self.RenderTargetWriteMask
	)
		
	uClassTraits
	(
		BlendStateDesc, 
		"AlphaToCoverageEnable", self.AlphaToCoverageEnable, 
		"IndependentBlendEnable", self.IndependentBlendEnable,
		"RenderTargets[0]", self.RenderTargets[0],
		"RenderTargets[1]", self.RenderTargets[1],
		"RenderTargets[2]", self.RenderTargets[2],
		"RenderTargets[3]", self.RenderTargets[3],
		"RenderTargets[4]", self.RenderTargets[4],
		"RenderTargets[5]", self.RenderTargets[5],
		"RenderTargets[6]", self.RenderTargets[6],
		"RenderTargets[7]", self.RenderTargets[7]
	)
	
	uEnumTraits(LayoutElement::FREQUENCY, "UNDEFINED", "PER_VERTEX", "PER_INSTANCE")
		
	uClassTraits
	(
		LayoutElement, 
		"InputIndex", self.InputIndex, 
		"BufferSlot", self.BufferSlot,
		"NumComponents", self.NumComponents,
		"ValueType", self.ValueType,
		"IsNormalized", self.IsNormalized,
		"RelativeOffset", self.RelativeOffset,
		"Frequency", self.Frequency,
		"InstanceDataStepRate", self.InstanceDataStepRate
	)

	uClassTraits
	(
		Pass,
		"Name", self.name_,
		"DepthState", self.depthState_,
		"RasterizerState", self.rasterizerState_,
		"BlendStateDesc", self.blendState_,
		"InputLayout", self.inputLayout_,
		//"ShaderDefines", self.allDefs_,
		"VertexShader", self.shaderStage_[0],
		"PixelShader", self.shaderStage_[1],
		"GeometryShader", self.shaderStage_[2],
		"HullShader", self.shaderStage_[3],
		"DomainShader", self.shaderStage_[4],
		"ComputeShader", self.shaderStage_[5]
	)

	uObject(Shader)
	{
		uFactory("Graphics")
		uAttribute("Name", shaderName_)
		uAttribute("Pass", passes_)	
	}

	Pass::Pass(const String& name) : name_(name)
	{
		for (int i = 0; i < 6; i++)
		{
			shaderStage_[i].shaderType_ = (ShaderType)(i + 1);
		}
	}

	Pass::~Pass()
	{
	}

	uint Pass::GetMask(Shader* shader, const String& defs)
	{
		unsigned mask = 0;
		for (uint i = 0; i < allDefs_.size(); i++)
		{
			if (defs.Find(allDefs_[i]) != String::NPOS)
			{
				mask |= (unsigned)(1 << i);
			}
		}

		return mask;
	}

	PipelineState* Pass::GetInstance(Shader* shader, const String& defs)
	{
		unsigned defMask = GetMask(shader, defs);

		return GetInstance(shader, defMask);
	}

	PipelineState* Pass::GetInstance(Shader* shader, unsigned defMask)
	{
		defMask &= allMask_;

		auto it = cachedPass_.find(defMask);;
		if (it != cachedPass_.end())
		{
			return it->second;
		}

		SPtr<PipelineState> inst(new PipelineState(*shader, *this, defMask));
		cachedPass_[defMask] = inst;
		return inst;
		
	}

	bool Pass::Prepare()
	{
		auto& computeShader = shaderStage_[5];
		if (computeShader)
		{
			allDefs_ = Shader::SplitDef(computeShader.defines_);

			if (allDefs_.size() > 0)
			{
				std::sort(allDefs_.begin(), allDefs_.end());
				allMask_ = (unsigned)(1 << (allDefs_.size() + 1)) - 1;
				computeShader.mask_ = allMask_;
			}
		}
		else
		{
			auto& vertexShader = shaderStage_[0];
			auto& pixelShader = shaderStage_[1];
			Vector<String> psDefs = Shader::SplitDef(pixelShader.defines_);
			Vector<String> vsDefs = Shader::SplitDef(vertexShader.defines_);
			allDefs_ = psDefs;

			for (auto& s : vsDefs)
			{
				if (!Contains(allDefs_, s))
				{
					allDefs_.push_back(s);
				}
			}
			
			if (allDefs_.size() > 0)
			{
				std::sort(allDefs_.begin(), allDefs_.end());
				allMask_ = (unsigned)(1 << (allDefs_.size() + 1)) - 1;

				for (uint i = 0; i < allDefs_.size(); i++)
				{
					if (Find(vsDefs, allDefs_[i]) != vsDefs.end())
					{
						pixelShader.mask_ |= (unsigned)(1 << i);
					}

					if (Find(psDefs, allDefs_[i]) != psDefs.end())
					{
						pixelShader.mask_ |= (unsigned)(1 << i);
					}
				}
			}

		}

		return true;
	}

	Shader::Shader()
	{
		passes_.reserve(16);
	}

	Shader::~Shader()
	{
	}

	bool Shader::Prepare()
	{
		for (auto& pass : passes_)
		{
			pass.Prepare();
		}

		return true;
	}

	bool Shader::Create()
	{
		return true;
	}

	Pass* Shader::AddPass(const String& name)
	{
		passes_.emplace_back(name);
		return &(passes_.back());
	}

	Pass* Shader::GetShaderPass(const StringID & passName)
	{
		for (auto& p : passes_)
		{
			if (p.name_ == passName)
			{
				return &p;
			}
		}

		return nullptr;
	}

	uint Shader::GetMask(const StringID & passName, const String& defs)
	{
		Pass* pass = GetShaderPass(passName);
		if (pass == nullptr)
		{
			return 0;
		}

		return pass->GetMask(this, defs);
	}

	PipelineState* Shader::GetPipeline(const StringID& passName, uint defMask)
	{
		Pass* pass = GetShaderPass(passName);
		if (pass == nullptr)
		{
			return nullptr;
		}

		return pass->GetInstance(this, defMask);
	}

	PipelineState* Shader::GetPipeline(const StringID& passName, const String& defs)
	{
		Pass* pass = GetShaderPass(passName);
		if (pass == nullptr)
		{
			return nullptr;
		}

		return pass->GetInstance(this, defs);
	}

	Vector<String> Shader::SplitDef(const String& defs)
	{
		if (defs.Empty())
		{
			return Vector<String>();
		}

		return defs.Split(' ');
	}

	String Shader::GetShaderPath(DeviceType renderID)
	{
		switch (renderID)
		{
		case DeviceType::D3D11:
		case DeviceType::D3D12:
			return "Shaders/HLSL/";
			
		case DeviceType::OpenGL:
			return "Shaders/GLSL/";
		case DeviceType::OpenGLES:
			return "Shaders/ESSL/";

		//case DeviceType::Vulkan:
		//	return "Shaders/VKSL/";
		//case DeviceType::Metal:
		//	return "Shaders/metal/";
		default:
			break;
		}

		return "";
	}



}