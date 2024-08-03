#pragma once
#include <memory>
#include <string>
#include <glslang/Public/ShaderLang.h>
#include "vulkan/vulkan_core.h"

enum ECompilerType
{
    HLSL,
    GLSL,
};


class FShader
{
public:
    FShader();
    virtual std::string GetSource();
    virtual EShLanguage GetShaderType() const;
    virtual ECompilerType GetCompilerType() const;
    bool IsCompiled() const;
    bool CreateModule(std::vector<uint32_t> SPIRV);

protected:
    VkShaderModule ShaderModule = VK_NULL_HANDLE;
};

class FDefaultVertexShader : public FShader
{
public:
    virtual ECompilerType GetCompilerType() const override { return GLSL; }
    virtual std::string GetSource() override { return "/GLSL/Defaults/DefaultVertex.vert"; }
    virtual EShLanguage GetShaderType() const override { return EShLangVertex; }
};

class FDefaultPixelShader : public FShader
{
public:
    virtual ECompilerType GetCompilerType() const override { return GLSL; }
    virtual std::string GetSource() override { return "/GLSL/Defaults/DefaultPixel.frag"; }
    virtual EShLanguage GetShaderType() const override { return EShLangFragment; }
};


class FShaderCompiler
{
public:
    FShaderCompiler();
    ~FShaderCompiler();
    static FShaderCompiler* Get();

    void CompileShaders();
    template<typename Shader>
    void AddShader();
    
private:
    void CompileGLSL(std::unique_ptr<FShader>& Shader);
    void CompileHLSL(std::unique_ptr<FShader>& Shader);
    
private:
    std::vector<std::unique_ptr<FShader>> GlobalShaders;
};

template <typename Shader>
void FShaderCompiler::AddShader()
{
    GlobalShaders.push_back(std::make_unique<Shader>());
}
