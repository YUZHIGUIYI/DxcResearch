//
// Created by ZZK on 2024/10/15.
//

#include <effect.h>
#include <cassert>

namespace toy
{
	constexpr uint32_t operator|(uint32_t other, ShaderType shader_type)
	{
		return (other | static_cast<uint32_t>(shader_type));
	}

	constexpr bool operator&(uint32_t other, ShaderType shader_type)
	{
		return (other & static_cast<uint32_t>(shader_type)) > 0;
	}

	constexpr uint32_t operator|(ShaderType shader_type, ShaderTargetProfile shader_target_profile)
	{
		return static_cast<uint32_t>(shader_type) | static_cast<uint32_t>(shader_target_profile);
	}

	// Hash function
	size_t string_to_id(std::string_view str_view)
	{
		static std::hash<std::string_view> hash;
		return hash(str_view);
	}

	// Convert dxc shader type to user defined shader type
	static ShaderType convert_to_internal_shader_type(D3D12_SHADER_VERSION_TYPE shader_version_type)
	{
		switch (shader_version_type)
		{
			case D3D12_SHVER_VERTEX_SHADER   : return ShaderType::VertexShader;
			case D3D12_SHVER_HULL_SHADER     : return ShaderType::HullShader;
			case D3D12_SHVER_DOMAIN_SHADER   : return ShaderType::DomainShader;
			case D3D12_SHVER_GEOMETRY_SHADER : return ShaderType::GeometryShader;
			case D3D12_SHVER_PIXEL_SHADER    : return ShaderType::PixelShader;
			case D3D12_SHVER_COMPUTE_SHADER  : return ShaderType::ComputeShader;
			default : assert(false && "Unsupported shader");
		}
	}

	// Get shader entry point via shader type
	std::wstring_view query_shader_entry_point(ShaderType shader_type)
	{
		switch (shader_type)
		{
			case ShaderType::VertexShader  : return L"VS";
			case ShaderType::HullShader    : return L"HS";
			case ShaderType::DomainShader  : return L"DS";
			case ShaderType::GeometryShader: return L"GS";
			case ShaderType::PixelShader   : return L"PS";
			case ShaderType::ComputeShader : return L"CS";
			default: assert(false && "Unsupported shader type");
		}
	}

	// shader target profile
	std::unordered_map<uint32_t, std::wstring_view> s_shader_target_profile_mapping {
		{ ShaderType::VertexShader | ShaderTargetProfile::ShaderModel_5_0, L"vs_5_0" },
		{ ShaderType::HullShader | ShaderTargetProfile::ShaderModel_5_0, L"hs_5_0" },
		{ ShaderType::DomainShader | ShaderTargetProfile::ShaderModel_5_0, L"ds_5_0" },
		{ ShaderType::GeometryShader | ShaderTargetProfile::ShaderModel_5_0, L"gs_5_0" },
		{ ShaderType::PixelShader | ShaderTargetProfile::ShaderModel_5_0, L"ps_5_0" },
		{ ShaderType::ComputeShader | ShaderTargetProfile::ShaderModel_5_0, L"cs_5_0" },

		{ ShaderType::VertexShader | ShaderTargetProfile::ShaderModel_5_1, L"vs_5_1" },
		{ ShaderType::HullShader | ShaderTargetProfile::ShaderModel_5_1, L"hs_5_1" },
		{ ShaderType::DomainShader | ShaderTargetProfile::ShaderModel_5_1, L"ds_5_1" },
		{ ShaderType::GeometryShader | ShaderTargetProfile::ShaderModel_5_1, L"gs_5_1" },
		{ ShaderType::PixelShader | ShaderTargetProfile::ShaderModel_5_1, L"ps_5_1" },
		{ ShaderType::ComputeShader | ShaderTargetProfile::ShaderModel_5_1, L"cs_5_1" },

		{ ShaderType::VertexShader | ShaderTargetProfile::ShaderModel_6_0, L"vs_6_0" },
		{ ShaderType::HullShader | ShaderTargetProfile::ShaderModel_6_0, L"hs_6_0" },
		{ ShaderType::DomainShader | ShaderTargetProfile::ShaderModel_6_0, L"ds_6_0" },
		{ ShaderType::GeometryShader | ShaderTargetProfile::ShaderModel_6_0, L"gs_6_0" },
		{ ShaderType::PixelShader | ShaderTargetProfile::ShaderModel_6_0, L"ps_6_0" },
		{ ShaderType::ComputeShader | ShaderTargetProfile::ShaderModel_6_0, L"cs_6_0" },
	};

	static std::wstring_view query_shader_target_profile(ShaderType shader_type, ShaderTargetProfile shader_target_profile)
	{
		const auto profile_key = shader_type | shader_target_profile;
		return s_shader_target_profile_mapping[profile_key];
	}

	// Dxc instance
	constexpr std::string_view s_compiler_path = "D:/Dev/CMakeCook/DXC_Research/dxc/bin/x64/dxcompiler.dll";
	constexpr std::wstring_view s_search_path = L"D:/Dev/CMakeCook/DXC_Research/shaders";
	DxcInStance::DxcInStance()
	{
		compiler_hmodule = LoadLibraryA(s_compiler_path.data());
		dxc_create_instance_pfn = reinterpret_cast<DxcCreateInstanceFn>(GetProcAddress(compiler_hmodule, "DxcCreateInstance"));
		dxc_create_instance_pfn(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		dxc_create_instance_pfn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		dxc_create_instance_pfn(CLSID_DxcValidator, IID_PPV_ARGS(validator.GetAddressOf()));
		utils->CreateDefaultIncludeHandler(include_handler.GetAddressOf());
	}

	DxcInStance::~DxcInStance()
	{
		if (compiler_hmodule)
		{
			FreeLibrary(compiler_hmodule);
			dxc_create_instance_pfn = nullptr;
		}
	}

	DxcInStance &DxcInStance::get()
	{
		static DxcInStance dxc_instance{};
		return dxc_instance;
	}

	DxcShaderResult DxcInStance::create_shader_from_file(std::wstring_view shader_filepath, ShaderType shader_type, ShaderTargetProfile shader_target_profile)
	{
		DxcShaderResult shader_result{};
		auto entry_point = query_shader_entry_point(shader_type);
		auto target_profile = query_shader_target_profile(shader_type, shader_target_profile);
		std::vector<const wchar_t *> compilation_arguments{
			L"-E", entry_point.data(),
			L"-T", target_profile.data(),
			L"-I", s_search_path.data(),
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			DXC_ARG_WARNINGS_ARE_ERRORS,
			DXC_ARG_ALL_RESOURCES_BOUND,
		};
#if defined(_DEBUG)
		compilation_arguments.push_back(DXC_ARG_DEBUG);
#else
		compilation_arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL1);
#endif

		// Load the shader source file to a blob.
		ComPtr<IDxcBlobEncoding> source_blob = nullptr;
		utils->LoadFile(shader_filepath.data(), nullptr, source_blob.GetAddressOf());
		const DxcBuffer source_buffer{
			.Ptr = source_blob->GetBufferPointer(),
			.Size = source_blob->GetBufferSize(),
			.Encoding = 0U,
		};

		// Compile shader
		ComPtr<IDxcResult> compiled_shader_buffer = nullptr;
		const HRESULT hr = compiler->Compile(&source_buffer,
								compilation_arguments.data(),
								static_cast<uint32_t>(compilation_arguments.size()),
								include_handler.Get(),
								IID_PPV_ARGS(compiled_shader_buffer.GetAddressOf()));
		if (FAILED(hr))
		{
			std::cout << std::format("Failed to compile shader with path\n");
		}

		// Get compilation errors (if any).
		ComPtr<IDxcBlobUtf8> errors = nullptr;
		compiled_shader_buffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
		if (errors != nullptr && errors->GetStringLength() > 0LLU)
		{
			const char *errorMessage = errors->GetStringPointer();
			std::cout << std::format("{}\n", errorMessage);
		}

		// Get shader blob
		compiled_shader_buffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shader_result.shader_blob.GetAddressOf()), nullptr);
		if (shader_result.shader_blob == nullptr)
		{
			std::cout << std::format("Failed to get shader blob\n");
		}

		// Get shader reflection data.
		ComPtr<IDxcBlob> reflection_blob = nullptr;
		compiled_shader_buffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflection_blob.GetAddressOf()), nullptr);
		const DxcBuffer reflection_buffer{
			.Ptr = reflection_blob->GetBufferPointer(),
			.Size = reflection_blob->GetBufferSize(),
			.Encoding = 0U,
		};
		utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(shader_result.shader_reflection.GetAddressOf()));
		if (shader_result.shader_reflection == nullptr)
		{
			std::cout << std::format("Failed to get shader reflection");
		}

		return shader_result;
	}

	// Constant buffer
	ConstantBuffer::ConstantBuffer(const std::string &cb_name, uint32_t slot, uint32_t size_in_bytes, uint8_t *initial_data)
	: constant_buffer(nullptr), upload_data(size_in_bytes), constant_buffer_name(cb_name), binding_slot(slot), is_dirty(false)
	{
		if (initial_data != nullptr)
		{
			std::memcpy(upload_data.data(), initial_data, size_in_bytes);
		}
	}

	HRESULT ConstantBuffer::create_buffer(ID3D11Device *device)
	{
		if (device != nullptr)
		{
			D3D11_BUFFER_DESC constant_buffer_desc{};
			constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
			constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			constant_buffer_desc.ByteWidth = static_cast<uint32_t>(upload_data.size());
			return device->CreateBuffer(&constant_buffer_desc, nullptr, constant_buffer.GetAddressOf());
		}
		return S_FALSE;
	}

	void ConstantBuffer::update_buffer(ID3D11DeviceContext *device_context)
	{
		if (is_dirty)
		{
			is_dirty = false;
			D3D11_MAPPED_SUBRESOURCE mapped_data{};
			device_context->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_data);
			std::memcpy(mapped_data.pData, upload_data.data(), upload_data.size());
			device_context->Unmap(constant_buffer.Get(), 0);
		}
	}

	void ConstantBuffer::set_shader_flag(ShaderType shader_type)
	{
		shader_flag = (shader_flag | shader_type);
	}

	void ConstantBuffer::emit_constant_buffer(ID3D11DeviceContext *device_context)
	{
		if (shader_flag & ShaderType::VertexShader) {
			bind_vs(device_context);
		}
		if (shader_flag & ShaderType::HullShader) {
			bind_hs(device_context);
		}
		if (shader_flag & ShaderType::DomainShader) {
			bind_ds(device_context);
		}
		if (shader_flag & ShaderType::GeometryShader) {
			bind_gs(device_context);
		}
		if (shader_flag & ShaderType::PixelShader) {
			bind_ps(device_context);
		}
		if (shader_flag & ShaderType::ComputeShader) {
			bind_cs(device_context);
		}
	}

	void ConstantBuffer::bind_vs(ID3D11DeviceContext *device_context)
	{
		device_context->VSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	void ConstantBuffer::bind_hs(ID3D11DeviceContext *device_context)
	{
		device_context->HSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	void ConstantBuffer::bind_ds(ID3D11DeviceContext *device_context)
	{
		device_context->DSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	void ConstantBuffer::bind_gs(ID3D11DeviceContext *device_context)
	{
		device_context->GSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	void ConstantBuffer::bind_ps(ID3D11DeviceContext *device_context)
	{
		device_context->PSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	void ConstantBuffer::bind_cs(ID3D11DeviceContext *device_context)
	{
		device_context->GSSetConstantBuffers(binding_slot, 1, constant_buffer.GetAddressOf());
	}

	// Constant buffer accessor
	ConstantBufferAccessor::ConstantBufferAccessor(ConstantBuffer *input_constant_buffer, const std::string &in_component_name, uint32_t in_offset, uint32_t in_size)
	: constant_buffer_ref(input_constant_buffer), component_name(in_component_name), component_offset(in_offset), component_size(in_size)
	{

	}

	void ConstantBufferAccessor::set_raw(const uint8_t *data, uint32_t offset_in_bytes, uint32_t size_in_bytes)
	{
		if (data == nullptr || offset_in_bytes > component_size) {
			return;
		}

		if ((offset_in_bytes + size_in_bytes) > component_size) {
			size_in_bytes = component_size - offset_in_bytes;
		}

		std::memcpy(constant_buffer_ref->upload_data.data() + component_offset + offset_in_bytes, data, size_in_bytes);
		constant_buffer_ref->is_dirty = true;
	}

	void ConstantBufferAccessor::set_matrix_in_bytes(const uint8_t *no_padding_data, uint32_t rows, uint32_t cols)
	{
		if (rows == 0 || rows > 4 || cols == 0 || cols > 4) {
			return;
		}

		uint32_t remain_bytes = component_size < 64 ? component_size : 64;
		uint8_t *dst_data = constant_buffer_ref->upload_data.data() + component_offset;
		while (remain_bytes > 0) {
			constexpr uint32_t stride = 16;
			uint32_t row_pitch = sizeof(uint32_t) * cols < remain_bytes ? sizeof(uint32_t) * cols : remain_bytes;
			std::memcpy(dst_data, no_padding_data, row_pitch);
			dst_data += stride;
			no_padding_data += sizeof(uint32_t) * cols;
			remain_bytes = remain_bytes < stride ? 0 : (remain_bytes - stride);
		}
		constant_buffer_ref->is_dirty = true;
	}

	void ConstantBufferAccessor::set_sint_matrix(const int32_t *no_padding_data, uint32_t rows, uint32_t cols)
	{
		set_matrix_in_bytes(reinterpret_cast<const uint8_t *>(no_padding_data), rows, cols);
	}

	void ConstantBufferAccessor::set_uint_matrix(const uint32_t *no_padding_data, uint32_t rows, uint32_t cols)
	{
		set_matrix_in_bytes(reinterpret_cast<const uint8_t *>(no_padding_data), rows, cols);
	}

	void ConstantBufferAccessor::set_float_matrix(const float *no_padding_data, uint32_t rows, uint32_t cols)
	{
		set_matrix_in_bytes(reinterpret_cast<const uint8_t *>(no_padding_data), rows, cols);
	}

	void ConstantBufferAccessor::set_sint_vector(std::span<int32_t> data)
	{
		auto components = (std::min)(static_cast<uint32_t>(data.size()), 4U);
		auto size_in_bytes = (std::min)(static_cast<uint32_t>(components * sizeof(uint32_t)), component_size);
		set_raw(reinterpret_cast<const uint8_t *>(data.data()), 0, size_in_bytes);
	}

	void ConstantBufferAccessor::set_uint_vector(std::span<uint32_t> data)
	{
		auto components = (std::min)(static_cast<uint32_t>(data.size()), 4U);
		auto size_in_bytes = (std::min)(static_cast<uint32_t>(components * sizeof(uint32_t)), component_size);
		set_raw(reinterpret_cast<const uint8_t *>(data.data()), 0, size_in_bytes);
	}

	void ConstantBufferAccessor::set_float_vector(std::span<float> data)
	{
		auto components = (std::min)(static_cast<uint32_t>(data.size()), 4U);
		auto size_in_bytes = (std::min)(static_cast<uint32_t>(components * sizeof(uint32_t)), component_size);
		set_raw(reinterpret_cast<const uint8_t *>(data.data()), 0, size_in_bytes);
	}

	void ConstantBufferAccessor::set_sint(int32_t data)
	{
		set_raw(reinterpret_cast<const uint8_t *>(&data), 0, sizeof(int32_t));
	}

	void ConstantBufferAccessor::set_uint(uint32_t data)
	{
		set_raw(reinterpret_cast<const uint8_t *>(&data), 0, sizeof(uint32_t));
	}

	void ConstantBufferAccessor::set_float(float data)
	{
		set_raw(reinterpret_cast<const uint8_t *>(&data), 0, sizeof(float));
	}

	// EmitShader
	void EmitShader::operator()(const VertexShaderInfo &vertex_shader) const
	{
		device_context->VSSetShader(vertex_shader.vs.Get(), nullptr, 0);
	}

	void EmitShader::operator()(const HullShaderInfo &hull_shader) const
	{
		device_context->HSSetShader(hull_shader.hs.Get(), nullptr, 0);
	}

	void EmitShader::operator()(const DomainShaderInfo &domain_shader) const
	{
		device_context->DSSetShader(domain_shader.ds.Get(), nullptr, 0);
	}

	void EmitShader::operator()(const GeometryShaderInfo &geometry_shader) const
	{
		device_context->GSSetShader(geometry_shader.gs.Get(), nullptr, 0);
	}

	void EmitShader::operator()(const PixelShaderInfo &pixel_shader) const
	{
		device_context->PSSetShader(pixel_shader.ps.Get(), nullptr, 0);
	}

	void EmitShader::operator()(const ComputeShaderInfo &compute_shader) const
	{
		device_context->CSSetShader(compute_shader.cs.Get(), nullptr, 0);
	}

	// Emit shader resource view
	static void emit_shader_resource_view(const ShaderResource &shader_resource, ID3D11DeviceContext *device_context)
	{
		if (shader_resource.shader_flag == ShaderType::VertexShader) {
			device_context->VSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		} else if (shader_resource.shader_flag == ShaderType::HullShader) {
			device_context->HSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		} else if (shader_resource.shader_flag == ShaderType::DomainShader) {
			device_context->DSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		} else if (shader_resource.shader_flag == ShaderType::GeometryShader) {
			device_context->GSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		} else if (shader_resource.shader_flag == ShaderType::PixelShader) {
			device_context->PSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		} else {
			device_context->CSSetShaderResources(shader_resource.bind_slot, 1, &shader_resource.srv);
		}
	}

	// Emit sampler
	static void emit_sampler_state(const SamplerState &sampler_state, ID3D11DeviceContext *device_context)
	{
		if (sampler_state.shader_flag == ShaderType::VertexShader) {
			device_context->VSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		} else if (sampler_state.shader_flag == ShaderType::HullShader) {
			device_context->HSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		} else if (sampler_state.shader_flag == ShaderType::DomainShader) {
			device_context->DSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		} else if (sampler_state.shader_flag == ShaderType::GeometryShader) {
			device_context->GSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		} else if (sampler_state.shader_flag == ShaderType::PixelShader) {
			device_context->PSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		} else {
			device_context->CSSetSamplers(sampler_state.bind_slot, 1, &sampler_state.sampler);
		}
	}

	// Emit unordered access view
	static void emit_unordered_access_view(RWResource &rw_resource, ID3D11DeviceContext *device_context)
	{
		if (rw_resource.shader_flag == ShaderType::PixelShader) {
			const bool need_init = rw_resource.first_init;
			rw_resource.first_init = false;
			device_context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
																	rw_resource.bind_slot, 1, &rw_resource.uav, (need_init ? &rw_resource.initial_count : nullptr));
		} else if (rw_resource.shader_flag == ShaderType::ComputeShader) {
			const bool need_init = rw_resource.enable_counter && rw_resource.first_init;
			rw_resource.first_init = false;
			device_context->CSSetUnorderedAccessViews(rw_resource.bind_slot, 1, &rw_resource.uav,
														(need_init ? &rw_resource.initial_count : nullptr));
		}
	}

	// Effect
	Effect::Effect(const PipelineStateObject &pipeline_state_object, ID3D11Device *device)
	{
		auto &&dxc_instance = DxcInStance::get();
		if (std::holds_alternative<GraphicsPipelineStateObject>(pipeline_state_object))
		{
			auto &&graphics_pipeline_state_object = std::get<GraphicsPipelineStateObject>(pipeline_state_object);
			rasterizer_state = graphics_pipeline_state_object.rasterizer_state;
			depth_stencil_state = graphics_pipeline_state_object.depth_stencil_state;
			blend_state = graphics_pipeline_state_object.blend_state;
			auto shader_target_profile = graphics_pipeline_state_object.shader_target_profile;

			// VS
			if (!graphics_pipeline_state_object.vs_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(graphics_pipeline_state_object.vs_path, ShaderType::VertexShader, shader_target_profile);
				update_shader_reflection(graphics_pipeline_state_object.vs_path, device, dxc_shader_result.shader_reflection.Get());
				VertexShaderInfo vs_info{};
				device->CreateVertexShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, vs_info.vs.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(vs_info));
			} else {
				pipeline_shader_manager.emplace_back(VertexShaderInfo{});
			}

			// HS
			if (!graphics_pipeline_state_object.hs_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(graphics_pipeline_state_object.hs_path, ShaderType::HullShader, shader_target_profile);
				update_shader_reflection(graphics_pipeline_state_object.hs_path, device, dxc_shader_result.shader_reflection.Get());
				HullShaderInfo hs_info{};
				device->CreateHullShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, hs_info.hs.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(hs_info));
			} else {
				pipeline_shader_manager.emplace_back(HullShaderInfo{});
			}

			// DS
			if (!graphics_pipeline_state_object.ds_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(graphics_pipeline_state_object.ds_path, ShaderType::DomainShader, shader_target_profile);
				update_shader_reflection(graphics_pipeline_state_object.ds_path, device, dxc_shader_result.shader_reflection.Get());
				DomainShaderInfo ds_info{};
				device->CreateDomainShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, ds_info.ds.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(ds_info));
			} else {
				pipeline_shader_manager.emplace_back(DomainShaderInfo{});
			}

			// GS
			if (!graphics_pipeline_state_object.gs_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(graphics_pipeline_state_object.gs_path, ShaderType::GeometryShader, shader_target_profile);
				update_shader_reflection(graphics_pipeline_state_object.gs_path, device, dxc_shader_result.shader_reflection.Get());
				GeometryShaderInfo gs_info{};
				device->CreateGeometryShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, gs_info.gs.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(gs_info));
			} else {
				pipeline_shader_manager.emplace_back(GeometryShaderInfo{});
			}

			// PS
			if (!graphics_pipeline_state_object.ps_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(graphics_pipeline_state_object.ps_path, ShaderType::PixelShader, shader_target_profile);
				update_shader_reflection(graphics_pipeline_state_object.ps_path, device, dxc_shader_result.shader_reflection.Get());
				PixelShaderInfo ps_info{};
				device->CreatePixelShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, ps_info.ps.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(ps_info));
			} else {
				pipeline_shader_manager.emplace_back(PixelShaderInfo{});
			}
		} else
		{
			auto &&compute_pipeline_state_object = std::get<ComputePipelineStateObject>(pipeline_state_object);
			auto shader_target_profile = compute_pipeline_state_object.shader_target_profile;

			// CS
			if (!compute_pipeline_state_object.cs_path.empty()) {
				auto dxc_shader_result = dxc_instance.create_shader_from_file(compute_pipeline_state_object.cs_path, ShaderType::ComputeShader, shader_target_profile);
				update_shader_reflection(compute_pipeline_state_object.cs_path, device, dxc_shader_result.shader_reflection.Get());
				ComputeShaderInfo cs_info{};
				device->CreateComputeShader(dxc_shader_result.shader_blob->GetBufferPointer(), dxc_shader_result.shader_blob->GetBufferSize(), nullptr, cs_info.cs.GetAddressOf());
				pipeline_shader_manager.emplace_back(std::move(cs_info));
			} else {
				pipeline_shader_manager.emplace_back(ComputeShaderInfo{});
			}
		}
	}

	void Effect::update_shader_reflection(std::wstring_view shader_name, ID3D11Device *device, ID3D12ShaderReflection *shader_reflection)
	{
		D3D12_SHADER_DESC shader_desc{};
		if (FAILED(shader_reflection->GetDesc(&shader_desc))) {
			std::cout << std::format("Failed to get shader reflection desc\n");
			return;
		}

		auto shader_type = static_cast<D3D12_SHADER_VERSION_TYPE>(D3D12_SHVER_GET_TYPE(shader_desc.Version));
		auto inner_shader_type = convert_to_internal_shader_type(shader_type);
		for (uint32_t i = 0; i < shader_desc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC shader_input_bind_desc{};
			shader_reflection->GetResourceBindingDesc(i, &shader_input_bind_desc);
			auto bind_type = shader_input_bind_desc.Type;
			if (bind_type == D3D_SIT_CBUFFER)
			{
				ID3D12ShaderReflectionConstantBuffer* shader_reflection_constant_buffer = shader_reflection->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC shader_buffer_desc{};
				shader_reflection_constant_buffer->GetDesc(&shader_buffer_desc);

				auto constant_buffer_id = string_to_id(shader_input_bind_desc.Name);
				ConstantBuffer *constant_buffer_ref = nullptr;
				if (constant_buffer_manager.contains(constant_buffer_id)) {
					constant_buffer_ref = constant_buffer_manager[constant_buffer_id].get();
					constant_buffer_ref->set_shader_flag(inner_shader_type);
				} else {
					constant_buffer_manager[constant_buffer_id] = std::make_unique<ConstantBuffer>(shader_input_bind_desc.Name, shader_input_bind_desc.BindPoint, shader_buffer_desc.Size);
					constant_buffer_ref = constant_buffer_manager[constant_buffer_id].get();
					constant_buffer_ref->create_buffer(device);
					constant_buffer_ref->set_shader_flag(inner_shader_type);
				}

				for (uint32_t j = 0; j < shader_buffer_desc.Variables; ++j)
				{
					ID3D12ShaderReflectionVariable* shader_reflection_variable = shader_reflection_constant_buffer->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC shader_variable_desc{};
					shader_reflection_variable->GetDesc(&shader_variable_desc);

					auto constant_buffer_var_id = string_to_id(shader_variable_desc.Name);
					if (!constant_buffer_accessor_manager.contains(constant_buffer_var_id)) {
						constant_buffer_accessor_manager[constant_buffer_var_id] = std::make_unique<ConstantBufferAccessor>(constant_buffer_ref, shader_variable_desc.Name,
																					shader_variable_desc.StartOffset, shader_variable_desc.Size);
					}
				}
				continue;
			}

			if (bind_type == D3D_SIT_TEXTURE || bind_type == D3D_SIT_TBUFFER || bind_type == D3D_SIT_STRUCTURED || bind_type == D3D_SIT_BYTEADDRESS)
			{
				auto srv_id = string_to_id(shader_input_bind_desc.Name);
				if (!shader_resource_manager.contains(srv_id)) {
					shader_resource_manager.try_emplace(srv_id, nullptr,  shader_input_bind_desc.Dimension, shader_input_bind_desc.BindPoint, inner_shader_type);
				}
				continue;
			}

			if (bind_type == D3D_SIT_UAV_RWTYPED || bind_type == D3D_SIT_UAV_RWSTRUCTURED || bind_type == D3D_SIT_UAV_RWBYTEADDRESS || bind_type == D3D_SIT_UAV_FEEDBACKTEXTURE ||
				bind_type == D3D_SIT_UAV_APPEND_STRUCTURED || bind_type == D3D_SIT_UAV_CONSUME_STRUCTURED || bind_type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER) {
				auto uav_id = string_to_id(shader_input_bind_desc.Name);
				if (!unordered_access_manager.contains(uav_id)) {
					unordered_access_manager.try_emplace(uav_id, nullptr, static_cast<D3D11_UAV_DIMENSION>(shader_input_bind_desc.Dimension), 0, shader_input_bind_desc.BindPoint,
											inner_shader_type, bind_type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER, false);
				}
				continue;
			}

			if (bind_type == D3D_SIT_SAMPLER)
			{
				auto sampler_id = string_to_id(shader_input_bind_desc.Name);
				if (!sampler_manager.contains(sampler_id)) {
					sampler_manager.try_emplace(sampler_id, nullptr, shader_input_bind_desc.BindPoint, inner_shader_type);
				}
			}
		}
	}

	ConstantBufferAccessor *Effect::query_constant_buffer_accessor(std::string_view variable_name)
	{
		auto constant_buffer_var_id = string_to_id(variable_name);
		if (constant_buffer_accessor_manager.contains(constant_buffer_var_id)) {
			return constant_buffer_accessor_manager[constant_buffer_var_id].get();
		}
		return nullptr;
	}

	void Effect::bind_shader_resource_view(std::string_view srv_name, ID3D11ShaderResourceView *srv)
	{
		auto shader_resource_id = string_to_id(srv_name);
		if (shader_resource_manager.contains(shader_resource_id)) {
			shader_resource_manager[shader_resource_id].srv = srv;
		}
	}

	void Effect::bind_sampler(std::string_view sampler_name, ID3D11SamplerState *sampler)
	{
		auto sampler_id = string_to_id(sampler_name);
		if (sampler_manager.contains(sampler_id)) {
			sampler_manager[sampler_id].sampler = sampler;
		}
	}

	void Effect::bind_unordered_access_view(std::string_view uav_name, ID3D11UnorderedAccessView *uav)
	{
		auto uav_id = string_to_id(uav_name);
		if (unordered_access_manager.contains(uav_id)) {
			unordered_access_manager[uav_id].uav = uav;
		}
	}

	void Effect::emit_pipeline(ID3D11DeviceContext *device_context)
	{
		for (auto &&shader_info : pipeline_shader_manager)
		{
			std::visit(EmitShader{ device_context }, shader_info);
		}

		for (auto &&constant_buffer_info : constant_buffer_manager)
		{
			constant_buffer_info.second->update_buffer(device_context);
			constant_buffer_info.second->emit_constant_buffer(device_context);
		}

		for (auto &&shader_resource_info : shader_resource_manager)
		{
			emit_shader_resource_view(shader_resource_info.second, device_context);
		}

		for (auto &&sampler_state_info : sampler_manager)
		{
			emit_sampler_state(sampler_state_info.second, device_context);
		}

		for (auto &&rw_resource_info : unordered_access_manager)
		{
			emit_unordered_access_view(rw_resource_info.second, device_context);
		}

		device_context->RSSetState(rasterizer_state.Get());
		device_context->OMSetDepthStencilState(depth_stencil_state.Get(), stencil_ref);
		device_context->OMSetBlendState(blend_state.Get(), blend_factor.data(), sample_mask);
	}


}








































