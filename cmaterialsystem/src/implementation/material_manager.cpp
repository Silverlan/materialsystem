// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include <VTFFile.h>
#endif
#include <VTFFormat.h>

module pragma.cmaterialsystem;

import :material_manager;

#undef max

CMaterialManager::CMaterialManager(prosper::IPrContext &context) : MaterialManager {}, ContextObject(context)
{
	m_textureManager = std::make_unique<pragma::material::TextureManager>(context);
	m_textureManager->SetRootDirectory("materials");

	context.GetShaderManager().RegisterShader("decompose_cornea", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::ShaderDecomposeCornea(context, identifier); });
	context.GetShaderManager().RegisterShader("ssbumpmap_to_normalmap", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::ShaderSSBumpMapToNormalMap(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::source2::ShaderGenerateTangentSpaceNormalMap(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map_proto", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::source2::ShaderGenerateTangentSpaceNormalMapProto(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_decompose_metalness_reflectance", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::source2::ShaderDecomposeMetalnessReflectance(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_decompose_pbr", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::source2::ShaderDecomposePBR(context, identifier); });
	context.GetShaderManager().RegisterShader("extract_image_channel", [](prosper::IPrContext &context, const std::string &identifier) { return new pragma::material::ShaderExtractImageChannel(context, identifier); });
	context.GetShaderManager().GetShader("copy_image"); // Make sure copy_image shader has been initialized
}

CMaterialManager::~CMaterialManager() {}

pragma::material::TextureManager &CMaterialManager::GetTextureManager() { return *m_textureManager; }

void CMaterialManager::SetShaderHandler(const std::function<void(pragma::material::Material *)> &handler) { m_shaderHandler = handler; }
std::function<void(pragma::material::Material *)> CMaterialManager::GetShaderHandler() const { return m_shaderHandler; }

void CMaterialManager::Update()
{
	m_textureManager->Poll();

	if(!m_reloadShaderQueue.empty()) {
		std::unordered_set<pragma::material::Material *> traversed;
		while(!m_reloadShaderQueue.empty()) {
			auto hMat = m_reloadShaderQueue.front();
			m_reloadShaderQueue.pop();

			if(!hMat.IsValid())
				continue;
			auto it = traversed.find(hMat.get());
			if(it != traversed.end())
				continue;
			traversed.insert(hMat.get());
			m_shaderHandler(hMat.get());
		}
	}
}

pragma::material::Material *CMaterialManager::CreateMaterial(const std::string *identifier, const std::string &shader, std::shared_ptr<pragma::datasystem::Block> root)
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	std::string matId;
	if(identifier != nullptr) {
		matId = *identifier;
		pragma::string::to_lower(matId);
		auto *mat = FindMaterial(matId);
		if(mat != nullptr) {
			mat->SetShaderInfo(shaderManager.PreRegisterShader(shader));
			return mat;
		}
	}
	else
		matId = "__anonymous" + std::to_string(m_unnamedIdx++);
	if(root == nullptr) {
		auto dataSettings = CreateDataSettings();
		root = std::make_shared<pragma::datasystem::Block>(*dataSettings);
	}
	pragma::material::CMaterial *mat = nullptr; // auto *mat = CreateMaterial<CMaterial>(shaderManager.PreRegisterShader(shader),root); // Claims ownership of 'root' and frees the memory at destruction
	mat->SetLoaded(true);
	mat->SetName(matId);
	AddMaterial(matId, *mat);
	return mat;
}
pragma::material::Material *CMaterialManager::CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<pragma::datasystem::Block> &root) { return CreateMaterial(&identifier, shader, root); }
pragma::material::Material *CMaterialManager::CreateMaterial(const std::string &shader, const std::shared_ptr<pragma::datasystem::Block> &root) { return CreateMaterial(nullptr, shader, root); }

void CMaterialManager::MarkForReload(pragma::material::CMaterial &mat)
{
	//m_reloadShaderQueue.push(mat.GetHandle());
}

void CMaterialManager::ReloadMaterialShaders()
{
	if(m_shaderHandler == nullptr)
		return;
	GetContext().WaitIdle();
	for(auto &hMat : m_materials) {
		if(hMat.IsValid() && hMat->IsLoaded() == true)
			m_shaderHandler(hMat.get());
	}
}

bool CMaterialManager::InitializeVMTData(VTFLib::CVMTFile &vmt, LoadInfo &info, pragma::datasystem::Block &rootData, pragma::datasystem::Settings &settings, const std::string &shader)
{
	//TODO: These do not work if the textures haven't been imported yet!!
	if(MaterialManager::InitializeVMTData(vmt, info, rootData, settings, shader) == false)
		return false;
	VTFLib::Nodes::CVMTNode *node = nullptr;
	auto *vmtRoot = vmt.GetRoot();
	if(pragma::string::compare<std::string>(shader, "eyes", false)) {
		info.shader = "eye_legacy";
		if((node = vmtRoot->GetNode("$iris")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				rootData.AddData("iris_map", std::make_shared<pragma::datasystem::Texture>(settings, irisNode->GetValue()));
			}
		}
		if((node = vmtRoot->GetNode("$basetexture")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				rootData.AddData("sclera_map", std::make_shared<pragma::datasystem::Texture>(settings, irisNode->GetValue()));
			}
		}
		rootData.AddValue("float", "iris_scale", "0.5");
	}
	else if(pragma::string::compare<std::string>(shader, "eyerefract", false)) {
		info.shader = "eye";
		std::string irisTexture = "";
		if((node = vmtRoot->GetNode("$iris")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				irisTexture = irisNode->GetValue();
			}
		}

		std::string corneaTexture = "";
		if((node = vmtRoot->GetNode("$corneatexture")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *corneaNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				corneaTexture = corneaNode->GetValue();
			}
		}

		// Some conversions are required for the iris and cornea textures for usage in Pragma
		auto &context = GetContext();
		auto *shaderDecomposeCornea = static_cast<pragma::material::ShaderDecomposeCornea *>(context.GetShader("decompose_cornea").get());
		if(shaderDecomposeCornea) {
			auto &textureManager = GetTextureManager();

			auto irisMap = textureManager.LoadAsset(irisTexture);
			if(irisMap == nullptr)
				irisMap = textureManager.GetErrorTexture();

			auto corneaMap = textureManager.LoadAsset(corneaTexture);
			if(corneaMap == nullptr)
				corneaMap = textureManager.GetErrorTexture();

			if(irisMap && irisMap->HasValidVkTexture() && corneaMap && corneaMap->HasValidVkTexture()) {
				// Prepare output textures (albedo, normal, parallax)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = pragma::math::max(irisMap->GetWidth(), corneaMap->GetWidth());
				imgCreateInfo.height = pragma::math::max(irisMap->GetHeight(), corneaMap->GetHeight());
				auto imgAlbedo = context.CreateImage(imgCreateInfo);

				imgCreateInfo.format = prosper::Format::R32G32B32A32_SFloat;
				auto imgNormal = context.CreateImage(imgCreateInfo);
				auto imgParallax = context.CreateImage(imgCreateInfo);
				auto imgNoise = context.CreateImage(imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texAlbedo = context.CreateTexture({}, *imgAlbedo, imgViewCreateInfo);
				auto texNormal = context.CreateTexture({}, *imgNormal, imgViewCreateInfo);
				auto texParallax = context.CreateTexture({}, *imgParallax, imgViewCreateInfo);
				auto texNoise = context.CreateTexture({}, *imgNoise, imgViewCreateInfo);
				auto rt = context.CreateRenderTarget({texAlbedo, texNormal, texParallax, texNoise}, shaderDecomposeCornea->GetRenderPass());

				auto dsg = shaderDecomposeCornea->CreateDescriptorSetGroup(pragma::material::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkIrisTex = irisMap->GetVkTexture();
				auto &vkCorneaTex = corneaMap->GetVkTexture();
				ds.SetBindingTexture(*vkIrisTex, pragma::math::to_integral(pragma::material::ShaderDecomposeCornea::TextureBinding::IrisMap));
				ds.SetBindingTexture(*vkCorneaTex, pragma::math::to_integral(pragma::material::ShaderDecomposeCornea::TextureBinding::CorneaMap));
				auto &setupCmd = context.GetSetupCommandBuffer();
				if(setupCmd->RecordBeginRenderPass(*rt)) {
					prosper::ShaderBindState bindState {*setupCmd};
					if(shaderDecomposeCornea->RecordBeginDraw(bindState)) {
						shaderDecomposeCornea->RecordDraw(bindState, ds);
						shaderDecomposeCornea->RecordEndDraw(bindState);
					}
					setupCmd->RecordEndRenderPass();
				}
				context.FlushSetupCommandBuffer();

				auto irisTextureNoExt = irisTexture;
				ufile::remove_extension_from_filename(irisTextureNoExt);
				auto corneaTextureNoExt = corneaTexture;
				ufile::remove_extension_from_filename(corneaTextureNoExt);

				auto albedoTexName = irisTextureNoExt + "_albedo";
				auto normalTexName = corneaTextureNoExt + "_normal";
				auto parallaxTexName = corneaTextureNoExt + "_parallax";
				auto noiseTexName = corneaTextureNoExt + "_noise";

				auto errHandler = [](const std::string &err) { std::cout << "WARNING: Unable to save eyeball image(s) as DDS: " << err << std::endl; };

				// TODO: Change width/height
				auto rootPath = "addons/converted/" + MaterialManager::GetRootMaterialLocation();
				pragma::image::TextureInfo texInfo {};
				texInfo.containerFormat = pragma::image::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = pragma::image::TextureInfo::AlphaMode::Auto;
				texInfo.flags = pragma::image::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = pragma::image::TextureInfo::InputFormat::R8G8B8A8_UInt;
				prosper::util::save_texture(rootPath + '/' + albedoTexName, texAlbedo->GetImage(), texInfo, errHandler);

				texInfo.outputFormat = pragma::image::TextureInfo::OutputFormat::GradientMap;
				texInfo.inputFormat = pragma::image::TextureInfo::InputFormat::R32G32B32A32_Float;
				prosper::util::save_texture(rootPath + '/' + parallaxTexName, texParallax->GetImage(), texInfo, errHandler);
				prosper::util::save_texture(rootPath + '/' + noiseTexName, texNoise->GetImage(), texInfo, errHandler);

				texInfo.outputFormat = pragma::image::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture(rootPath + '/' + normalTexName, texNormal->GetImage(), texInfo, errHandler);

				// TODO: These should be ematerial::ALBEDO_MAP_IDENTIFIER/ematerial::NORMAL_MAP_IDENTIFIER/ematerial::PARALLAX_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("albedo_map", std::make_shared<pragma::datasystem::Texture>(settings, albedoTexName));
				rootData.AddData("normal_map", std::make_shared<pragma::datasystem::Texture>(settings, normalTexName));
				rootData.AddData("parallax_map", std::make_shared<pragma::datasystem::Texture>(settings, parallaxTexName));
				rootData.AddData("noise_map", std::make_shared<pragma::datasystem::Texture>(settings, noiseTexName));
				rootData.AddValue("float", "metalness_factor", "0.0");
				rootData.AddValue("float", "roughness_factor", "0.0");

				// Default subsurface scattering values
				rootData.AddValue("float", "subsurface_multiplier", "0.01");
				rootData.AddValue("color", "subsurface_color", "242 210 157");
				rootData.AddValue("int", "subsurface_method", "5");
				rootData.AddValue("vector", "subsurface_radius", "112 52.8 1.6");
			}

			auto ptrRoot = std::static_pointer_cast<pragma::datasystem::Block>(rootData.shared_from_this());
			if((node = vmtRoot->GetNode("$eyeballradius")) != nullptr)
				pragma::material::get_vmt_data<pragma::datasystem::Bool, int32_t>(ptrRoot, settings, "eyeball_radius", node);
			if((node = vmtRoot->GetNode("$dilation")) != nullptr)
				pragma::material::get_vmt_data<pragma::datasystem::Bool, int32_t>(ptrRoot, settings, "pupil_dilation", node);
		}
	}
	else if(pragma::string::compare<std::string>(shader, "spritecard", false)) {
		// Some Source Engine textures contain embedded animation sheet data.
		// Since our texture formats don't support that, we'll have to extract it and
		// store it separately.
		info.shader = "particle";
		if((node = vmtRoot->GetNode("$basetexture")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *baseTexture = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				auto &textureManager = GetTextureManager();

				auto &context = GetContext();
				auto baseTexMap = textureManager.LoadAsset(baseTexture->GetValue());
				if(baseTexMap != nullptr) {
					auto &baseTexName = baseTexMap->GetName();
					auto name = MaterialManager::GetRootMaterialLocation() + '/' + baseTexName + ".vtf";
					auto fptr = pragma::fs::open_file(name.c_str(), pragma::fs::FileMode::Read | pragma::fs::FileMode::Binary);
					if(fptr) {
						pragma::fs::File f {fptr};
						VTFLib::CVTFFile fVtf {};
						if(fVtf.Load(&f, false)) {
							vlUInt resSize;
							auto *ptr = fVtf.GetResourceData(VTF_RSRC_SHEET, resSize);
							if(ptr) {
								pragma::util::DataStream ds {ptr, resSize};
								ds->SetOffset(0);
								auto version = ds->Read<int32_t>();
								assert(version == 0 || version == 1);
								auto numSequences = ds->Read<int32_t>();

								pragma::material::SpriteSheetAnimation anim {};
								auto &sequences = anim.sequences;
								sequences.reserve(numSequences);
								for(auto i = decltype(numSequences) {0}; i < numSequences; ++i) {
									auto seqIdx = ds->Read<int32_t>();
									if(seqIdx >= sequences.size())
										sequences.resize(seqIdx + 1);
									auto &seq = sequences.at(seqIdx);
									seq.loop = !static_cast<bool>(ds->Read<int32_t>());
									auto numFrames = ds->Read<int32_t>();
									seq.frames.resize(numFrames);
									auto sequenceLength = ds->Read<float>();
									for(auto j = decltype(numFrames) {0}; j < numFrames; ++j) {
										auto &frame = seq.frames.at(j);
										frame.duration = ds->Read<float>();

										auto constexpr MAX_IMAGES_PER_FRAME = 4u;
										frame.uvStart = ds->Read<Vector2>();
										frame.uvEnd = ds->Read<Vector2>();

										// Animation data can contain multiple images per frame.
										// I'm not sure what the purpose of that is (multi-texture?), but we'll ignore it for
										// the time being.
										if(version > 0)
											ds->SetOffset(ds->GetOffset() + sizeof(Vector2) * 2 * (MAX_IMAGES_PER_FRAME - 1));
#if 0
										for(auto t=decltype(MAX_IMAGES_PER_FRAME){0u};t<MAX_IMAGES_PER_FRAME;++t)
										{
											auto &img = frame.images.at(t);
											img.xMin = ds->Read<float>();
											img.yMin = ds->Read<float>();
											img.xMax = ds->Read<float>();
											img.yMax = ds->Read<float>();

											if(version == 0)
											{
												for(uint8_t i=1;i<MAX_IMAGES_PER_FRAME;++i)
													frame.images.at(i) = img;
												break;
											}
										}
#endif
									}
								}
								auto sequenceFilePath = pragma::util::Path {"addons/converted/" + MaterialManager::GetRootMaterialLocation() + '/' + baseTexName + ".psd"};
								pragma::fs::create_path(sequenceFilePath.GetPath().data());
								auto fSeq = pragma::fs::open_file<pragma::fs::VFilePtrReal>(sequenceFilePath.GetString().c_str(), pragma::fs::FileMode::Write | pragma::fs::FileMode::Binary);
								if(fSeq) {
									anim.Save(fSeq);
									rootData.AddValue("string", "animation", pragma::util::Path {baseTexName}.GetString());
								}
							}
						}
					}
				}
			}
		}

		auto *nodeOverbrightFactor = vmtRoot->GetNode("$overbrightfactor");
		if(nodeOverbrightFactor == nullptr)
			nodeOverbrightFactor = vmtRoot->GetNode("srgb?$overbrightfactor");
		if(nodeOverbrightFactor) {
			float overbrightFactor;
			if(pragma::material::vmt_parameter_to_numeric_type(nodeOverbrightFactor, overbrightFactor) && overbrightFactor != 0.f) {
				// Overbright factors can get fairly large (e.g. 31 -> "particle/blood1/blood_goop3_spray"), we'll scale it down for Pragma
				// so that a factor of 30 roughly equals 1.9
				overbrightFactor = pragma::math::max(overbrightFactor, 1.2f);
				overbrightFactor = logf(overbrightFactor) / logf(6.f); // log base 6
				Vector4 colorFactor {0.f, 0.f, 0.f, 0.f};
				auto vColorFactor = rootData.GetValue("bloom_color_factor");
				if(vColorFactor && typeid(*vColorFactor) == typeid(pragma::datasystem::Vector4))
					colorFactor = static_cast<pragma::datasystem::Vector4 &>(*vColorFactor).GetValue();
				colorFactor += Vector4 {overbrightFactor, overbrightFactor, overbrightFactor, 0.f};

				rootData.AddValue("vector4", "bloom_color_factor", std::to_string(colorFactor.r) + ' ' + std::to_string(colorFactor.g) + ' ' + std::to_string(colorFactor.b) + " 1.0");
			}
		}

		auto *nodeAddSelf = vmtRoot->GetNode("$addself");
		float addSelf;
		if(nodeAddSelf && pragma::material::vmt_parameter_to_numeric_type(nodeAddSelf, addSelf)) {
			Vector4 colorFactor {1.f, 1.f, 1.f, 1.f};
			auto vColorFactor = rootData.GetValue("color_factor");
			if(vColorFactor && typeid(*vColorFactor) == typeid(pragma::datasystem::Vector4))
				colorFactor = static_cast<pragma::datasystem::Vector4 &>(*vColorFactor).GetValue();
			colorFactor += Vector4 {addSelf, addSelf, addSelf, 0.f};

			rootData.AddValue("vector4", "color_factor", std::to_string(colorFactor.r) + ' ' + std::to_string(colorFactor.g) + ' ' + std::to_string(colorFactor.b) + " 1.0");
		}
	}
	int32_t ssBumpmap;
	if((node = vmtRoot->GetNode("$ssbump")) != nullptr && pragma::material::vmt_parameter_to_numeric_type<int32_t>(node, ssBumpmap) && ssBumpmap != 0) {
		// Material is using a self-shadowing bump map, which Pragma doesn't support, so we'll convert it to a normal map.
		std::string bumpMapTexture = "";
		if((node = vmtRoot->GetNode("$bumpmap")) != nullptr) {
			if(node->GetType() == NODE_TYPE_STRING) {
				auto *bumpNapNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				bumpMapTexture = bumpNapNode->GetValue();
			}
		}

		auto &context = GetContext();
		auto *shaderSSBumpMapToNormalMap = static_cast<pragma::material::ShaderSSBumpMapToNormalMap *>(context.GetShader("ssbumpmap_to_normalmap").get());
		context.GetShaderManager().GetShader("copy_image"); // Make sure copy_image shader has been initialized
		if(shaderSSBumpMapToNormalMap) {
			auto &textureManager = GetTextureManager();

			auto bumpMap = textureManager.LoadAsset(bumpMapTexture);
			if(bumpMap && bumpMap->HasValidVkTexture()) {
				// Prepare output texture (normal map)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R32G32B32A32_SFloat;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = bumpMap->GetWidth();
				imgCreateInfo.height = bumpMap->GetHeight();
				auto imgNormal = context.CreateImage(imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texNormal = context.CreateTexture({}, *imgNormal, imgViewCreateInfo);
				auto rt = context.CreateRenderTarget({texNormal}, shaderSSBumpMapToNormalMap->GetRenderPass());

				auto dsg = shaderSSBumpMapToNormalMap->CreateDescriptorSetGroup(pragma::material::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkBumpMapTex = bumpMap->GetVkTexture();
				ds.SetBindingTexture(*vkBumpMapTex, pragma::math::to_integral(pragma::material::ShaderSSBumpMapToNormalMap::TextureBinding::SSBumpMap));
				auto &setupCmd = context.GetSetupCommandBuffer();
				if(setupCmd->RecordBeginRenderPass(*rt)) {
					prosper::ShaderBindState bindState {*setupCmd};
					if(shaderSSBumpMapToNormalMap->RecordBeginDraw(bindState)) {
						shaderSSBumpMapToNormalMap->RecordDraw(bindState, ds);
						shaderSSBumpMapToNormalMap->RecordEndDraw(bindState);
					}
					setupCmd->RecordEndRenderPass();
				}
				context.FlushSetupCommandBuffer();

				auto bumpMapTextureNoExt = bumpMapTexture;
				ufile::remove_extension_from_filename(bumpMapTexture);

				auto normalTexName = bumpMapTexture + "_normal";

				auto errHandler = [](const std::string &err) { std::cout << "WARNING: Unable to save converted ss bumpmap as DDS: " << err << std::endl; };

				auto rootPath = "addons/converted/" + MaterialManager::GetRootMaterialLocation();
				pragma::image::TextureInfo texInfo {};
				texInfo.containerFormat = pragma::image::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = pragma::image::TextureInfo::AlphaMode::None;
				texInfo.flags |= pragma::image::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = pragma::image::TextureInfo::InputFormat::R32G32B32A32_Float;
				texInfo.outputFormat = pragma::image::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture(rootPath + '/' + normalTexName, texNormal->GetImage(), texInfo, errHandler);

				// TODO: This should be ematerial::NORMAL_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("normal_map", std::make_shared<pragma::datasystem::Texture>(settings, normalTexName));
			}
		}
	}
	info.saveOnDisk = true;
	GetTextureManager().ClearUnused();
	return true;
}

void CMaterialManager::SetErrorMaterial(pragma::material::Material *mat)
{
	MaterialManager::SetErrorMaterial(mat);
	if(mat != nullptr) {
		auto *diffuse = mat->GetDiffuseMap();
		if(diffuse != nullptr) {
			m_textureManager->SetErrorTexture(std::static_pointer_cast<pragma::material::Texture>(diffuse->texture));
			return;
		}
	}
	m_textureManager->SetErrorTexture(nullptr);
}

pragma::material::Material *CMaterialManager::Load(const std::string &path, const std::function<void(pragma::material::Material *)> &onMaterialLoaded, const std::function<void(std::shared_ptr<pragma::material::Texture>)> &onTextureLoaded, bool bReload, bool *bFirstTimeError, bool bLoadInstantly)
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	if(bFirstTimeError != nullptr)
		*bFirstTimeError = false;
	auto bInitializeTextures = false;
	auto bCopied = false;
	LoadInfo info {};
	if(!MaterialManager::Load(path, info, bReload)) {
		if(info.material != nullptr) // bReload was true, the material was already loaded and reloading failed
			return info.material;
		info.material = GetErrorMaterial();
		if(info.material == nullptr)
			return nullptr;
		if(bFirstTimeError != nullptr)
			*bFirstTimeError = true;
		//info.material = info.material->Copy(); // Copy error material
		bCopied = true;
		//bInitializeTextures = true; // TODO: Do we need to do this? (Error material should already be initialized); Callbacks will still have to be called!
	}
	else if(info.material != nullptr) // Material already exists, reload textured?
	{
		info.material->SetErrorFlag(false);
		if(bReload == false || !info.material->IsLoaded()) // Can't reload if material hasn't even been fully loaded yet
		{
			/*if(!static_cast<CMaterial*>(info.material)->HaveTexturesBeenLoaded())
			{
				auto *mat = info.material;
				auto texLoadFlags = TextureLoadFlags::None;
				pragma::math::set_flag(texLoadFlags,TextureLoadFlags::LoadInstantly,true);
				static_cast<CMaterial*>(info.material)->InitializeTextures(info.material->GetDataBlock(),[this,onMaterialLoaded,mat]() {
					mat->SetLoaded(true);
					if(onMaterialLoaded != nullptr)
						onMaterialLoaded(mat);
					if(m_shaderHandler != nullptr)
						m_shaderHandler(mat);
				},[onTextureLoaded](std::shared_ptr<Texture> texture) {
					if(onTextureLoaded != nullptr)
						onTextureLoaded(texture);
				},texLoadFlags);
			}*/
			return info.material;
		}
		info.material->Initialize(shaderManager.PreRegisterShader(info.shader), info.root);
		bInitializeTextures = true;
	}
	else // Failed to load material
	{
		//info.material = CreateMaterial<CMaterial>(shaderManager.PreRegisterShader(info.shader),info.root);
		bInitializeTextures = true;
	}
	info.material->SetName(info.identifier);
	// The material has to be registered before calling the callbacks below
	AddMaterial(info.identifier, *info.material);
	if(bInitializeTextures == true) {
		auto *mat = info.material;
		auto texLoadFlags = pragma::material::TextureLoadFlags::None;
		pragma::math::set_flag(texLoadFlags, pragma::material::TextureLoadFlags::LoadInstantly, bLoadInstantly);
		pragma::math::set_flag(texLoadFlags, pragma::material::TextureLoadFlags::Reload, bReload);
		/*static_cast<CMaterial*>(info.material)->InitializeTextures(info.material->GetDataBlock(),[this,onMaterialLoaded,mat]() {
			mat->SetLoaded(true);
			if(onMaterialLoaded != nullptr)
				onMaterialLoaded(mat);
			if(m_shaderHandler != nullptr)
				m_shaderHandler(mat);
		},[onTextureLoaded](std::shared_ptr<Texture> texture) {
			if(onTextureLoaded != nullptr)
				onTextureLoaded(texture);
		},texLoadFlags);*/
	}
	else if(bCopied == true) // Material has been copied; Callbacks will have to be called anyway (To make sure descriptor set is initialized properly!)
	{
		if(onMaterialLoaded != nullptr)
			onMaterialLoaded(info.material);
		if(m_shaderHandler != nullptr)
			m_shaderHandler(info.material);
	}
	if(info.saveOnDisk) {
		std::string err;
		info.material->Save("addons/converted/materials/" + info.material->GetName(), err, true);
	}
	return info.material;
}

pragma::material::Material *CMaterialManager::Load(const std::string &path, bool bReload, bool loadInstantly, bool *bFirstTimeError) { return Load(path, nullptr, nullptr, bReload, bFirstTimeError, loadInstantly); }
