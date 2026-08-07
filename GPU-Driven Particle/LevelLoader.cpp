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
	if (source.contains("lifeTimeMin")) settings.lifeTimeMin = source.at("lifeTimeMin").get<float>();
	if (source.contains("lifeTimeMax")) settings.lifeTimeMax = source.at("lifeTimeMax").get<float>();
	if (source.contains("speedMin")) settings.speedMin = source.at("speedMin").get<float>();
	if (source.contains("speedMax")) settings.speedMax = source.at("speedMax").get<float>();
	if (source.contains("shapeType")) settings.shapeType = source.at("shapeType").get<int>();
	if (source.contains("velocityMode")) settings.velocityMode = source.at("velocityMode").get<int>();
	if (source.contains("sphereRadius")) settings.sphereRadius = source.at("sphereRadius").get<float>();
	if (source.contains("sphereSurfaceOnly")) settings.sphereSurfaceOnly = source.at("sphereSurfaceOnly").get<bool>();
	if (source.contains("coneAngle")) settings.coneAngle = source.at("coneAngle").get<float>();
	if (source.contains("collisionEnabled")) settings.collisionEnabled = source.at("collisionEnabled").get<bool>();
	if (source.contains("restitution")) settings.restitution = source.at("restitution").get<float>();
	if (source.contains("friction")) settings.friction = source.at("friction").get<float>();
	if (source.contains("forceAvoidEnabled")) settings.forceAvoidEnabled = source.at("forceAvoidEnabled").get<bool>();
	if (source.contains("forceTangentEnabled")) settings.forceTangentEnabled = source.at("forceTangentEnabled").get<bool>();
	if (source.contains("forceAvoidStrength")) settings.forceAvoidStrength = source.at("forceAvoidStrength").get<float>();
	if (source.contains("forceTangentStrength")) settings.forceTangentStrength = source.at("forceTangentStrength").get<float>();
	if (source.contains("forceRadius")) settings.forceRadius = source.at("forceRadius").get<float>();
	if (source.contains("rendererType")) settings.rendererType = source.at("rendererType").get<int>();
	if (source.contains("blendMode")) settings.blendMode = source.at("blendMode").get<int>();
	if (source.contains("textureIndex")) settings.textureIndex = source.at("textureIndex").get<int>();
	if (source.contains("sortEnabled")) settings.sortEnabled = source.at("sortEnabled").get<bool>();
	if (source.contains("gravity")) ReadFloat3(source.at("gravity"), (prefix + ".settings.gravity").c_str(), settings.gravity);
	if (source.contains("boxExtents")) ReadFloat3(source.at("boxExtents"), (prefix + ".settings.boxExtents").c_str(), settings.boxExtents);
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
		if (emitterJson.contains("settings"))
		{
			GP::ParticleSettings settings;
			ApplyEmitterSettings(emitterJson.at("settings"), settings, prefix);
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
		scene.Clear();
		particles.ClearEmitters();

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
			if (emitterJson.contains("settings"))
				ApplyEmitterSettings(emitterJson.at("settings"), emitter.GetSettings(), prefix);
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
			object.GetTransform().SetRotation(ReadQuaternion(objectJson.at("rotation"), (prefix + ".rotation").c_str()));

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
	particleSystemJson["collision"] = collisionJson;

	ojson emittersJson = ojson::array();
	for (size_t index = 0; index < particles.GetEmitterCount(); ++index)
	{
		ParticleEmitter& emitter = particles.GetEmitter(index);
		const ParticleSettings& settings = emitter.GetSettings();

		ojson settingsJson;
		settingsJson["spawnRate"] = settings.spawnRate;
		settingsJson["lifeTimeMin"] = settings.lifeTimeMin;
		settingsJson["lifeTimeMax"] = settings.lifeTimeMax;
		settingsJson["speedMin"] = settings.speedMin;
		settingsJson["speedMax"] = settings.speedMax;
		settingsJson["shapeType"] = settings.shapeType;
		settingsJson["velocityMode"] = settings.velocityMode;
		settingsJson["sphereRadius"] = settings.sphereRadius;
		settingsJson["sphereSurfaceOnly"] = settings.sphereSurfaceOnly;
		settingsJson["coneAngle"] = settings.coneAngle;
		settingsJson["collisionEnabled"] = settings.collisionEnabled;
		settingsJson["restitution"] = settings.restitution;
		settingsJson["friction"] = settings.friction;
		settingsJson["forceAvoidEnabled"] = settings.forceAvoidEnabled;
		settingsJson["forceTangentEnabled"] = settings.forceTangentEnabled;
		settingsJson["forceAvoidStrength"] = settings.forceAvoidStrength;
		settingsJson["forceTangentStrength"] = settings.forceTangentStrength;
		settingsJson["forceRadius"] = settings.forceRadius;
		settingsJson["rendererType"] = settings.rendererType;
		settingsJson["blendMode"] = settings.blendMode;
		settingsJson["textureIndex"] = settings.textureIndex;
		settingsJson["sortEnabled"] = settings.sortEnabled;
		settingsJson["gravity"] = Float3Json(settings.gravity);
		settingsJson["boxExtents"] = Float3Json(settings.boxExtents);

		ojson emitterJson;
		emitterJson["position"] = Vec3Json(emitter.GetBasePosition());
		emitterJson["settings"] = settingsJson;
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
