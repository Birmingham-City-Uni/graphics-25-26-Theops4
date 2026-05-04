#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "SpotLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	
	// *** Load shaders and textures ***

	// vector of textures
	std::vector<std::string> textureFiles = {
		"../models/Pulse_Rifle_Tex.png",
		"../models/Pulse_Rifle_Tex.png",
		"../models/Bars_Tex.png",
		"../models/Bars_Tex.png",
		"../models/Wall_Tex.png",
		"../models/Wall_Tex.png",
		"../models/Floor_Tex.png",
		"../models/Dark_Wall_Tex.png",
		"../models/Floor_Tex.png",
		"../models/Soldier_Tex.png",
		"../models/Soldier_Tex.png",
	};

	std::vector<std::string> shaderTypes = {
		"texturedLambertian",
		"texturedLambertian",
		"texturedLambertian",
		"texturedLambertian",
		"Phong",
		"Phong",
		"Phong",
		"texturedLambertian",
		"Phong",
		"Phong",
		"Phong"
	};

	//vector of texture data 
	std::vector<std::vector<uint8_t>> textureDataList(textureFiles.size());
	//vector of shaders
	std::vector<std::unique_ptr<Shader>> textureShaders;

	//loop to decide on shader type and load texture data
	for (size_t i = 0; i < textureFiles.size(); ++i) {
		unsigned int w, h;
		lodepng::decode(textureDataList[i], w, h, textureFiles[i]);

		if (shaderTypes[i] == "texturedLambertian") {
			textureShaders.push_back(std::make_unique<TexturedLambertianShader>(&textureDataList[i], w, h));
		}

		else {
			textureShaders.push_back(std::make_unique<PhongShader>(&textureDataList[i], w, h, Eigen::Vector3f(0.5f, 0.5f, 0.5f), 128.f));
		}

	}

	// *** Set up scene ***
	Scene scene;

	Model sceneModel("../models/GameScene12.obj");
	for (size_t i = 0; i < sceneModel.meshes.size(); ++i) {
		scene.renderables.push_back(std::make_shared<BVHNode>(sceneModel, textureShaders[i].get(), 4, makeTranslationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f)) * uniformScale(0.05) * rotateY(0.f), &sceneModel.meshes[i]));
	}
	
	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight = Eigen::Vector3f(0.2f, 0.2f, 0.2f);

	std::vector<std::unique_ptr<Light>> lightSources;
	/*lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));*/
	/*lightSources.push_back(std::make_unique<SpotLight>( Eigen::Vector3f(1.f, 3.f, 5.f), Eigen::Vector3f(30.f, 30.f, 30.f) ,Eigen::Vector3f(0.f, -1.f, 0.f), 0.785f));*/
	lightSources.push_back(std::make_unique<SpotLight>( Eigen::Vector3f(1.f, 9.f, 11.f), Eigen::Vector3f(3.49f, 6.24f, 5.75f) * 90.f, Eigen::Vector3f(-2.2f, -1.f, -2.2f).normalized(), 1.5f));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(8.f, 0.f, -20.f), Eigen::Vector3f(3.49f, 7.24f, 5.55f) * 3.f));
	/*lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.f, 0.f, -18.f), Eigen::Vector3f(3.49f, 7.24f, 6.5f) * 2.f));*/
	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;

			//fog colour
			Eigen::Vector3f fogColor = Eigen::Vector3f(4.49f, 7.24f, 6.75f)/9.f;
			//fog density
			float fogDensity = 0.025f;
			//fog distance
			float fogstart = 15.f;

			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				// Apply fog effect based on distance
				float fogDistance = std::max(0.0f, hitInfo.hitT - fogstart);
				float fog = 1.f - std::exp(-fogDensity * fogDistance);

				//interpolate between the original color and the fog color based on the fog factor
				color = color * (1.f - fog) + fogColor * fog;

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
