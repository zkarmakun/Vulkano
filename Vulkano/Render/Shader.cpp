#include "Shader.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "Core/Assertion.h"
#include "Core/Paths.h"
#include <dxcapi.h>
#include <wrl.h>

#include "VulkanInterface.h"

using Microsoft::WRL::ComPtr;

FShader::FShader()
{
}

FShader::~FShader()
{
	Release();
}

void FShader::SetShaderIntrinsics(ECompilerType InCompilerType, const std::string& Source, const std::string& Entry,
	EShLanguage InShaderType)
{
	CompilerType = InCompilerType;
	SourcePath = Source;
	EntryPoint = Entry;
	ShaderType = InShaderType;
}

std::string FShader::GetSource() const
{
	return SourcePath;
}

EShLanguage FShader::GetShaderType() const
{
	return ShaderType;
}

ECompilerType FShader::GetCompilerType() const
{
	return CompilerType;
}

const std::string& FShader::GetEntryPoint() const
{
	return EntryPoint;
}

bool FShader::IsCompiled() const
{
	return ShaderModule != VK_NULL_HANDLE;
}

bool FShader::CreateModule(std::vector<uint32_t> SPIRV)
{
	if(IsCompiled())
	{
		return true;
	}
	
	if(SPIRV.empty())
	{
		VK_LOG(LOG_WARNING, "FShader::CreateModule Fail creating shader module SPIRV code is empty: %s", GetSource().c_str());
		return false;
	}
	
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = SPIRV.size() * sizeof(uint32_t);
	shaderModuleCreateInfo.pCode = SPIRV.data();
	if(vkCreateShaderModule(FVulkan::GetDevice(), &shaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
	{
		VK_LOG(LOG_WARNING, "FShader::CreateModule Fail creating shader module: %s", GetSource().c_str());
		return false;
	}
	return true;
}

void FShader::Release()
{
	if(IsCompiled())
	{
		vkDestroyShaderModule(FVulkan::GetDevice(), ShaderModule, nullptr);
		ShaderModule = VK_NULL_HANDLE;
	}
}

VkShaderModule& FShader::GetShader()
{
	return ShaderModule;
}

FShaderCompiler::FShaderCompiler()
{
    glslang::InitializeProcess();
}

FShaderCompiler::~FShaderCompiler()
{
    glslang::FinalizeProcess();
}

FShaderCompiler* FShaderCompiler::Get()
{
	static FShaderCompiler* Instance;
	if(!Instance)
	{
		Instance = new FShaderCompiler();
	}
	return Instance;
}

void FShaderCompiler::CompileShaders()
{
	VK_LOG(LOG_INFO, "Compiling shaders: %i", GlobalShaders.size());
	for(const std::pair<const std::type_index, std::shared_ptr<FShader>>& Elem : GlobalShaders)
	{
		std::shared_ptr<FShader> Shader = Elem.second;
		if(!Shader->IsCompiled())
		{
			Compile(Shader);
		}
	}
}

void FShaderCompiler::CleanUpShaders() const
{
	VK_LOG(LOG_INFO, "Cleaning global shaders: %i", GlobalShaders.size());
	for(const std::pair<const std::type_index, std::shared_ptr<FShader>>& Elem : GlobalShaders)
	{
		std::shared_ptr<FShader> Shader = Elem.second;
		if(Shader->IsCompiled())
		{
			Shader->Release();
		}
	}
}

void InitResources(TBuiltInResource& Resources)
{
    Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;
		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

void FShaderCompiler::Compile(std::shared_ptr<FShader>& Shader)
{
	std::string ShaderDirectory = FPaths::GetShaderDirectory();
	std::string FilePath = FPaths::GetShaderDirectory() + Shader->GetSource();
	
	if(!FPaths::FileExists(FilePath))
	{
		fatal("File shader path does not exist %s", FilePath.c_str());
	}

	std::string SourceCode = FPaths::LoadFileToString(FilePath);
	const char* shaderStrings = SourceCode.c_str();
	glslang::EShSource SourceType = Shader->GetCompilerType() == ECompilerType::GLSL ? glslang::EShSourceGlsl : glslang::EShSourceHlsl;

	glslang::EShTargetClientVersion Version;
	switch (FVulkan::GetMinorVersion())
	{
	case 1:
		Version = glslang::EShTargetVulkan_1_1;
		break;
	case 2:
		Version = glslang::EShTargetVulkan_1_2;
		break;
	case 3:
		Version = glslang::EShTargetVulkan_1_3;
		break;
	default:
		Version = glslang::EShTargetVulkan_1_3;
	}
	
	glslang::TShader shader(Shader->GetShaderType());
	shader.setEnvClient(glslang::EShClient::EShClientVulkan, Version);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
	shader.setStrings(&shaderStrings, 1);
	shader.setEntryPoint(Shader->GetEntryPoint().c_str());
	shader.setEnvInput(SourceType, Shader->GetShaderType(), glslang::EShClientVulkan, 460);

	TBuiltInResource Resources = {};
	InitResources(Resources);
	if (!shader.parse(&Resources , 460, true, EShMsgDefault))
	{
		std::string Message = "Shader compile error:\n";
		Message += shader.getInfoLog();
		int result = MessageBoxA(NULL, Message.c_str(), "Shader compilation retry", MB_ICONQUESTION | MB_YESNO);
		if( result == IDYES)
		{
			Compile(Shader);
		}
		else
		{
			fatal("Failing compiling shader, no retry was selected, terminating program");
		}
		return;
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault))
	{
		VK_LOG(LOG_ERROR, "Shader link error: %s\n", program.getInfoLog());
		return;
	}

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*program.getIntermediate(Shader->GetShaderType()), spirv);
	if(Shader->CreateModule(spirv))
	{
		VK_LOG(LOG_SUCCESS, "Compiled shader: %s", FPaths::GetFileName(FilePath).c_str());
	}
}
