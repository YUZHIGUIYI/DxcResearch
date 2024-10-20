//
// Created by ZZK on 2024/10/15.
//

#pragma once

#include <iostream>
#include <format>
#include <string_view>
#include <utility>
#include <vector>
#include <span>
#include <unordered_map>
#include <variant>
#include <array>

#include <wrl/client.h>

#include <d3d11.h>
#include <dxgi.h>
#include <Inc/dxcapi.h>
#include <Inc/d3d12shader.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace toy
{
	// Shader type
	enum class ShaderType
	{
		VertexShader   = 0x1,
		HullShader     = 0x2,
		DomainShader   = 0x4,
		GeometryShader = 0x8,
		PixelShader    = 0x10,
		ComputeShader  = 0x20,
	};

	// Shader model target profile
	enum class ShaderTargetProfile
	{
		ShaderModel_5_0 = 0x40,
		ShaderModel_5_1 = 0x80,
		ShaderModel_6_0 = 0x100,
		ShaderModel_6_1 = 0x200,
		ShaderModel_6_2 = 0x400,
		ShaderModel_6_3 = 0x800,
		ShaderModel_6_4 = 0x1000,
		ShaderModel_6_5 = 0x2000,
		ShaderModel_6_6 = 0x4000
	};

	// Shader input parameter mask
	enum class ShaderInputParaMask
	{
		R = 1,
		RG = 3,
		RGB = 7,
		RGBA = 15
	};

	// Shader input parameter component type
	enum class ShaderInputParaType
	{
		Unknown = 0,
		UInt32 = 16,
		SInt32 = 32,
		Float32 = 64
	};

	// Shader input parameter DXGI format and its size in bytes
	struct DXGIFormatDesc
	{
		DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
		uint32_t size_in_bytes = 0;
	};

	// Dxc compiler result
	struct DxcShaderResult
	{
		ComPtr<IDxcBlob> shader_blob = nullptr;
		ComPtr<ID3D12ShaderReflection> shader_reflection = nullptr;
	};

	// Helper function
	constexpr uint32_t operator|(uint32_t other, ShaderType shader_type);

	constexpr bool operator&(uint32_t other, ShaderType shader_type);

	size_t string_to_id(std::string_view str_view);

	// DXC instance
	struct DxcInStance
	{
	private:
		using DxcCreateInstanceFn = decltype(&::DxcCreateInstance);

		ComPtr<IDxcUtils> utils = nullptr;
		ComPtr<IDxcCompiler3> compiler = nullptr;
		ComPtr<IDxcValidator> validator = nullptr;
		ComPtr<IDxcIncludeHandler> include_handler = nullptr;
		HMODULE compiler_hmodule = nullptr;
		DxcCreateInstanceFn dxc_create_instance_pfn = nullptr;

	private:
		DxcInStance();

	public:
		~DxcInStance();

		DxcInStance(const DxcInStance &) = delete;
		DxcInStance &operator=(const DxcInStance &) = delete;
		DxcInStance(DxcInStance &&) = delete;
		DxcInStance &operator=(DxcInStance &&) = delete;

		static DxcInStance &get();

		DxcShaderResult create_shader_from_file(std::wstring_view shader_filepath, ShaderType shader_type, ShaderTargetProfile shader_target_profile);
	};

	// Constant buffer and its accessor
	struct ConstantBufferAccessor;

	struct ConstantBuffer
	{
	private:
		ComPtr<ID3D11Buffer> constant_buffer = nullptr;
		std::vector<uint8_t> upload_data = {};
		std::string constant_buffer_name = {};
		uint32_t binding_slot = 0;
		uint32_t shader_flag = 0;
		bool is_dirty = false;

		friend struct ConstantBufferAccessor;

	public:
		ConstantBuffer() = default;
		ConstantBuffer(const std::string &cb_name, uint32_t slot, uint32_t size_in_bytes, uint8_t *initial_data = nullptr);

		HRESULT create_buffer(ID3D11Device *device);

		void update_buffer(ID3D11DeviceContext *device_context);

		void transmit_upload_data(ConstantBuffer &other) const;

		void set_shader_flag(ShaderType shader_type);

		void emit_constant_buffer(ID3D11DeviceContext *device_context);

		void bind_vs(ID3D11DeviceContext *device_context);

		void bind_hs(ID3D11DeviceContext *device_context);

		void bind_ds(ID3D11DeviceContext *device_context);

		void bind_gs(ID3D11DeviceContext *device_context);

		void bind_ps(ID3D11DeviceContext *device_context);

		void bind_cs(ID3D11DeviceContext *device_context);
	};

	struct ConstantBufferAccessor
	{
	private:
		ConstantBuffer *constant_buffer_ref;
		std::string component_name = {};
		uint32_t component_offset = 0;
		uint32_t component_size = 0;

	public:
		explicit ConstantBufferAccessor(ConstantBuffer *input_constant_buffer, const std::string &in_component_name, uint32_t in_offset, uint32_t in_size);

		void set_raw(const uint8_t *data, uint32_t offset_in_bytes, uint32_t size_in_bytes);

		void set_matrix_in_bytes(const uint8_t *no_padding_data, uint32_t rows, uint32_t cols);

		void set_sint_matrix(const int32_t *no_padding_data, uint32_t rows, uint32_t cols);

		void set_uint_matrix(const uint32_t *no_padding_data, uint32_t rows, uint32_t cols);

		void set_float_matrix(const float *no_padding_data, uint32_t rows, uint32_t cols);

		void set_sint_vector(std::span<int32_t> data);

		void set_uint_vector(std::span<uint32_t> data);

		void set_float_vector(std::span<float> data);

		void set_sint(int32_t data);

		void set_uint(uint32_t data);

		void set_float(float data);
	};

	// Shader info
	struct VertexShaderInfo
	{
		ComPtr<ID3D11VertexShader> vs = nullptr;
	};

	struct HullShaderInfo
	{
		ComPtr<ID3D11HullShader> hs = nullptr;
	};

	struct DomainShaderInfo
	{
		ComPtr<ID3D11DomainShader> ds = nullptr;
	};

	struct GeometryShaderInfo
	{
		ComPtr<ID3D11GeometryShader> gs = nullptr;
	};

	struct PixelShaderInfo
	{
		ComPtr<ID3D11PixelShader> ps = nullptr;
	};

	struct ThreadGroupConf
	{
		uint32_t thread_group_size_x = 1;
		uint32_t thread_group_size_y = 1;
		uint32_t thread_group_size_z = 1;
	};

	struct ComputeShaderInfo
	{
		ComPtr<ID3D11ComputeShader> cs = nullptr;
		ThreadGroupConf thread_group_conf{};
	};

	// Shader resource view info
	struct ShaderResource
	{
		ID3D11ShaderResourceView *srv = nullptr;
		D3D11_SRV_DIMENSION srv_dimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		uint32_t bind_slot = 0;
		ShaderType shader_flag = ShaderType::VertexShader;
	};

	// Unordered access view info
	struct RWResource
	{
		ID3D11UnorderedAccessView *uav = nullptr;
		D3D11_UAV_DIMENSION uav_dimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uint32_t initial_count = 0;
		uint32_t bind_slot = 0;
		ShaderType shader_flag = ShaderType::VertexShader;
		bool enable_counter = false;
		bool first_init = false;
	};

	// Sampler state
	struct SamplerState
	{
		ID3D11SamplerState *sampler = nullptr;
		uint32_t bind_slot = 0;
		ShaderType shader_flag = ShaderType::VertexShader;
	};

	using ShaderInfo = std::variant<VertexShaderInfo, HullShaderInfo, DomainShaderInfo, GeometryShaderInfo, PixelShaderInfo, ComputeShaderInfo>;

	// Bind shaders to pipeline
	struct EmitShader
	{
		ID3D11DeviceContext *device_context = nullptr;

		void operator()(const VertexShaderInfo &vertex_shader) const;

		void operator()(const HullShaderInfo &hull_shader) const;

		void operator()(const DomainShaderInfo &domain_shader) const;

		void operator()(const GeometryShaderInfo &geometry_shader) const;

		void operator()(const PixelShaderInfo &pixel_shader) const;

		void operator()(const ComputeShaderInfo &compute_shader) const;
	};

	// Pipeline state object
	struct GraphicsPipelineStateObject
	{
		ComPtr<ID3D11RasterizerState> rasterizer_state = nullptr;
		ComPtr<ID3D11DepthStencilState> depth_stencil_state = nullptr;
		ComPtr<ID3D11BlendState> blend_state = nullptr;
		std::wstring_view vs_path{};
		std::wstring_view hs_path{};
		std::wstring_view ds_path{};
		std::wstring_view gs_path{};
		std::wstring_view ps_path{};
		ShaderTargetProfile shader_target_profile = ShaderTargetProfile::ShaderModel_5_1;
	};

	struct ComputePipelineStateObject
	{
		std::wstring_view cs_path{};
		ShaderTargetProfile shader_target_profile = ShaderTargetProfile::ShaderModel_5_1;
	};

	using PipelineStateObject = std::variant<GraphicsPipelineStateObject, ComputePipelineStateObject>;

	// Effect
	struct Effect
	{
	protected:
		std::unordered_map<size_t, std::unique_ptr<ConstantBuffer>> constant_buffer_manager;
		std::unordered_map<size_t, std::unique_ptr<ConstantBufferAccessor>> constant_buffer_accessor_manager;
		std::unordered_map<size_t, ShaderResource> shader_resource_manager;
		std::unordered_map<size_t, RWResource> unordered_access_manager;
		std::unordered_map<size_t, SamplerState> sampler_manager;
		std::vector<ShaderInfo> pipeline_shader_manager;

	public:
		Effect();
		virtual ~Effect();

		ConstantBufferAccessor *query_constant_buffer_accessor(std::string_view variable_name);

		void transmit_constant_buffer(Effect &other, std::string_view constant_buffer_name);

		void bind_shader_resource_view(std::string_view srv_name, ID3D11ShaderResourceView *srv);

		void bind_sampler(std::string_view sampler_name, ID3D11SamplerState *sampler);

		void bind_unordered_access_view(std::string_view uav_name, ID3D11UnorderedAccessView *uav);

		void emit_pipeline(ID3D11DeviceContext *device_context);

		virtual void set_stencil_ref(uint32_t stencil_value) = 0;

		virtual void set_blend_factor(std::span<float> blend_value) = 0;

		virtual void emit_graphics_pipeline(ID3D11DeviceContext *device_context) = 0;

		virtual void emit_compute_pipeline(ID3D11DeviceContext *device_context) = 0;

		virtual void dispatch(ID3D11DeviceContext *device_context, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z) = 0;

	protected:
		void update_shader_reflection(std::wstring_view shader_name, ID3D11Device *device, ID3D12ShaderReflection *shader_reflection);
	};

	struct GraphicsEffect final : Effect
	{
	private:
		ComPtr<ID3D11RasterizerState> rasterizer_state = nullptr;
		ComPtr<ID3D11DepthStencilState> depth_stencil_state = nullptr;
		ComPtr<ID3D11BlendState> blend_state = nullptr;
		ComPtr<ID3D11InputLayout> vertex_input_layout = nullptr;
		std::array<float, 4> blend_factor{ 0.0f, 0.0f, 0.0f, 0.0f };
		uint32_t sample_mask = 0xffffffff;
		uint32_t stencil_ref = 0;

	public:
		explicit GraphicsEffect(const PipelineStateObject &pipeline_state_object, ID3D11Device *device);

		~GraphicsEffect() override = default;
		GraphicsEffect(const GraphicsEffect &) = delete;
		GraphicsEffect &operator=(const GraphicsEffect &) = delete;
		GraphicsEffect(GraphicsEffect &&) = delete;
		GraphicsEffect &operator=(GraphicsEffect &&) = delete;

		void set_stencil_ref(uint32_t stencil_value) override;

		void set_blend_factor(std::span<float> blend_value) override;

		void emit_graphics_pipeline(ID3D11DeviceContext *device_context) override;
	};

	struct ComputeEffect final : Effect
	{
	private:
		ThreadGroupConf thread_group_conf{};

	public:
		explicit ComputeEffect(const PipelineStateObject &pipeline_state_object, ID3D11Device *device);

		~ComputeEffect() override = default;
		ComputeEffect(const ComputeEffect &) = delete;
		ComputeEffect &operator=(const ComputeEffect &) = delete;
		ComputeEffect(ComputeEffect &&) = delete;
		ComputeEffect &operator=(ComputeEffect &&) = delete;

		void emit_compute_pipeline(ID3D11DeviceContext *device_context) override;

		void dispatch(ID3D11DeviceContext *device_context, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z) override;
	};
}














































