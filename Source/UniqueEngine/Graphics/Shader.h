#pragma once
#include "Resource/Resource.h"
#include "GraphicsDefs.h"
#include "ShaderVariation.h"
#include <DepthStencilState.h>
#include <BlendState.h>
#include <RasterizerState.h>

namespace Unique
{
	class Shader;

	using BlendFactor = BLEND_FACTOR;
	using BlendOperation = BLEND_OPERATION;

	class Pass
	{
	public:
		Pass(const String& name = "");
		~Pass();

		uint GetMask(Shader* shader, const String& defs);

		PipelineState* GetInstance(Shader* shader, const String & defs);

		PipelineState* GetInstance(Shader* shader, unsigned defMask);

		bool Prepare();

		StringID				name_;
		BlendStateDesc			blendState_;
		DepthStencilStateDesc	depthState_;
		RasterizerStateDesc		rasterizerState_;
		Vector<LayoutElement>	inputLayout_;
		ShaderStage				shaderStage_[6];
	private:
		
		Vector<String>			allDefs_;
		uint					allMask_ = 0;
		HashMap<uint, SPtr<PipelineState>> cachedPass_;

		friend class Shader;
		friend class ShaderVariation;
		friend class PipelineState;
	};

	class Shader : public Resource
	{
		uRTTI(Shader, Resource)
	public:
		Shader();
		~Shader();

		virtual bool Prepare();
		
		virtual bool Create();

		Pass* AddPass(const String& name);
		Pass* GetShaderPass(const StringID & pass);

		uint GetMask(const StringID& passName, const String & defs);

		PipelineState* GetPipeline(const StringID& passName, uint defMask);

		PipelineState* GetPipeline(const StringID& passName, const String & defs);

		static Vector<String> SplitDef(const String& defs);

		static String GetShaderPath(DeviceType renderID);
	private:
		String shaderName_;
		Vector<Pass> passes_;
	};

}