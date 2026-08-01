#include "pch.h"

#include "LevelLoader.h"
#include "ThirdParty/json.hpp"

#include "BufferManager.h"
#include "Camera.h"
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
	if (source.contains("rendererType")) settings.rendererType = source.at("rendererType").get<int>();
	if (source.contains("blendMode")) settings.blendMode = source.at("blendMode").get<int>();
	if (source.contains("textureIndex")) settings.textureIndex = source.at("textureIndex").get<int>();
	if (source.contains("sortEnabled")) settings.sortEnabled = source.at("sortEnabled").get<bool>();
	if (source.contains("gravity")) ReadFloat3(source.at("gravity"), (prefix + ".settings.gravity").c_str(), settings.gravity);
	if (source.contains("boxExtents")) ReadFloat3(source.at("boxExtents"), (prefix + ".settings.boxExtents").c_str(), settings.boxExtents);
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

		if (root.at("version").get<int>() != 1)
		{
			outError = "Unsupported scene version";
			return false;
		}

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
