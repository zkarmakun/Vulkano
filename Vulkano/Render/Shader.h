#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
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
    ~FShader();
    void SetShaderIntrinsics(ECompilerType InCompilerType, const std::string& Source, const std::string& Entry, EShLanguage InShaderType);
    
    virtual std::string GetSource() const;
    EShLanguage GetShaderType() const;
    ECompilerType GetCompilerType() const;
    const std::string& GetEntryPoint() const;
    bool IsCompiled() const;
    bool CreateModule(std::vector<uint32_t> SPIRV);
    void Release();
    VkShaderModule& GetShader();

protected:
    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    std::string EntryPoint = "main";
    std::string SourcePath;
    ECompilerType CompilerType = GLSL;
    EShLanguage ShaderType = EShLangVertex;
};

class FDefaultVertexShader : public FShader
{
public:
    /*virtual ECompilerType GetCompilerType() const override { return HLSL; }
    virtual std::string GetSource() override { return "/HLSL/Defaults/DefaultVertex.hlsl"; }
    virtual EShLanguage GetShaderType() const override { return EShLangVertex; }*/
};

class FDefaultPixelShader : public FShader
{
public:
    /*virtual ECompilerType GetCompilerType() const override { return HLSL; }
    virtual std::string GetSource() override { return "/HLSL/Defaults/DefaultPixel.hlsl"; }
    virtual EShLanguage GetShaderType() const override { return EShLangFragment; }*/
};


class FShaderCompiler
{
public:
    FShaderCompiler();
    ~FShaderCompiler();
    static FShaderCompiler* Get();

    void CompileShaders();
    void CleanUpShaders() const;
    template<typename Shader>
    void AddShader(ECompilerType CompilerType, const std::string& Source, const std::string& Entry, EShLanguage ShaderType);

    template <typename Shader>
    std::shared_ptr<Shader> FindShader();
private:
    void Compile(std::shared_ptr<FShader>& Shader);
    
private:
    std::unordered_map<std::type_index, std::shared_ptr<FShader>> GlobalShaders;
};

template <typename Shader>
void FShaderCompiler::AddShader(ECompilerType CompilerType, const std::string& Source, const std::string& Entry, EShLanguage ShaderType)
{
    GlobalShaders[typeid(Shader)] = std::make_shared<Shader>();
    GlobalShaders[typeid(Shader)]->SetShaderIntrinsics(CompilerType, Source, Entry, ShaderType);
}

template <typename Shader>
std::shared_ptr<Shader> FShaderCompiler::FindShader()
{
    auto it = GlobalShaders.find(typeid(Shader));
    if (it != GlobalShaders.end()) {
        return std::dynamic_pointer_cast<Shader>(it->second);
    }
    return nullptr;
}