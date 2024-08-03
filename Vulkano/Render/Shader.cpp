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

std::string FShader::GetSource()
{
	return "";
}

EShLanguage FShader::GetShaderType() const
{
	return EShLangCompute;
}

ECompilerType FShader::GetCompilerType() const
{
	return GLSL;
}

bool FShader::IsCompiled() const
{
	return ShaderModule != VK_NULL_HANDLE;
}

bool FShader::CreateModule(std::vector<uint32_t> SPIRV)
{
	if(SPIRV.empty())
	{
		return false;
	}
	
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = SPIRV.size();
	shaderModuleCreateInfo.pCode = SPIRV.data();
	if(vkCreateShaderModule(FVulkan::GetDevice(), &shaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
	{
		VK_LOG(LOG_WARNING, "FShader::CreateModule Fail creating shader module: %s", GetSource().c_str());
		return false;
	}
	return true;
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
	for(auto& Shader : GlobalShaders)
	{
		if(!Shader->IsCompiled())
		{
			if(Shader->GetCompilerType() == HLSL)
			{
				CompileHLSL(Shader);
			}
			else
			{
				CompileGLSL(Shader);
			}
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

void FShaderCompiler::CompileGLSL(std::unique_ptr<FShader>& Shader)
{
	std::string ShaderDirectory = FPaths::GetShaderDirectory();
	std::string FilePath = FPaths::GetShaderDirectory() + Shader->GetSource();
	
	if(!FPaths::FileExists(FilePath))
	{
		VK_LOG(LOG_WARNING, "File shader path does not exist %s", FilePath.c_str());
		return;
	}

	std::string SourceCode = FPaths::LoadFileToString(FilePath);
	const char* shaderStrings[1];
	shaderStrings[0] = SourceCode.c_str();
	
	glslang::TShader shader(Shader->GetShaderType());
	shader.setStrings(shaderStrings, 1);

	TBuiltInResource Resources = {};
	InitResources(Resources);
	if (!shader.parse(&Resources , 450, true, EShMsgDefault))
	{
		std::string Message = "Shader compile error:\n";
		Message += shader.getInfoLog();
		int result = MessageBoxA(NULL, Message.c_str(), "Shader compilation retry", MB_ICONQUESTION | MB_YESNO);
		if( result == IDYES)
		{
			CompileGLSL(Shader);
		}
		else
		{
			checkf(0, "Failing compiling shader, no retry was selected, terminating program");
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

void FShaderCompiler::CompileHLSL(std::unique_ptr<FShader>& Shader)
{
	std::string ShaderDirectory = FPaths::GetShaderDirectory();
	std::string FilePath = FPaths::GetShaderDirectory() + Shader->GetSource() + ".hlsl";
	
	if(!FPaths::FileExists(FilePath))
	{
		VK_LOG(LOG_WARNING, "File shader path does not exist %s", FilePath.c_str());
		return;
	}

	std::string SourceCode = FPaths::LoadFileToString(FilePath);

	auto StringConv([](const std::string& str)
	{
		std::wstring wstr(str.begin(), str.end());
		return wstr;
	});

	auto GetTarget([](EShLanguage Type)
	{
		switch (Type)
		{
		case EShLangVertex: return "vs_6_0";
		case EShLangFragment: return "ps_6_0";
		case EShLangCompute: return "vs_6_0";
		}
		return "";
	});

	std::wstring wSource = StringConv(SourceCode);
	std::wstring wTargetProfile = StringConv(GetTarget(Shader->GetShaderType()));
	
	ComPtr<IDxcCompiler> compiler;
	ComPtr<IDxcLibrary> library;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ComPtr<IDxcOperationResult> result;

	// Initialize DXC
	DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)compiler.GetAddressOf());
	DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)library.GetAddressOf());

	// Create blob from the shader source
	library->CreateBlobFromFile(wSource.c_str(), nullptr, &sourceBlob);

	// Compile the shader
	std::vector<LPCWSTR> arguments = {
		wSource.c_str(),
		L"-T", wTargetProfile.c_str(),
		L"-spirv",
		L"-fvk-use-dx-layout",
	};

	compiler->Compile(
		sourceBlob.Get(),
		wSource.c_str(),
		L"main",
		wTargetProfile.c_str(),
		arguments.data(),
		(UINT)arguments.size(),
		nullptr,
		0,
		nullptr,
		&result
	);

	// Check for compilation errors
	HRESULT hr;
	result->GetStatus(&hr);
	if (FAILED(hr)) {
		ComPtr<IDxcBlobEncoding> errors;
		result->GetErrorBuffer(&errors);
		std::wstring Error = (const wchar_t*)errors->GetBufferPointer();
		int result2 = MessageBox(NULL, Error.c_str(), L"Shader compilation retry", MB_ICONQUESTION | MB_YESNO);
		if( result2 == IDYES)
		{
			CompileHLSL(Shader);
		}
		else
		{
			checkf(0, "Failing compiling shader, no retry was selected, terminating program");
		}
		return;
	}

	// Get the compiled SPIR-V blob
	ComPtr<IDxcBlob> shader;
	result->GetResult(&shader);

	std::vector<uint32_t> spirv(shader->GetBufferSize() / 4);
	memcpy(spirv.data(), shader->GetBufferPointer(), shader->GetBufferSize());
	if(Shader->CreateModule(spirv))
	{
		VK_LOG(LOG_SUCCESS, "Compiled shader: %s", FPaths::GetFileName(FilePath).c_str());
	}
}
