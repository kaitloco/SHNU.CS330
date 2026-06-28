///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <iostream>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";

	const char* g_MaterialAmbientStrength = "material.ambientStrength";
	const char* g_MaterialAmbientColor = "material.ambientColor";
	const char* g_MaterialDiffuseColor = "material.diffuseColor";
	const char* g_MaterialSpecularColor = "material.specularColor";
	const char* g_MaterialShininess = "material.shininess";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();

	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	//parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	if (image)
	{
		std::cout << "Successfully loaded image:" << filename
			<< ", width:" << width
			<< ", height:" << height
			<< ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with "
				<< colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	return false;
}

/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	// if there are no defined materials, then return false
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			//color and material properties for the current object being rendered
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// set the model transformation matrix for the shader
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// ensure that the scale values are not zero

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;
	// set the current color value for the shader

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureSlot = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)

			//defining the shader material properties for the current object being rendered
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/**************************************************************/

/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	//Defining the basic shapes to be used in the scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();

	//creating defined path to scene textures 

	CreateGLTexture(
		"../../../Utilities/textures/breadcrust.jpg", "breadcrust");
	CreateGLTexture(
		"../../../Utilities/textures/cheese_wheel.jpg", "cheese");

	CreateGLTexture(
		"../../../Utilities/textures/rusticwood.jpg", "wood");

	CreateGLTexture("../../../Utilities/textures/stainless.jpg", "metal");

	CreateGLTexture(
		"../../../Utilities/textures/tilesf2.jpg", "marble");

	BindGLTextures();

	// define materials
	DefineObjectMaterials();

	SetupSceneLights();
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees;
	float YrotationDegrees;
	float ZrotationDegrees;
	glm::vec3 positionXYZ;

	/****************** FLOOR ******************/
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderColor(0.82f, 0.76f, 0.66f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	/****************** WALL ******************/
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 5.0f, -2.0f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderColor(0.93f, 0.91f, 0.87f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	/****************** TABLE TOP ******************/
	scaleXYZ = glm::vec3(4.5f, 0.15f, 1.2f);
	positionXYZ = glm::vec3(0.0f, 1.6f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderTexture("wood");
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/****************** TABLE LOWER SHELF ******************/
	scaleXYZ = glm::vec3(4.0f, 0.1f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 0.6f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	/****************** TABLE LEGS ******************/
	scaleXYZ = glm::vec3(0.12f, 1.5f, 0.12f);

	positionXYZ = glm::vec3(2.1f, 0.75f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(2.1f, 0.75f, -0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-2.1f, 0.75f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-2.1f, 0.75f, -0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	/****************** BASKETS ******************/
	
	scaleXYZ = glm::vec3(0.6f, 0.7f, 0.6f);

	positionXYZ = glm::vec3(-1.0f, 1.35f, 0.0f);
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.55f, 0.42f, 0.28f, 1.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	positionXYZ = glm::vec3(1.0f, 1.35f, 0.0f);
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.32f, 0.22f, 0.14f, 1.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************** BOOKS ******************/
	
	scaleXYZ = glm::vec3(0.5f, 0.08f, 0.7f);

	positionXYZ = glm::vec3(-0.7f, 1.71f, -0.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.1f, 0.12f, 0.2f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.7f, 1.79f, -0.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.85f, 0.82f, 0.7f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/****************** Fish bowl  ******************/
	
	scaleXYZ = glm::vec3(0.25f);
	positionXYZ = glm::vec3(-0.7f, 2.08f, -0.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.45f, 0.25f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	/****************** FRAME ******************/
	
	scaleXYZ = glm::vec3(3.6f, 1.8f, 0.08f);
	positionXYZ = glm::vec3(0.0f, 4.2f, -1.85f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("default");
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/****************** LAMP BASE ******************/

	scaleXYZ = glm::vec3(0.3f, 0.8f, 0.3f);
	positionXYZ = glm::vec3(1.0f, 1.675f, -0.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.42f, 0.38f, 0.34f, 1.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************** LAMP SHADE ******************/
	
	scaleXYZ = glm::vec3(0.45f, 0.35f, 0.45f);
	positionXYZ = glm::vec3(1.0f, 2.475f, -0.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.85f, 0.7f, 0.45f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
}
/***********************************************************
 *  DefineObjectMaterials()
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL material;

	
	material.ambientStrength = 0.05f;
	material.ambientColor = glm::vec3(1.0f);
	
	material.diffuseColor = glm::vec3(0.15f);
	material.specularColor = glm::vec3(0.5f);
	material.shininess = 32.0f;
	material.tag = "default";

	m_objectMaterials.push_back(material);
}

/***********************************************************
 *  SetupSceneLights()
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	if (m_pShaderManager == NULL)
		return;

	
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(3.0f, 8.0f, 5.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.15f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 1.0f, 0.95f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.8f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	// set a low specular intensity for the first light source to avoid overly bright highlights
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.02f);
	// set the second light source to have a cooler color and a lower intensity
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-4.0f, 5.0f, 3.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f, 0.05f, 0.1f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.3f, 0.4f, 0.8f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.3f, 0.3f, 0.5f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.015f);

	
	for (int i = 2; i < 4; i++)
	{
		// set the remaining light sources to have no effect on the scene
		std::string base = "lightSources[" + std::to_string(i) + "].";
		m_pShaderManager->setVec3Value(base + "position", glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(base + "ambientColor", glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(base + "diffuseColor", glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(base + "specularColor", glm::vec3(0.0f));
		m_pShaderManager->setFloatValue(base + "focalStrength", 1.0f);  // avoid pow(0,0)
		m_pShaderManager->setFloatValue(base + "specularIntensity", 0.0f);
	}
}