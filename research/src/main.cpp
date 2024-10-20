//
// Created by ZZK on 2024/10/14.
//

#include <effect.h>

constexpr std::string_view s_compiler_path = "D:/Dev/CMakeCook/DXC_Research/dxc/bin/x64/dxcompiler.dll";
// constexpr std::wstring_view s_shader_path = L"D:/Dev/CMakeCook/DXC_Research/shaders/transmittance.hlsl";
constexpr std::wstring_view s_shader_path = L"D:/Dev/CMakeCook/DXC_Research/shaders/geometry_vs.hlsl";
constexpr std::wstring_view s_search_path = L"D:/Dev/CMakeCook/DXC_Research/shaders";

int main()
{
	using DxcCreateInstanceFn = decltype(&::DxcCreateInstance);
	const HMODULE compiler_hmodule = LoadLibraryA(s_compiler_path.data());
	auto dxc_create_instance_pfn = reinterpret_cast<DxcCreateInstanceFn>(GetProcAddress(compiler_hmodule, "DxcCreateInstance"));

	static ComPtr<IDxcUtils> utils = nullptr;
	static ComPtr<IDxcCompiler3> compiler = nullptr;
	static ComPtr<IDxcValidator> validator;
	static ComPtr<IDxcIncludeHandler> include_handler = nullptr;

	dxc_create_instance_pfn(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	dxc_create_instance_pfn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	dxc_create_instance_pfn(CLSID_DxcValidator, IID_PPV_ARGS(validator.GetAddressOf()));
	utils->CreateDefaultIncludeHandler(include_handler.GetAddressOf());

	std::vector<const wchar_t *> compilationArguments
	{
		L"-E", L"VS",
		L"-T", L"vs_6_0",
		L"-I", s_search_path.data(),
		DXC_ARG_PACK_MATRIX_ROW_MAJOR,
		DXC_ARG_WARNINGS_ARE_ERRORS,
		DXC_ARG_ALL_RESOURCES_BOUND,
	};
	compilationArguments.push_back(DXC_ARG_DEBUG);

	// Load the shader source file to a blob.
	ComPtr<IDxcBlobEncoding> sourceBlob = nullptr;
	utils->LoadFile(s_shader_path.data(), nullptr, sourceBlob.GetAddressOf());
	DxcBuffer sourceBuffer
	{
		.Ptr = sourceBlob->GetBufferPointer(),
		.Size = sourceBlob->GetBufferSize(),
		.Encoding = 0u,
	};

	// Compile the shader.
	ComPtr<IDxcResult> compiledShaderBuffer = nullptr;
	const HRESULT hr = compiler->Compile(&sourceBuffer,
							compilationArguments.data(),
							static_cast<uint32_t>(compilationArguments.size()),
							include_handler.Get(),
							IID_PPV_ARGS(compiledShaderBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		std::cout << std::format("Failed to compile shader with path\n");
	}

	// Get compilation errors (if any).
	ComPtr<IDxcBlobUtf8> errors = nullptr;
	compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
	if (errors != nullptr && errors->GetStringLength() > 0LLU)
	{
		const char *errorMessage = errors->GetStringPointer();
		std::cout << std::format("{}\n", errorMessage);
	}

	// Get shader reflection data.
	ComPtr<IDxcBlob> reflectionBlob = nullptr;
	compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.GetAddressOf()), nullptr);
	const DxcBuffer reflectionBuffer
	{
		.Ptr = reflectionBlob->GetBufferPointer(),
		.Size = reflectionBlob->GetBufferSize(),
		.Encoding = 0U,
	};

	ComPtr<ID3D12ShaderReflection> shaderReflection = nullptr;
	utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.GetAddressOf()));
	D3D12_SHADER_DESC shaderDesc{};
	shaderReflection->GetDesc(&shaderDesc);

	auto shaderType = static_cast<D3D12_SHADER_VERSION_TYPE>(D3D12_SHVER_GET_TYPE(shaderDesc.Version));

	if (shaderType == D3D12_SHVER_VERTEX_SHADER) {
		for (uint32_t index = 0; index < shaderDesc.InputParameters; ++index) {
			D3D12_SIGNATURE_PARAMETER_DESC signature_parameter_desc{};
			shaderReflection->GetInputParameterDesc(index, &signature_parameter_desc);

			std::cout << std::format("name: {}; index: {}, mask: {}, component type: {}\n",
							signature_parameter_desc.SemanticName, signature_parameter_desc.SemanticIndex, signature_parameter_desc.Mask, static_cast<uint32_t>(signature_parameter_desc.ComponentType));
		}
	}

	for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
		shaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc);

		if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
		{
			ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = shaderReflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
			shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);

			std::cout << std::format("Found CBuffer!\n");

			for (uint32_t j = 0; j < constantBufferDesc.Variables; ++j)
			{
				ID3D12ShaderReflectionVariable* shader_reflection_variable = shaderReflectionConstantBuffer->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC constantBufferVarDesc{};
				shader_reflection_variable->GetDesc(&constantBufferVarDesc);
				std::cout << std::format("Constant buffer variable: {}\n", j);
			}
		} else if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
		{
			std::cout << std::format("Found Texture!\n");
		}
	}


	std::cout << std::format("Finished.\n");
}