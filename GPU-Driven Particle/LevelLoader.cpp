#include "pch.h"

#include "LevelLoader.h"
#include "ThirdParty/json.hpp"

#include "BufferManager.h"
#include "Camera.h"
#include "Mesh.h"
#include "MeshLibrary.h"
#include "ParticleSystem.h"
#include "Scene.h"
#include "SceneObject.h"

#include <fstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

static Math::Vector3 ReadVec3(const json& value, const char* fieldName)
{
	if (!value.is_array() || value.size() != 3)
		throw std::runtime_error(std::string(fieldName) + " must be an array of 3 numbers");

	try
	{
		return Math::Vector3(
			value.at(0).get<float>(),
			value.at(1).get<float>(),
			value.at(2).get<float>());
	}
	catch (const json::exception&)
	{
		throw std::runtime_error(std::string(fieldName) + " must contain only numbers");
	}
}

static Math::Vector4 ReadVec4(const json& value, const char* fieldName)
{
	if (!value.is_array() || value.size() != 4)
		throw std::runtime_error(std::string(fieldName) + " must be an array of 4 numbers");

	try
	{
		return Math::Vector4(
			value.at(0).get<float>(),
			value.at(1).get<float>(),
			value.at(2).get<float>(),
			value.at(3).get<float>());
	}
	catch (const json::exception&)
	{
		throw std::runtime_error(std::string(fieldName) + " must contain only numbers");
	}
}

static Math::Quaternion ReadQuaternion(const json& value, const char* fieldName)
{
	if (!value.is_array() || value.size() != 4)
		throw std::runtime_error(std::string(fieldName) + " must be [x, y, z, w]");

	const float x = value.at(0).get<float>();
	const float y = value.at(1).get<float>();
	const float z = value.at(2).get<float>();
	const float w = value.at(3).get<float>();
	const float lengthSq = x * x + y * y + z * z + w * w;

	if (lengthSq < 1e-8f)
		throw std::runtime_error(std::string(fieldName) + " must not be zero");

	return Math::Normalize(Math::Quaternion(DirectX::XMVectorSet(x, y, z, w)));
}

static void ReadFloat3(const json& value, const char* fieldName, float (&destination)[3])
{
	const Math::Vector3 source = ReadVec3(value, fieldName);
	DirectX::XMFLOAT3 unpacked;
	DirectX::XMStoreFloat3(&unpacked, source);
	destination[0] = unpacked.x;
	destination[1] = unpacked.y;
	destination[2] = unpacked.z;
}

static void ReadFloat4(const json& value, const char* fieldName, float (&destination)[4])
{
	const Math::Vector4 source = ReadVec4(value, fieldName);
	DirectX::XMFLOAT4 unpacked;
	DirectX::XMStoreFloat4(&unpacked, source);
	destination[0] = unpacked.x;
	destination[1] = unpacked.y;
	destination[2] = unpacked.z;
	destination[3] = unpacked.w;
}

static void RequireArray(const json& value, const char* fieldName)
{
	if (!value.is_array())
		throw std::runtime_error(std::string(fieldName) + " must be an array");
}

static void ApplyEmitterSettings(const json& source, GP::ParticleSettings& settings, const std::string& prefix)
{
	if (!source.is_object())
		throw std::runtime_error(prefix + ".settings must be an object");

	if (source.contains("spawnRate")) settings.spawnRate = source.at("spawnRate").get<float>();
	if (source.contains("burstCount")) settings.burstCount = source.at("burstCount").get<int>();
	if (source.contains("loopMode")) settings.loopMode = source.at("loopMode").get<int>();
	if (source.contains("loopDuration")) settings.loopDuration = source.at("loopDuration").get<float>();
	if (source.contains("loopCount")) settings.loopCount = source.at("loopCount").get<int>();
	if (source.contains("orbitEnabled")) settings.orbitEnabled = source.at("orbitEnabled").get<bool>();
	if (source.contains("orbitRadius")) settings.orbitRadius = source.at("orbitRadius").get<float>();
	if (source.contains("orbitSpeed")) settings.orbitSpeed = source.at("orbitSpeed").get<float>();
	if (source.contains("lifeTimeMin")) settings.lifeTimeMin = source.at("lifeTimeMin").get<float>();
	if (source.contains("lifeTimeMax")) settings.lifeTimeMax = source.at("lifeTimeMax").get<float>();
	if (source.contains("speedMin")) settings.speedMin = source.at("speedMin").get<float>();
	if (source.contains("speedMax")) settings.speedMax = source.at("speedMax").get<float>();
	if (source.contains("spinSpeedMin")) settings.spinSpeedMin = source.at("spinSpeedMin").get<float>();
	if (source.contains("spinSpeedMax")) settings.spinSpeedMax = source.at("spinSpeedMax").get<float>();
	if (source.contains("initAngleMin")) settings.initAngleMin = source.at("initAngleMin").get<float>();
	if (source.contains("initAngleMax")) settings.initAngleMax = source.at("initAngleMax").get<float>();
	if (source.contains("rotationRateMin")) settings.rotationRateMin = source.at("rotationRateMin").get<float>();
	if (source.contains("rotationRateMax")) settings.rotationRateMax = source.at("rotationRateMax").get<float>();
	if (source.contains("rotationAxis")) ReadFloat3(source.at("rotationAxis"), (prefix + ".settings.rotationAxis").c_str(), settings.rotationAxis);
	if (source.contains("randomRotationAxis")) settings.randomRotationAxis = source.at("randomRotationAxis").get<bool>();
	if (source.contains("randomInitOrientation")) settings.randomInitOrientation = source.at("randomInitOrientation").get<bool>();
	if (source.contains("sizeMode")) settings.sizeMode = source.at("sizeMode").get<int>();
	if (source.contains("sizeMin")) ReadFloat3(source.at("sizeMin"), (prefix + ".settings.sizeMin").c_str(), settings.sizeMin);
	if (source.contains("sizeMax")) ReadFloat3(source.at("sizeMax"), (prefix + ".settings.sizeMax").c_str(), settings.sizeMax);
	if (source.contains("dirSpread")) settings.dirSpread = source.at("dirSpread").get<float>();
	if (source.contains("posSpread")) settings.posSpread = source.at("posSpread").get<float>();
	if (source.contains("startColor")) ReadFloat4(source.at("startColor"), (prefix + ".settings.startColor").c_str(), settings.startColor);
	if (source.contains("randomSpawnBrightness")) settings.randomSpawnBrightness = source.at("randomSpawnBrightness").get<bool>();
	if (source.contains("shapeType")) settings.shapeType = source.at("shapeType").get<int>();
	if (source.contains("velocityMode")) settings.velocityMode = source.at("velocityMode").get<int>();
	if (source.contains("sphereRadius")) settings.sphereRadius = source.at("sphereRadius").get<float>();
	if (source.contains("sphereSurfaceOnly")) settings.sphereSurfaceOnly = source.at("sphereSurfaceOnly").get<bool>();
	if (source.contains("coneAngle")) settings.coneAngle = source.at("coneAngle").get<float>();
	if (source.contains("endColor")) ReadFloat4(source.at("endColor"), (prefix + ".settings.endColor").c_str(), settings.endColor);
	if (source.contains("sizeOverLife")) settings.sizeOverLife = source.at("sizeOverLife").get<bool>();
	if (source.contains("colorOverLife")) settings.colorOverLife = source.at("colorOverLife").get<bool>();
	if (source.contains("alphaOverLife")) settings.alphaOverLife = source.at("alphaOverLife").get<bool>();
	if (source.contains("collisionEnabled")) settings.collisionEnabled = source.at("collisionEnabled").get<bool>();
	if (source.contains("restitution")) settings.restitution = source.at("restitution").get<float>();
	if (source.contains("friction")) settings.friction = source.at("friction").get<float>();
	if (source.contains("forceAvoidEnabled")) settings.forceAvoidEnabled = source.at("forceAvoidEnabled").get<bool>();
	if (source.contains("forceTangentEnabled")) settings.forceTangentEnabled = source.at("forceTangentEnabled").get<bool>();
	if (source.contains("forceAvoidStrength")) settings.forceAvoidStrength = source.at("forceAvoidStrength").get<float>();
	if (source.contains("forceTangentStrength")) settings.forceTangentStrength = source.at("forceTangentStrength").get<float>();
	if (source.contains("forceTangentAxis")) ReadFloat3(source.at("forceTangentAxis"), (prefix + ".settings.forceTangentAxis").c_str(), settings.forceTangentAxis);
	if (source.contains("surfaceInfluenceRadius")) settings.surfaceInfluenceRadius = source.at("surfaceInfluenceRadius").get<float>();
	else if (source.contains("forceRadius")) settings.surfaceInfluenceRadius = source.at("forceRadius").get<float>();
	if (source.contains("forceCurlEnabled")) settings.forceCurlEnabled = source.at("forceCurlEnabled").get<bool>();
	if (source.contains("curlFrequency")) settings.curlFrequency = source.at("curlFrequency").get<float>();
	else if (source.contains("forceCurlScale")) settings.curlFrequency = source.at("forceCurlScale").get<float>();
	if (source.contains("curlTargetSpeed")) settings.curlTargetSpeed = source.at("curlTargetSpeed").get<float>();
	else if (source.contains("forceCurlStrength")) settings.curlTargetSpeed = source.at("forceCurlStrength").get<float>();
	if (source.contains("curlResponseRate")) settings.curlResponseRate = source.at("curlResponseRate").get<float>();
	if (source.contains("curlPsiBoundary")) settings.curlPsiBoundary = source.at("curlPsiBoundary").get<bool>();
	if (source.contains("forceAttractEnabled")) settings.forceAttractEnabled = source.at("forceAttractEnabled").get<bool>();
	if (source.contains("forceAttractStrength")) settings.forceAttractStrength = source.at("forceAttractStrength").get<float>();
	if (source.contains("forceAttractTarget")) settings.forceAttractTarget = source.at("forceAttractTarget").get<int>();
	if (source.contains("morphEnabled")) settings.morphEnabled = source.at("morphEnabled").get<bool>();
	if (source.contains("morphStrength")) settings.morphStrength = source.at("morphStrength").get<float>();
	if (source.contains("morphTargetPosition")) ReadFloat3(source.at("morphTargetPosition"), (prefix + ".settings.morphTargetPosition").c_str(), settings.morphTargetPosition);
	if (source.contains("morphTargetRotation")) ReadFloat3(source.at("morphTargetRotation"), (prefix + ".settings.morphTargetRotation").c_str(), settings.morphTargetRotation);
	if (source.contains("morphTargetScale")) ReadFloat3(source.at("morphTargetScale"), (prefix + ".settings.morphTargetScale").c_str(), settings.morphTargetScale);
	if (source.contains("rendererType")) settings.rendererType = source.at("rendererType").get<int>();
	if (source.contains("blendMode")) settings.blendMode = source.at("blendMode").get<int>();
	if (source.contains("alignmentMode")) settings.alignmentMode = source.at("alignmentMode").get<int>();
	if (source.contains("textureIndex")) settings.textureIndex = source.at("textureIndex").get<int>();
	if (source.contains("subImagesX")) settings.subImagesX = source.at("subImagesX").get<int>();
	if (source.contains("subImagesY")) settings.subImagesY = source.at("subImagesY").get<int>();
	if (source.contains("sortEnabled")) settings.sortEnabled = source.at("sortEnabled").get<bool>();
	if (source.contains("ribbonUVMode")) settings.ribbonUVMode = source.at("ribbonUVMode").get<int>();
	if (source.contains("gravity")) ReadFloat3(source.at("gravity"), (prefix + ".settings.gravity").c_str(), settings.gravity);
	if (source.contains("boxExtents")) ReadFloat3(source.at("boxExtents"), (prefix + ".settings.boxExtents").c_str(), settings.boxExtents);
}

// sequence JSON 검증 및 파싱
static void ReadSequenceStages(const json& sequenceJson, GP::MeshLibrary& meshLibrary,
	const std::string& prefix, GP::EmitterSequence* outSequence)
{
	if (!sequenceJson.is_object())
		throw std::runtime_error(prefix + ".sequence must be an object");

	if (sequenceJson.contains("loop"))
	{
		const bool loop = sequenceJson.at("loop").get<bool>();
		if (outSequence)
			outSequence->loop = loop;
	}

	const json& stagesJson = sequenceJson.at("stages");
	RequireArray(stagesJson, (prefix + ".sequence.stages").c_str());
	if (stagesJson.empty())
		throw std::runtime_error(prefix + ".sequence.stages must contain at least one stage");

	for (size_t i = 0; i < stagesJson.size(); ++i)
	{
		const json& stageJson = stagesJson.at(i);
		const std::string stagePrefix = prefix + ".sequence.stages[" + std::to_string(i) + "]";
		if (!stageJson.is_object())
			throw std::runtime_error(stagePrefix + " must be an object");

		GP::SequenceStage stage;
		if (stageJson.contains("duration"))
			stage.duration = stageJson.at("duration").get<float>();

		if (stageJson.contains("morph"))
		{
			const json& morphJson = stageJson.at("morph");
			if (!morphJson.is_object())
				throw std::runtime_error(stagePrefix + ".morph must be an object");

			stage.replacesMorphTarget = true;
			stage.morphMeshPath = morphJson.at("mesh").get<std::string>();
			stage.morphSampleCount = morphJson.at("sampleCount").get<uint32_t>();
			stage.morphSeed = morphJson.at("seed").get<uint32_t>();

			if (stage.morphSampleCount == 0)
				throw std::runtime_error(stagePrefix + ".morph.sampleCount must be greater than zero");

			GP::Mesh* mesh = meshLibrary.Get(stage.morphMeshPath.c_str());
			if (mesh == nullptr)
				throw std::runtime_error(stagePrefix + ".morph.mesh failed to load: " + stage.morphMeshPath);
			if (mesh->GetCPUVertices().empty() || mesh->GetCPUIndices().empty())
				throw std::runtime_error(stagePrefix + ".morph.mesh has no cpu geometry: " + stage.morphMeshPath);
		}

		if (stageJson.contains("settings"))
		{
			GP::ParticleSettings validationDummy;
			ApplyEmitterSettings(stageJson.at("settings"), validationDummy, stagePrefix);
		}

		if (outSequence)
			outSequence->stages.push_back(stage);
	}
}

// Load가 기존 씬을 지우기 전에 JSON과 메시 경로를 전량 검증
// 여기를 통과하면 Load 본문은 던지지 않으므로, 실패해도 현재 씬과 파티클 상태가 유지
// 메시 로드를 실제로 시도하니 MeshLibrary 캐시는 채워질 수 있음
// Load와 같은 필드를 같은 조건으로 검사 (Load에 필드 추가 시 여기도 같이 추가할 것)
static void ValidateSceneData(const json& root, GP::MeshLibrary& meshLibrary)
{
	if (root.at("version").get<int>() != 1)
		throw std::runtime_error("Unsupported scene version");

	const json& cameraJson = root.at("camera");
	ReadVec3(cameraJson.at("position"), "camera.position");
	ReadVec3(cameraJson.at("lookAt"), "camera.lookAt");
	ReadVec3(cameraJson.at("up"), "camera.up");
	const float fovYRadians = cameraJson.at("fovYRadians").get<float>();
	const float nearZ = cameraJson.at("nearZ").get<float>();
	const float farZ = cameraJson.at("farZ").get<float>();
	if (fovYRadians <= 0.0f || nearZ <= 0.0f || farZ <= nearZ)
		throw std::runtime_error("camera requires fovYRadians > 0 and 0 < nearZ < farZ");

	const json& particleSystemJson = root.at("particleSystem");
	if (!particleSystemJson.is_object())
		throw std::runtime_error("particleSystem must be an object");

	if (particleSystemJson.contains("halfResolution"))
		particleSystemJson.at("halfResolution").get<bool>();

	if (particleSystemJson.contains("collision"))
	{
		const json& collisionJson = particleSystemJson.at("collision");
		if (!collisionJson.is_object())
			throw std::runtime_error("particleSystem.collision must be an object");

		GP::CollisionSettings collision;
		if (collisionJson.contains("planeEnabled")) collisionJson.at("planeEnabled").get<bool>();
		if (collisionJson.contains("planeNormal")) ReadFloat3(collisionJson.at("planeNormal"), "particleSystem.collision.planeNormal", collision.planeNormal);
		if (collisionJson.contains("planeOffset")) collisionJson.at("planeOffset").get<float>();
		if (collisionJson.contains("sphereEnabled")) collisionJson.at("sphereEnabled").get<bool>();
		if (collisionJson.contains("sphereCenter")) ReadFloat3(collisionJson.at("sphereCenter"), "particleSystem.collision.sphereCenter", collision.sphereCenter);
		if (collisionJson.contains("sphereRadius")) collisionJson.at("sphereRadius").get<float>();
		if (collisionJson.contains("sdfEnabled")) collisionJson.at("sdfEnabled").get<bool>();
		if (collisionJson.contains("useBVH")) collisionJson.at("useBVH").get<bool>();
	}

	const json& emittersJson = particleSystemJson.at("emitters");
	RequireArray(emittersJson, "particleSystem.emitters");
	if (emittersJson.empty())
		throw std::runtime_error("particleSystem.emitters must contain at least one emitter");
	for (size_t index = 0; index < emittersJson.size(); ++index)
	{
		const json& emitterJson = emittersJson.at(index);
		if (!emitterJson.is_object())
			throw std::runtime_error("particleSystem.emitters[" + std::to_string(index) + "] must be an object");

		const std::string prefix = "particleSystem.emitters[" + std::to_string(index) + "]";
		ReadVec3(emitterJson.at("position"), (prefix + ".position").c_str());
		if (emitterJson.contains("rotation"))
		{
			float rotation[3];
			ReadFloat3(emitterJson.at("rotation"), (prefix + ".rotation").c_str(), rotation);
		}
		if (emitterJson.contains("settings"))
		{
			GP::ParticleSettings settings;
			ApplyEmitterSettings(emitterJson.at("settings"), settings, prefix);
		}
		if (emitterJson.contains("morph"))
		{
			const json& morphJson = emitterJson.at("morph");
			if (!morphJson.is_object())
				throw std::runtime_error(prefix + ".morph must be an object");

			const std::string meshPath = morphJson.at("mesh").get<std::string>();
			const uint32_t sampleCount = morphJson.at("sampleCount").get<uint32_t>();
			morphJson.at("seed").get<uint32_t>();

			if (sampleCount == 0)
				throw std::runtime_error(prefix + ".morph.sampleCount must be greater than zero");

			GP::Mesh* mesh = meshLibrary.Get(meshPath.c_str());
			if (mesh == nullptr)
				throw std::runtime_error(prefix + ".morph.mesh failed to load: " + meshPath);
			if (mesh->GetCPUVertices().empty() || mesh->GetCPUIndices().empty())
				throw std::runtime_error(prefix + ".morph.mesh has no cpu geometry: " + meshPath);
		}
		if (emitterJson.contains("particleMesh"))
		{
			const std::string meshPath = emitterJson.at("particleMesh").get<std::string>();
			if (meshLibrary.Get(meshPath.c_str()) == nullptr)
				throw std::runtime_error(prefix + ".particleMesh failed to load: " + meshPath);
		}
		if (emitterJson.contains("sequence"))
			ReadSequenceStages(emitterJson.at("sequence"), meshLibrary, prefix, nullptr);
	}

	const json& objectsJson = root.at("objects");
	RequireArray(objectsJson, "objects");
	for (size_t index = 0; index < objectsJson.size(); ++index)
	{
		const json& objectJson = objectsJson.at(index);
		if (!objectJson.is_object())
			throw std::runtime_error("objects[" + std::to_string(index) + "] must be an object");

		const std::string prefix = "objects[" + std::to_string(index) + "]";
		const std::string meshPath = objectJson.at("mesh").get<std::string>();
		if (meshLibrary.Get(meshPath.c_str()) == nullptr)
			throw std::runtime_error(prefix + ".mesh failed to load: " + meshPath);

		if (objectJson.contains("name"))
			objectJson.at("name").get<std::string>();
		ReadVec3(objectJson.at("position"), (prefix + ".position").c_str());
		ReadQuaternion(objectJson.at("rotation"), (prefix + ".rotation").c_str());
		if (objectJson.value("scale", 1.0f) <= 0.0f)
			throw std::runtime_error(prefix + ".scale must be greater than zero");

		if (objectJson.contains("color"))
		{
			float color[4];
			ReadFloat4(objectJson.at("color"), (prefix + ".color").c_str(), color);
		}
		objectJson.value("visible", true);
		objectJson.value("sdfCollider", false);
	}
}

using ojson = nlohmann::ordered_json;

static ojson Vec3Json(const Math::Vector3& v)
{
	DirectX::XMFLOAT3 f;
	DirectX::XMStoreFloat3(&f, v);
	return ojson::array({ f.x, f.y, f.z });
}

static ojson Float3Json(const float (&v)[3])
{
	return ojson::array({ v[0], v[1], v[2] });
}

static ojson Float4Json(const float (&v)[4])
{
	return ojson::array({ v[0], v[1], v[2], v[3] });
}

static ojson QuaternionJson(const Math::Quaternion& q)
{
	DirectX::XMFLOAT4 f;
	DirectX::XMStoreFloat4(&f, q);
	return ojson::array({ f.x, f.y, f.z, f.w });
}

// ParticleSettings -> JSON
static ojson EmitterSettingsToJson(const GP::ParticleSettings& settings)
{
	ojson settingsJson;
	settingsJson["spawnRate"] = settings.spawnRate;
	settingsJson["burstCount"] = settings.burstCount;
	settingsJson["loopMode"] = settings.loopMode;
	settingsJson["loopDuration"] = settings.loopDuration;
	settingsJson["loopCount"] = settings.loopCount;
	settingsJson["orbitEnabled"] = settings.orbitEnabled;
	settingsJson["orbitRadius"] = settings.orbitRadius;
	settingsJson["orbitSpeed"] = settings.orbitSpeed;
	settingsJson["lifeTimeMin"] = settings.lifeTimeMin;
	settingsJson["lifeTimeMax"] = settings.lifeTimeMax;
	settingsJson["speedMin"] = settings.speedMin;
	settingsJson["speedMax"] = settings.speedMax;
	settingsJson["spinSpeedMin"] = settings.spinSpeedMin;
	settingsJson["spinSpeedMax"] = settings.spinSpeedMax;
	settingsJson["initAngleMin"] = settings.initAngleMin;
	settingsJson["initAngleMax"] = settings.initAngleMax;
	settingsJson["rotationRateMin"] = settings.rotationRateMin;
	settingsJson["rotationRateMax"] = settings.rotationRateMax;
	settingsJson["rotationAxis"] = Float3Json(settings.rotationAxis);
	settingsJson["randomRotationAxis"] = settings.randomRotationAxis;
	settingsJson["randomInitOrientation"] = settings.randomInitOrientation;
	settingsJson["sizeMode"] = settings.sizeMode;
	settingsJson["sizeMin"] = Float3Json(settings.sizeMin);
	settingsJson["sizeMax"] = Float3Json(settings.sizeMax);
	settingsJson["dirSpread"] = settings.dirSpread;
	settingsJson["posSpread"] = settings.posSpread;
	settingsJson["startColor"] = Float4Json(settings.startColor);
	settingsJson["randomSpawnBrightness"] = settings.randomSpawnBrightness;
	settingsJson["shapeType"] = settings.shapeType;
	settingsJson["velocityMode"] = settings.velocityMode;
	settingsJson["sphereRadius"] = settings.sphereRadius;
	settingsJson["sphereSurfaceOnly"] = settings.sphereSurfaceOnly;
	settingsJson["coneAngle"] = settings.coneAngle;
	settingsJson["endColor"] = Float4Json(settings.endColor);
	settingsJson["sizeOverLife"] = settings.sizeOverLife;
	settingsJson["colorOverLife"] = settings.colorOverLife;
	settingsJson["alphaOverLife"] = settings.alphaOverLife;
	settingsJson["collisionEnabled"] = settings.collisionEnabled;
	settingsJson["restitution"] = settings.restitution;
	settingsJson["friction"] = settings.friction;
	settingsJson["forceAvoidEnabled"] = settings.forceAvoidEnabled;
	settingsJson["forceTangentEnabled"] = settings.forceTangentEnabled;
	settingsJson["forceAvoidStrength"] = settings.forceAvoidStrength;
	settingsJson["forceTangentStrength"] = settings.forceTangentStrength;
	settingsJson["forceTangentAxis"] = Float3Json(settings.forceTangentAxis);
	settingsJson["surfaceInfluenceRadius"] = settings.surfaceInfluenceRadius;
	settingsJson["forceCurlEnabled"] = settings.forceCurlEnabled;
	settingsJson["curlFrequency"] = settings.curlFrequency;
	settingsJson["curlTargetSpeed"] = settings.curlTargetSpeed;
	settingsJson["curlResponseRate"] = settings.curlResponseRate;
	settingsJson["curlPsiBoundary"] = settings.curlPsiBoundary;
	settingsJson["forceAttractEnabled"] = settings.forceAttractEnabled;
	settingsJson["forceAttractStrength"] = settings.forceAttractStrength;
	settingsJson["forceAttractTarget"] = settings.forceAttractTarget;
	settingsJson["morphEnabled"] = settings.morphEnabled;
	settingsJson["morphStrength"] = settings.morphStrength;
	settingsJson["morphTargetPosition"] = Float3Json(settings.morphTargetPosition);
	settingsJson["morphTargetRotation"] = Float3Json(settings.morphTargetRotation);
	settingsJson["morphTargetScale"] = Float3Json(settings.morphTargetScale);
	settingsJson["rendererType"] = settings.rendererType;
	settingsJson["blendMode"] = settings.blendMode;
	settingsJson["alignmentMode"] = settings.alignmentMode;
	settingsJson["textureIndex"] = settings.textureIndex;
	settingsJson["subImagesX"] = settings.subImagesX;
	settingsJson["subImagesY"] = settings.subImagesY;
	settingsJson["sortEnabled"] = settings.sortEnabled;
	settingsJson["ribbonUVMode"] = settings.ribbonUVMode;
	settingsJson["gravity"] = Float3Json(settings.gravity);
	settingsJson["boxExtents"] = Float3Json(settings.boxExtents);
	return settingsJson;
}

bool GP::LevelLoader::Load(const char* path, Scene& scene, MeshLibrary& meshLibrary, Camera& camera, ParticleSystem& particles, std::string& outError)
{
	outError.clear();

	std::ifstream file(path);
	if (!file)
	{
		outError = std::string("Cannot open scene file: ") + path;
		return false;
	}

	try
	{
		json root;
		file >> root;

		ValidateSceneData(root, meshLibrary);

		// 검증을 통과한 경우에만 기존 런타임 상태를 교체
		particles.ClearSDFColliders();
		particles.ClearEmitters();
		particles.ClearMorphTargets();
		scene.Clear();

		const json& cameraJson = root.at("camera");
		const Math::Vector3 position = ReadVec3(cameraJson.at("position"), "camera.position");
		const Math::Vector3 lookAt = ReadVec3(cameraJson.at("lookAt"), "camera.lookAt");
		const Math::Vector3 up = ReadVec3(cameraJson.at("up"), "camera.up");
		const float fovYRadians = cameraJson.at("fovYRadians").get<float>();
		const float nearZ = cameraJson.at("nearZ").get<float>();
		const float farZ = cameraJson.at("farZ").get<float>();

		if (fovYRadians <= 0.0f || nearZ <= 0.0f || farZ <= nearZ)
			throw std::runtime_error("camera requires fovYRadians > 0 and 0 < nearZ < farZ");

		const float cameraAspect =
			static_cast<float>(Graphics::g_SceneColorBuffer.GetHeight()) /
			static_cast<float>(Graphics::g_SceneColorBuffer.GetWidth());
		camera.SetEyeAtUp(position, lookAt, up);
		camera.SetPerspective(fovYRadians, cameraAspect, nearZ, farZ);

		const json& particleSystemJson = root.at("particleSystem");
		if (!particleSystemJson.is_object())
			throw std::runtime_error("particleSystem must be an object");

		if (particleSystemJson.contains("halfResolution"))
			particles.GetHalfResolution() = particleSystemJson.at("halfResolution").get<bool>();

		if (particleSystemJson.contains("collision"))
		{
			const json& collisionJson = particleSystemJson.at("collision");
			if (!collisionJson.is_object())
				throw std::runtime_error("particleSystem.collision must be an object");

			GP::CollisionSettings& collision = particles.GetCollisionSettings();
			if (collisionJson.contains("planeEnabled")) collision.planeEnabled = collisionJson.at("planeEnabled").get<bool>();
			if (collisionJson.contains("planeNormal")) ReadFloat3(collisionJson.at("planeNormal"), "particleSystem.collision.planeNormal", collision.planeNormal);
			if (collisionJson.contains("planeOffset")) collision.planeOffset = collisionJson.at("planeOffset").get<float>();
			if (collisionJson.contains("sphereEnabled")) collision.sphereEnabled = collisionJson.at("sphereEnabled").get<bool>();
			if (collisionJson.contains("sphereCenter")) ReadFloat3(collisionJson.at("sphereCenter"), "particleSystem.collision.sphereCenter", collision.sphereCenter);
			if (collisionJson.contains("sphereRadius")) collision.sphereRadius = collisionJson.at("sphereRadius").get<float>();
			if (collisionJson.contains("sdfEnabled")) collision.sdfEnabled = collisionJson.at("sdfEnabled").get<bool>();
			if (collisionJson.contains("useBVH")) collision.useBVH = collisionJson.at("useBVH").get<bool>();
		}

		const json& emittersJson = particleSystemJson.at("emitters");
		RequireArray(emittersJson, "particleSystem.emitters");
		if (emittersJson.empty())
			throw std::runtime_error("particleSystem.emitters must contain at least one emitter");
		for (size_t index = 0; index < emittersJson.size(); ++index)
		{
			const json& emitterJson = emittersJson.at(index);
			if (!emitterJson.is_object())
				throw std::runtime_error("particleSystem.emitters[" + std::to_string(index) + "] must be an object");

			const std::string prefix = "particleSystem.emitters[" + std::to_string(index) + "]";
			const Math::Vector3 emitterPosition = ReadVec3(emitterJson.at("position"), (prefix + ".position").c_str());

			if (index >= particles.GetEmitterCount())
				particles.AddEmitter(Math::OrthogonalTransform(emitterPosition));

			GP::ParticleEmitter& emitter = particles.GetEmitter(index);
			emitter.SetBasePosition(emitterPosition);
			if (emitterJson.contains("rotation"))
			{
				float rotation[3];
				ReadFloat3(emitterJson.at("rotation"), (prefix + ".rotation").c_str(), rotation);
				emitter.SetBaseRotationEuler(rotation);
			}
			if (emitterJson.contains("settings"))
				ApplyEmitterSettings(emitterJson.at("settings"), emitter.GetSettings(), prefix);
			if (emitterJson.contains("particleMesh"))
			{
				const std::string meshPath = emitterJson.at("particleMesh").get<std::string>();
				emitter.SetParticleMesh(meshLibrary.Get(meshPath.c_str()), meshPath);
			}
			GP::MorphTargetSet* emitterMorphTarget = nullptr;
			if (emitterJson.contains("morph"))
			{
				const json& morphJson = emitterJson.at("morph");
				const std::string meshPath = morphJson.at("mesh").get<std::string>();
				const uint32_t sampleCount = morphJson.at("sampleCount").get<uint32_t>();
				const uint32_t seed = morphJson.at("seed").get<uint32_t>();

				GP::Mesh* mesh = meshLibrary.Get(meshPath.c_str());
				emitterMorphTarget = particles.ResolveSurfaceMorphTarget(meshPath, *mesh, sampleCount, seed);
				emitter.SetMorphTarget(emitterMorphTarget);
				emitter.SetMorphTargetPath(meshPath);
				emitter.SetMorphTargetSampleCount(sampleCount);
				emitter.SetMorphTargetSeed(seed);
			}
			if (emitterJson.contains("sequence"))
			{
				ReadSequenceStages(emitterJson.at("sequence"), meshLibrary, prefix, &emitter.GetSequence());
				EmitterSequence& sequence = emitter.GetSequence();
				const json& stagesJson = emitterJson.at("sequence").at("stages");
				for (size_t stageIndex = 0; stageIndex < sequence.stages.size(); ++stageIndex)
				{
					// 첫 스테이지는 이미터 설정, 이후는 직전 스테이지 완성본을 복사
					sequence.stages[stageIndex].settings = (stageIndex == 0) ?
						emitter.GetSettings() : sequence.stages[stageIndex - 1].settings;

					// JSON에 적힌 키만 그 복사본 위에 덮어씀 (스테이지 완성본 확정)
					const json& stageJson = stagesJson.at(stageIndex);
					if (stageJson.contains("settings"))
						ApplyEmitterSettings(stageJson.at("settings"), sequence.stages[stageIndex].settings,
							prefix + ".sequence.stages[" + std::to_string(stageIndex) + "]");
				}
				MorphTargetSet* inheritedTarget = emitterMorphTarget; // 스테이지 0 기준 = 이미터 자기 타깃
				for (size_t stageIndex = 0; stageIndex < sequence.stages.size(); ++stageIndex)
				{
					SequenceStage& currentStage = sequence.stages[stageIndex];
					// morph 쓰는 스테이지만
					if (currentStage.replacesMorphTarget)
					{
						Mesh* mesh = meshLibrary.Get(currentStage.morphMeshPath.c_str());
						inheritedTarget = particles.ResolveSurfaceMorphTarget(currentStage.morphMeshPath, *mesh, currentStage.morphSampleCount, currentStage.morphSeed);
					}
					currentStage.morphTarget = inheritedTarget; // 스테이지에 할당
				}
				emitter.ApplyStage(); // 스테이지 시작
			}
		}

		const json& objectsJson = root.at("objects");
		RequireArray(objectsJson, "objects");
		for (size_t index = 0; index < objectsJson.size(); ++index)
		{
			const json& objectJson = objectsJson.at(index);
			if (!objectJson.is_object())
				throw std::runtime_error("objects[" + std::to_string(index) + "] must be an object");

			const std::string prefix = "objects[" + std::to_string(index) + "]";
			const std::string meshPath = objectJson.at("mesh").get<std::string>();
			GP::Mesh* mesh = meshLibrary.Get(meshPath.c_str());
			if (mesh == nullptr)
				throw std::runtime_error(prefix + ".mesh failed to load: " + meshPath);

			GP::SceneObject& object = scene.CreateObject(mesh);
			object.SetName(objectJson.value("name", meshPath));
			object.GetTransform().SetTranslation(ReadVec3(objectJson.at("position"), (prefix + ".position").c_str()));
			object.SetRotation(ReadQuaternion(objectJson.at("rotation"), (prefix + ".rotation").c_str()));

			const float scale = objectJson.value("scale", 1.0f);
			if (scale <= 0.0f)
				throw std::runtime_error(prefix + ".scale must be greater than zero");
			object.GetTransform().SetScale(scale);

			if (objectJson.contains("color"))
				ReadFloat4(objectJson.at("color"), (prefix + ".color").c_str(), object.GetMaterial().baseColor);
			object.SetVisible(objectJson.value("visible", true));
			object.SetSDFCollider(objectJson.value("sdfCollider", false));
		}
	}
	catch (const json::exception& e)
	{
		outError = std::string("Invalid scene JSON: ") + e.what();
		return false;
	}
	catch (const std::exception& e)
	{
		outError = std::string("Invalid scene data: ") + e.what();
		return false;
	}

	return true;
}

bool GP::LevelLoader::Save(const char* path, const Scene& scene, const Camera& camera, ParticleSystem& particles, std::string& outError)
{
	outError.clear();

	ojson root;
	root["version"] = 1;

	ojson cameraJson;
	cameraJson["position"] = Vec3Json(camera.GetPosition());
	cameraJson["lookAt"] = Vec3Json(camera.GetPosition() + camera.GetForward());
	cameraJson["up"] = Vec3Json(camera.GetUp());
	cameraJson["fovYRadians"] = camera.GetFOV();
	cameraJson["nearZ"] = camera.GetNearClip();
	cameraJson["farZ"] = camera.GetFarClip();
	root["camera"] = cameraJson;

	ojson particleSystemJson;
	particleSystemJson["halfResolution"] = particles.GetHalfResolution();

	const CollisionSettings& collision = particles.GetCollisionSettings();
	ojson collisionJson;
	collisionJson["planeEnabled"] = collision.planeEnabled;
	collisionJson["planeNormal"] = Float3Json(collision.planeNormal);
	collisionJson["planeOffset"] = collision.planeOffset;
	collisionJson["sphereEnabled"] = collision.sphereEnabled;
	collisionJson["sphereCenter"] = Float3Json(collision.sphereCenter);
	collisionJson["sphereRadius"] = collision.sphereRadius;
	collisionJson["sdfEnabled"] = collision.sdfEnabled;
	collisionJson["useBVH"] = collision.useBVH;
	particleSystemJson["collision"] = collisionJson;

	ojson emittersJson = ojson::array();
	for (size_t index = 0; index < particles.GetEmitterCount(); ++index)
	{
		ParticleEmitter& emitter = particles.GetEmitter(index);
		const EmitterSequence& sequence = emitter.GetSequence();

		ojson emitterJson;
		emitterJson["position"] = Vec3Json(emitter.GetBasePosition());
		emitterJson["rotation"] = Float3Json(emitter.GetBaseRotationEuler());
		if (!emitter.GetParticleMeshPath().empty())
			emitterJson["particleMesh"] = emitter.GetParticleMeshPath();
		if (!emitter.GetMorphTargetPath().empty())
		{
			ojson morphJson;
			morphJson["mesh"] = emitter.GetMorphTargetPath();
			morphJson["sampleCount"] = emitter.GetMorphTargetSampleCount();
			morphJson["seed"] = emitter.GetMorphTargetSeed();
			emitterJson["morph"] = morphJson;
		}

		if (sequence.stages.empty())
		{
			emitterJson["settings"] = EmitterSettingsToJson(emitter.GetSettings());
		}
		else
		{
			// 시퀀스가 있으면 라이브 settings는 ApplyStage가 덮은 상태라 기준값으로 못 씀
			// 스테이지 0 완성본을 이미터 settings로 내보내고, diff 기준도 여기서 시작
			ojson base = EmitterSettingsToJson(sequence.stages[0].settings);
			emitterJson["settings"] = base;

			ojson sequenceJson;
			sequenceJson["loop"] = sequence.loop;
			ojson stagesJson = ojson::array();
			for (const SequenceStage& stage : sequence.stages)
			{
				ojson stageJson;
				stageJson["duration"] = stage.duration;

				if (stage.replacesMorphTarget)
				{
					ojson morphJson;
					morphJson["mesh"] = stage.morphMeshPath;
					morphJson["sampleCount"] = stage.morphSampleCount;
					morphJson["seed"] = stage.morphSeed;
					stageJson["morph"] = morphJson;
				}

				ojson current = EmitterSettingsToJson(stage.settings);
				ojson patch;
				for (auto it = current.begin(); it != current.end(); ++it)
				{
					if (it.value() != base[it.key()])
						patch[it.key()] = it.value();
				}
				if (!patch.empty())
					stageJson["settings"] = patch;

				base = std::move(current);
				stagesJson.push_back(stageJson);
			}
			sequenceJson["stages"] = stagesJson;
			emitterJson["sequence"] = sequenceJson;
		}
		emittersJson.push_back(emitterJson);
	}
	particleSystemJson["emitters"] = emittersJson;
	root["particleSystem"] = particleSystemJson;

	ojson objectsJson = ojson::array();
	for (const auto& object : scene.GetObjects())
	{
		const Mesh* mesh = object->GetMesh();
		if (mesh == nullptr || mesh->GetSourcePath().empty())
		{
			outError = "Object '" + object->GetName() + "' has no mesh source path, cannot save";
			return false;
		}

		ojson objectJson;
		objectJson["name"] = object->GetName();
		objectJson["mesh"] = mesh->GetSourcePath();
		objectJson["position"] = Vec3Json(object->GetTransform().GetTranslation());
		objectJson["rotation"] = QuaternionJson(object->GetTransform().GetRotation());
		objectJson["scale"] = (float)object->GetTransform().GetScale();
		objectJson["color"] = Float4Json(object->GetMaterial().baseColor);
		objectJson["visible"] = object->IsVisible();
		objectJson["sdfCollider"] = object->IsSDFCollider();
		objectsJson.push_back(objectJson);
	}
	root["objects"] = objectsJson;

	std::ofstream file(path);
	if (!file)
	{
		outError = std::string("Cannot open scene file for writing: ") + path;
		return false;
	}
	file << root.dump(2) << '\n';

	if (!file.good())
	{
		outError = std::string("Failed to write scene file: ") + path;
		return false;
	}
	return true;
}
