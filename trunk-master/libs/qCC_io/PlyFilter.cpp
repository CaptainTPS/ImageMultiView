//##########################################################################
//#                                                                        #
//#                              CLOUDCOMPARE                              #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 or later of the License.      #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#include "PlyFilter.h"

//Local
#include "PlyOpenDlg.h"

//Qt
#include <QImage>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

//qCC_db
#include <ccLog.h>
#include <ccMesh.h>
#include <ccPointCloud.h>
#include <ccMaterial.h>
#include <ccMaterialSet.h>
#include <ccProgressDialog.h>
#include <ccScalarField.h>

//System
#include <string.h>
#include <assert.h>
#if defined(CC_WINDOWS)
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

using namespace CCLib;

bool PlyFilter::canLoadExtension(QString upperCaseExt) const
{
	return (upperCaseExt == "PLY");
}

bool PlyFilter::canSave(CC_CLASS_ENUM type, bool& multiple, bool& exclusive) const
{
	if (	type == CC_TYPES::MESH
		||	type == CC_TYPES::POINT_CLOUD)
	{
		multiple = false;
		exclusive = true;
		return true;
	}
	return false;
}

static e_ply_storage_mode s_defaultOutputFormat = PLY_DEFAULT;
void PlyFilter::SetDefaultOutputFormat(e_ply_storage_mode format)
{
	s_defaultOutputFormat = format;
}

CC_FILE_ERROR PlyFilter::saveToFile(ccHObject* entity, QString filename, SaveParameters& parameters)
{
	e_ply_storage_mode outputFormat = s_defaultOutputFormat;

	//ask for output format
	if (parameters.alwaysDisplaySaveDialog)
	{
		QMessageBox msgBox(QMessageBox::Question,"Choose output format","Save in BINARY or ASCII format?");
		msgBox.addButton("BINARY", QMessageBox::AcceptRole);
		QPushButton *asciiButton = msgBox.addButton("ASCII", QMessageBox::AcceptRole);
		msgBox.exec();
		outputFormat = msgBox.clickedButton() == asciiButton ? PLY_ASCII : PLY_DEFAULT;
	}

	return saveToFile(entity,filename,outputFormat);
}

CC_FILE_ERROR PlyFilter::saveToFile(ccHObject* entity, QString filename, e_ply_storage_mode storageType)
{
	if (!entity || filename.isEmpty())
		return CC_FERR_BAD_ARGUMENT;

	ccGenericPointCloud* vertices = NULL;
	ccGenericMesh* mesh = NULL;
	if (entity->isKindOf(CC_TYPES::MESH))
	{
		mesh = ccHObjectCaster::ToGenericMesh(entity);
		vertices = mesh->getAssociatedCloud();
	}
	else if (entity->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		vertices = ccHObjectCaster::ToGenericPointCloud(entity);
	}

	if (!vertices)
		return CC_FERR_BAD_ENTITY_TYPE;

	p_ply ply = ply_create(qPrintable(filename), storageType, NULL, 0, NULL);
	if (!ply)
		return CC_FERR_WRITING;

	//Has the cloud been recentered?
	e_ply_type coordType = vertices->isShifted() || sizeof(PointCoordinateType) > 4 ? PLY_DOUBLE : PLY_FLOAT; //we use double coordinates for shifted vertices (i.e. >1e6)

	int result = 1;
	unsigned vertCount = vertices->size();

	//3D points (x,y,z)
	if (ply_add_element(ply, "vertex", vertCount))
	{
		result = ply_add_scalar_property(ply, "x", coordType);
		result = ply_add_scalar_property(ply, "y", coordType);
		result = ply_add_scalar_property(ply, "z", coordType);
	}
	else result = 0;

	ccMaterial::CShared material(0);
	if (mesh && mesh->hasMaterials())
	{
		//look for textures/materials in case there's no color
		//if (!mesh->hasColors())
		{
			unsigned textureCount = 0;
			const ccMaterialSet* materials = mesh->getMaterialSet();
			assert(materials);
			if (materials)
			{
				for (size_t i=0; i < materials->size(); ++i)
				{
					//texture?
					if (!materials->at(i)->getTexture().isNull())
					{
						//save first encountered texture
						if (!material)
							material = materials->at(i);
						++textureCount;
					}
				}
			}

			//one texture: we can save it with the PLY file
			if (textureCount == 1)
			{
				if (materials->size() == 1)
				{
					//just one texture/material --> we can handle it 
				}
				else if (materials->size() > 1)
				{
					if (mesh->hasColors())
					{
						ccLog::Error("PLY files can't handle multiple materials/textures!\nOnly the RGB field will be saved.");
					}
					else
					{
						if (mesh->isA(CC_TYPES::MESH))
						{
							if (QMessageBox::question(	0,
														"Multiple materials, one texture",
														"This mesh has only one texture but multiple materials. PLY files can only handle one texture.\nShall we drop the materials (yes) or convert all materials and texture to per-vertex RGB colors? (no)",
														QMessageBox::Yes | QMessageBox::No,
														QMessageBox::Yes) == QMessageBox::No)
							{
								//we can forget the texture
								material = ccMaterial::CShared(0);
								//try to convert materials to RGB
								if (!static_cast<ccMesh*>(mesh)->convertMaterialsToVertexColors())
								{
									ccLog::Error("Conversion failed! (not enough memory?)");
								}
							}
						}
						else if (mesh->isA(CC_TYPES::SUB_MESH))
						{
							//we can forget the texture
							material = ccMaterial::CShared(0);
							ccLog::Warning("This sub-mesh has one texture and multiple materials. As this is a sub-mesh, we will ignore them...  you should convert the parent mesh textures/materials to RGB colors first");
						}
						else
						{
							assert(false);
						}
					}
				}
				else
				{
					assert(false);
				}
			}
			else if (textureCount > 1) //multiple materials
			{
				assert(materials->size() != 0);
				//we can forget the (first) texture (if any)
				material = ccMaterial::CShared(0);

				if (mesh->hasColors())
				{
					ccLog::Error("PLY files can't handle multiple materials/textures!\nOnly the RGB field will be saved.");
				}
				else
				{
					if (mesh->isA(CC_TYPES::MESH))
					{
						//we ask the user if he wants to convert them to RGB
						if (QMessageBox::question(	0,
													"Multiple textures/materials",
													"PLY files can't handle multiple textures/materials!\nDo you want to convert them to per-vertex RGB colors?",
													QMessageBox::Yes | QMessageBox::No,
													QMessageBox::No ) == QMessageBox::Yes)
						{
							if (!static_cast<ccMesh*>(mesh)->convertMaterialsToVertexColors())
							{
								ccLog::Error("Conversion failed! (not enough memory?)");
							}
						}
					}
					else if (mesh->isA(CC_TYPES::SUB_MESH))
					{
						ccLog::Warning("This sub-mesh has multiple textures/materials. PLY files can't handle them.\nAs this is a sub-mesh, we will have to ignore them... you should convert the parent mesh textures/materials to RGB colors first");
					}
					else
					{
						assert(false);
					}
				}
			}
		}
		//else
		//{
		//	ccLog::Warning("[PLY] PLY files can't handle materials/textures! RGB field will be saved instead");
		//	ccLog::Warning("[PLY] Note: you can convert materials/textures to RGB if necessary (see 'Edit > Mesh' menu)");
		//}
	}

	bool hasUniqueColor = false;
	ColorCompType uniqueColor[3] = {0,0,0};
	if (material)
	{
		//Material without texture?
		if (!material->hasTexture())
		{
			const ccColor::Rgbaf& diffuse = material->getDiffuseFront();
			uniqueColor[0] = static_cast<ColorCompType>(diffuse.r * ccColor::MAX);
			uniqueColor[1] = static_cast<ColorCompType>(diffuse.g * ccColor::MAX);
			uniqueColor[2] = static_cast<ColorCompType>(diffuse.b * ccColor::MAX);
			hasUniqueColor = true;
			material = ccMaterial::CShared(0); //we can forget it!
		}
	}

	//RGB colors
	bool hasColors = vertices->hasColors();
	if (hasColors)
	{
		hasColors = true;
		//if (ply_add_element(ply, "color", vertCount))
		//{
			result = ply_add_scalar_property(ply, "red", PLY_UCHAR);
			result = ply_add_scalar_property(ply, "green", PLY_UCHAR);
			result = ply_add_scalar_property(ply, "blue", PLY_UCHAR);
		//}
		//else result = 0;
	}

	//Normals (nx,ny,nz)
	bool hasNormals = vertices->hasNormals();
	if (hasNormals)
	{
		//if (ply_add_element(ply, "normal", vertCount))
		//{
			e_ply_type normType = (sizeof(PointCoordinateType) > 4 ? PLY_DOUBLE : PLY_FLOAT);
			result = ply_add_scalar_property(ply, "nx", normType);
			result = ply_add_scalar_property(ply, "ny", normType);
			result = ply_add_scalar_property(ply, "nz", normType);
		//}
		//else result = 0;
	}

	//Scalar fields
	std::vector<ccScalarField*> scalarFields;
	if (vertices->isA(CC_TYPES::POINT_CLOUD))
	{
		ccPointCloud* ccCloud = static_cast<ccPointCloud*>(vertices);
		unsigned sfCount = ccCloud->getNumberOfScalarFields();
		if (sfCount)
		{
			e_ply_type scalarType = (sizeof(ScalarType) > 4 ? PLY_DOUBLE : PLY_FLOAT);

			scalarFields.resize(sfCount);
			unsigned unnamedSF = 0;
			for (unsigned i=0; i<sfCount; ++i)
			{
				scalarFields[i] = static_cast<ccScalarField*>(ccCloud->getScalarField(i));
				const char* sfName = scalarFields[i]->getName();
				QString propName;
				if (!sfName)
				{
					if (unnamedSF++ == 0)
						propName = "scalar";
					else
						propName = QString("scalar_%1").arg(unnamedSF);
				}
				else
				{
					propName = QString("scalar_%1").arg(sfName);
					propName.replace(' ','_');
				}

				result = ply_add_scalar_property(ply, qPrintable(propName), scalarType);
			}
		}
	}
	
	//Mesh
	unsigned triNum = 0;
	if (mesh)
	{
		triNum = mesh->size();
		if (triNum>0 && ply_add_element(ply, "face", triNum))
		{
			//DGM: don't change the field name (vertex_indices) as Meshlab
			//only support this one! (grrrrrrrrr)
			result = ply_add_list_property(ply, "vertex_indices", PLY_UCHAR, PLY_INT);

			//texture & texture coordinates?
			if (material)
			{
				assert(material->hasTexture() && mesh->getTexCoordinatesTable());
				QFileInfo fileInfo(material->getTextureFilename());
				QString defaultTextureName = fileInfo.fileName();
				if (fileInfo.suffix().isNull())
					defaultTextureName += QString(".png");
				//try to save the texture!
				QString textureFilePath = QFileInfo(filename).absolutePath() + QString('/') + defaultTextureName;
				if (!material->getTexture().mirrored().save(textureFilePath)) //mirrored --> see ccMaterial
				{
					ccLog::Warning(QString("[PLY] Failed to save texture in '%1'!").arg(textureFilePath));
					material = ccMaterial::CShared(0);
				}
				else
				{
					//save texture filename as a comment!
					result = ply_add_comment(ply,qPrintable(QString("TEXTUREFILE %1").arg(defaultTextureName)));
					//DGM FIXME: is this the right name?
					result = ply_add_list_property(ply, "texcoord", PLY_UCHAR, PLY_FLOAT); //'texcoord' to mimick Photoscan
					
					ccLog::Print(QString("[PLY] Texture file: %1").arg(textureFilePath));
				}
			}
		}
		else result = 0;
	}

	result = ply_add_comment(ply,"Author: CloudCompare (TELECOM PARISTECH/EDF R&D)");
	result = ply_add_obj_info(ply,"Generated by CloudCompare!");

	//try to write header
	result = ply_write_header(ply);
	if (!result)
	{
		ply_close(ply);
		return CC_FERR_WRITING;
	}

	//save the point cloud (=vertices)
	for (unsigned i=0; i<vertCount; ++i)
	{
		const CCVector3* P = vertices->getPoint(i);
		CCVector3d Pglobal = vertices->toGlobal3d<PointCoordinateType>(*P);
		ply_write(ply, Pglobal.x);
		ply_write(ply, Pglobal.y);
		ply_write(ply, Pglobal.z);

		if (hasColors)
		{
			const ColorCompType* col = vertices->getPointColor(i);
			ply_write(ply,static_cast<double>(col[0]));
			ply_write(ply,static_cast<double>(col[1]));
			ply_write(ply,static_cast<double>(col[2]));
		}
		else if (hasUniqueColor)
		{
			ply_write(ply, static_cast<double>(uniqueColor[0]));
			ply_write(ply, static_cast<double>(uniqueColor[1]));
			ply_write(ply, static_cast<double>(uniqueColor[2]));
		}

		if (hasNormals)
		{
			const CCVector3& N = vertices->getPointNormal(i);
			ply_write(ply, static_cast<double>(N.x));
			ply_write(ply, static_cast<double>(N.y));
			ply_write(ply, static_cast<double>(N.z));
		}

		for (std::vector<ccScalarField*>::const_iterator sf = scalarFields.begin(); sf != scalarFields.end(); ++sf)
		{
			ply_write(ply, (*sf)->getGlobalShift() + (*sf)->getValue(i));
		}
	}

	//and the mesh structure
	if (mesh)
	{
		mesh->placeIteratorAtBegining();
		for (unsigned i=0;i<triNum;++i)
		{
			const CCLib::VerticesIndexes* tsi = mesh->getNextTriangleVertIndexes(); //DGM: getNextTriangleVertIndexes is faster for mesh groups!
			ply_write(ply,double(3));
			assert(tsi->i1<vertCount);
			assert(tsi->i2<vertCount);
			assert(tsi->i3<vertCount);
			ply_write(ply,double(tsi->i1));
			ply_write(ply,double(tsi->i2));
			ply_write(ply,double(tsi->i3));

			if (material) //texture coordinates
			{
				ply_write(ply,double(6));
				float *tx1=0,*tx2=0,*tx3=0;
				mesh->getTriangleTexCoordinates(i,tx1,tx2,tx3);
				ply_write(ply,tx1 ? (double)tx1[0] : -1.0);
				ply_write(ply,tx1 ? (double)tx1[1] : -1.0);
				ply_write(ply,tx2 ? (double)tx2[0] : -1.0);
				ply_write(ply,tx2 ? (double)tx2[1] : -1.0);
				ply_write(ply,tx3 ? (double)tx3[0] : -1.0);
				ply_write(ply,tx3 ? (double)tx3[1] : -1.0);
			}
		}
	}

	ply_close(ply);

	return CC_FERR_NO_ERROR;
}

#define PROCESS_EVENTS_FREQ 10000

#define ELEM_POS_0	0x00000000
#define ELEM_POS_1	0x00000001
#define ELEM_POS_2	0x00000002
#define ELEM_POS_3	0x00000003
#define ELEM_EOL	0x00000004

#define POS_MASK	0x00000003

static CCVector3d s_Point(0,0,0);
static int s_PointCount = 0;
static bool s_PointDataCorrupted = false;
static FileIOFilter::LoadParameters s_loadParameters;
static CCVector3d s_Pshift(0,0,0);

static int vertex_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	double val = ply_get_argument_value(argument);

	// This looks like it should always be true, 
	// but it's false if x is NaN.
	if (val == val)
	{
		s_Point.u[flags & POS_MASK] = val;
	}
	else
	{
		//warning: corrupted data!
		s_PointDataCorrupted = true;
		s_Point.u[flags & POS_MASK] = 0;
		//return 0;
	}

	if (flags & ELEM_EOL)
	{
		//first point: check for 'big' coordinates
		if (s_PointCount == 0)
		{
			if (FileIOFilter::HandleGlobalShift(s_Point,s_Pshift,s_loadParameters))
			{
				cloud->setGlobalShift(s_Pshift);
				ccLog::Warning("[PLYFilter::loadFile] Cloud (vertices) has been recentered! Translation: (%.2f,%.2f,%.2f)",s_Pshift.x,s_Pshift.y,s_Pshift.z);
			}
		}

		cloud->addPoint(CCVector3::fromArray((s_Point + s_Pshift).u));
		++s_PointCount;

		s_PointDataCorrupted = false;
		if ((s_PointCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static CCVector3 s_Normal(0,0,0);
static int s_NormalCount = 0;

static int normal_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	s_Normal.u[flags & POS_MASK] = static_cast<PointCoordinateType>( ply_get_argument_value(argument) );

	if (flags & ELEM_EOL)
	{
		cloud->addNorm(s_Normal);
		++s_NormalCount;

		if ((s_NormalCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static ColorCompType s_color[3] = {0,0,0};
static int s_ColorCount = 0;

static int rgb_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	p_ply_property prop;
	ply_get_argument_property(argument, &prop, NULL, NULL);
	e_ply_type type;
	ply_get_property_info(prop, NULL, &type, NULL, NULL);

	switch(type)
	{
	case PLY_FLOAT:
	case PLY_DOUBLE:
	case PLY_FLOAT32:
	case PLY_FLOAT64:
		s_color[flags & POS_MASK] = static_cast<ColorCompType>(std::min(std::max(0.0, ply_get_argument_value(argument)), 1.0) * ccColor::MAX);
		break;
	case PLY_INT8:
	case PLY_UINT8:
	case PLY_CHAR:
	case PLY_UCHAR:
		s_color[flags & POS_MASK] = static_cast<ColorCompType>(ply_get_argument_value(argument));
		break;
	default:
		s_color[flags & POS_MASK] = static_cast<ColorCompType>(ply_get_argument_value(argument));
		break;
	}

	if (flags & ELEM_EOL)
	{
		cloud->addRGBColor(s_color);
		++s_ColorCount;

		if ((s_ColorCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static int s_IntensityCount = 0;
static int grey_cb(p_ply_argument argument)
{
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), NULL);

	p_ply_property prop;
	ply_get_argument_property(argument, &prop, NULL, NULL);
	e_ply_type type;
	ply_get_property_info(prop, NULL, &type, NULL, NULL);

	ColorCompType G;

	switch(type)
	{
	case PLY_FLOAT:
	case PLY_DOUBLE:
	case PLY_FLOAT32:
	case PLY_FLOAT64:
		G = static_cast<ColorCompType>(std::min(std::max(0.0, ply_get_argument_value(argument)), 1.0) * ccColor::MAX);
		break;
	case PLY_INT8:
	case PLY_UINT8:
	case PLY_CHAR:
	case PLY_UCHAR:
		G = static_cast<ColorCompType>(ply_get_argument_value(argument));
		break;
	default:
		G = static_cast<ColorCompType>(ply_get_argument_value(argument));
		break;
	}

	cloud->addGreyColor(G);
	++s_IntensityCount;

	if ((s_IntensityCount % PROCESS_EVENTS_FREQ) == 0)
		QCoreApplication::processEvents();

	return 1;
}

static unsigned s_totalScalarCount = 0;
static int scalar_cb(p_ply_argument argument)
{
	CCLib::ScalarField* sf = 0;
	ply_get_argument_user_data(argument, (void**)(&sf), NULL);

	p_ply_element element;
	long instance_index;
	ply_get_argument_element(argument, &element, &instance_index);

	ScalarType scal = static_cast<ScalarType>(ply_get_argument_value(argument));
	sf->setValue(instance_index,scal);

	if ((++s_totalScalarCount % PROCESS_EVENTS_FREQ) == 0)
		QCoreApplication::processEvents();

	return 1;
}

static unsigned s_tri[3];
static unsigned s_triCount = 0;
static bool s_unsupportedPolygonType = false;
static int face_cb(p_ply_argument argument)
{
	ccMesh* mesh=0;
	ply_get_argument_user_data(argument, (void**)(&mesh), NULL);
	assert(mesh);
	if (!mesh)
		return 1;

	long length, value_index;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	//unsupported polygon type!
	if (length != 3)
	{
		s_unsupportedPolygonType = true;
		return 1;
	}
	if (value_index < 0 || value_index > 2)
		return 1;

	s_tri[value_index] = static_cast<unsigned>(ply_get_argument_value(argument));

	if (value_index == 2)
	{
		mesh->addTriangle(s_tri[0], s_tri[1], s_tri[2]);
		++s_triCount;

		if ((s_triCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static float s_texCoord[6];
static unsigned s_texCoordCount = 0;
static bool s_invalidTexCoordinates = false;
static int texCoords_cb(p_ply_argument argument)
{
	TextureCoordsContainer* texCoords = 0;
	ply_get_argument_user_data(argument, (void**)(&texCoords), NULL);
	assert(texCoords);
	if (!texCoords)
		return 1;

	long length, value_index;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	//unsupported/invalid coordinates!
	if (length != 6)
	{
		s_invalidTexCoordinates = true;
		return 1;
	}
	if (value_index < 0 || value_index > 5)
		return 1;

	s_texCoord[value_index] = static_cast<float>(ply_get_argument_value(argument));

	if (((value_index+1) % 2) == 0)
	{
		texCoords->addElement(s_texCoord + value_index - 1);
		++s_texCoordCount;

		if ((s_texCoordCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static int s_maxTextureIndex = -1;
static int texIndexes_cb(p_ply_argument argument)
{
	ccMesh::triangleMaterialIndexesSet* texIndexes = 0;
	ply_get_argument_user_data(argument, (void**)(&texIndexes), NULL);
	assert(texIndexes);
	if (!texIndexes)
		return 1;

	p_ply_element element;
	long instance_index;
	ply_get_argument_element(argument, &element, &instance_index);

	int index = static_cast<int>(ply_get_argument_value(argument));
	if (index > s_maxTextureIndex)
	{
		s_maxTextureIndex = -1;
	}
	texIndexes->addElement(index);

	if ((texIndexes->currentSize() % PROCESS_EVENTS_FREQ) == 0)
		QCoreApplication::processEvents();

	return 1;
}


CC_FILE_ERROR PlyFilter::loadFile(QString filename, ccHObject& container, LoadParameters& parameters)
{
	return loadFile(filename, QString(), container, parameters);
}

CC_FILE_ERROR PlyFilter::loadFile(QString filename, QString inputTextureFilename, ccHObject& container, LoadParameters& parameters)
{
	//reset statics!
	s_triCount = 0;
	s_unsupportedPolygonType = false;
	s_texCoordCount = 0;
	s_invalidTexCoordinates = false;
	s_totalScalarCount = 0;
	s_IntensityCount = 0;
	s_ColorCount = 0;
	s_NormalCount = 0;
	s_PointCount = 0;
	s_PointDataCorrupted = false;
	s_loadParameters = parameters;
	s_Pshift = CCVector3d(0,0,0);

	/****************/
	/***  Header  ***/
	/****************/

	//open a PLY file for reading
	p_ply ply = ply_open(qPrintable(filename), NULL, 0, NULL);
	if (!ply)
		return CC_FERR_READING;

	ccLog::PrintDebug(QString("[PLY] Opening file '%1' ...").arg(filename));

	if (!ply_read_header(ply))
	{
		ply_close(ply);
		return CC_FERR_WRONG_FILE_TYPE;
	}

	//storage mode: little/big endian
	e_ply_storage_mode storage_mode;
	get_plystorage_mode(ply, &storage_mode);

	/*****************/
	/***  Texture  ***/
	/*****************/
	//eventual texture files declared in the comments (keyword: TEXTUREFILE)
	QStringList textureFileNames;
	//texture coordinates
	TextureCoordsContainer* texCoords = 0;
	//texture indexes
	ccMesh::triangleMaterialIndexesSet* texIndexes = 0;

	/******************/
	/***  Comments  ***/
	/******************/
	{
		//display comments
		const char* lastComment = NULL;
		while ((lastComment = ply_get_next_comment(ply, lastComment)))
		{
			ccLog::Print("[PLY][Comment] %s", lastComment);

			//specific case: TextureFile 'filename.ext'
			if (QString(lastComment).toUpper().startsWith("TEXTUREFILE "))
			{
				textureFileNames << QString(lastComment).mid(12).trimmed();
			}
		}
	}

	//external texture filename?
	if (!inputTextureFilename.isEmpty())
	{
		//add it to the set of textures (if it's not already there!)
		if (!textureFileNames.contains(inputTextureFilename))
		{
			textureFileNames.push_back(inputTextureFilename);
		}
	}

	/*******************************/
	/***  Elements & properties  ***/
	/*******************************/

	//Point-based elements (points, colors, normals, etc.)
	std::vector<plyElement> pointElements;
	//Mesh-based elements (vertices, etc.)
	std::vector<plyElement> meshElements;

	//Point-based element properties (coordinates, color components, etc.)
	std::vector<plyProperty> stdProperties;
	//Mesh-based multi-element properties (vertex indexes, etc.)
	std::vector<plyProperty> listProperties;
	//Mesh-based single-element properties (texture index, etc.)
	std::vector<plyProperty> singleProperties;

	try
	{
		//last read element
		plyElement lastElement;
		lastElement.elem = 0;
		while ((lastElement.elem = ply_get_next_element(ply, lastElement.elem)))
		{
			//we get next element info
			ply_get_element_info(lastElement.elem, &lastElement.elementName, &lastElement.elementInstances);

			if (lastElement.elementInstances == 0)
			{
				ccLog::Warning("[PLY] Element '%s' was ignored as it has 0 instance!", lastElement.elementName);
				continue;
			}

			lastElement.properties.clear();
			lastElement.propertiesCount = 0;
			lastElement.isFace = false;
			//printf("Element: %s\n",lastElement.elementName);

			//last read property
			plyProperty lastProperty;
			lastProperty.prop = 0;
			lastProperty.elemIndex = 0;

			while ((lastProperty.prop = ply_get_next_property(lastElement.elem,lastProperty.prop)))
			{
				//we get next property info
				ply_get_property_info(lastProperty.prop, &lastProperty.propName, &lastProperty.type, &lastProperty.length_type, &lastProperty.value_type);
				//printf("\tProperty: %s (%s)\n",lastProperty.propName,e_ply_type_names[lastProperty.type]);

				if (lastProperty.type == 16) //PLY_LIST
				{
					lastElement.isFace = true;
				}

				lastElement.properties.push_back(lastProperty);
				++lastElement.propertiesCount;
			}

			//if we have a "face-like" element
			if (lastElement.isFace)
			{
				//we store its properties in 'listProperties'
				for (size_t i=0; i<lastElement.properties.size(); ++i)
				{
					plyProperty& prop = lastElement.properties[i];
					prop.elemIndex = static_cast<int>(meshElements.size());

					if (prop.type == 16)
					{
						//multiple elements per face (vertex indexes, texture coordinates, etc.)
						listProperties.push_back(prop);
					}
					else
					{
						//single element per face (texture index, etc.)
						singleProperties.push_back(prop);
					}
				}
				meshElements.push_back(lastElement);
			}
			else //else if we have a "point-like" element
			{
				//we store its properties in 'stdProperties'
				for (size_t i=0; i<lastElement.properties.size(); ++i)
				{
					plyProperty& prop = lastElement.properties[i];
					prop.elemIndex = (int)pointElements.size();
					stdProperties.push_back(prop);
				}
				pointElements.push_back(lastElement);
			}
		}
	}
	catch (const std::bad_alloc&)
	{
		//not enough memory
		return CC_FERR_NOT_ENOUGH_MEMORY;
	}

	//We need some points at least!
	if (pointElements.empty())
	{
		ply_close(ply);
		return CC_FERR_NO_LOAD;
	}

	/**********************/
	/***  Objects info  ***/
	/**********************/
	{
		const char* lastObjInfo = NULL;
		while ((lastObjInfo = ply_get_next_obj_info(ply, lastObjInfo)))
		{
			ccLog::Print("[PLY][Info] %s",lastObjInfo);
		}
	}

	/****************/
	/***  Dialog  ***/
	/****************/

	//properties indexes (0 = unassigned)
	static const unsigned nStdProp = 10;
	int stdPropIndexes[nStdProp] = {0,0,0,0,0,0,0,0,0,0};
	int& xIndex = stdPropIndexes[0];
	int& yIndex = stdPropIndexes[1];
	int& zIndex = stdPropIndexes[2];
	int& nxIndex = stdPropIndexes[3];
	int& nyIndex = stdPropIndexes[4];
	int& nzIndex = stdPropIndexes[5];
	int& rIndex = stdPropIndexes[6];
	int& gIndex = stdPropIndexes[7];
	int& bIndex = stdPropIndexes[8];
	int& iIndex = stdPropIndexes[9];

	std::vector<int> sfPropIndexes;
	//int& sfIndex = stdPropIndexes[10];

	static const unsigned nListProp = 2;
	int listPropIndexes[nListProp] = {0,0};
	int& facesIndex = listPropIndexes[0];
	int& texCoordsIndex = listPropIndexes[1];

	static const unsigned nSingleProp = 1;
	int singlePropIndexes[nSingleProp] = {0};
	int& texNumberIndex = singlePropIndexes[0];

	//Combo box items for standard properties (coordinates, color components, etc.)
	QStringList stdPropsText;
	stdPropsText << QString("None");
	{
		for (int i=1; i<=static_cast<int>(stdProperties.size()); ++i)
		{
			plyProperty& pp = stdProperties[i-1];
			QString itemText = QString("%1 - %2 [%3]").arg(pointElements[pp.elemIndex].elementName).arg(pp.propName).arg(e_ply_type_names[pp.type]);
			assert(pp.type!=16); //we don't want any PLY_LIST here
			stdPropsText << itemText;

			QString elementName = QString(pointElements[pp.elemIndex].elementName).toUpper();
			QString propName = QString(pp.propName).toUpper();

			if (nxIndex == 0 && (propName.contains("NX") || (elementName.contains("NORM") && propName.endsWith("X"))))
				nxIndex = i;
			else if (nyIndex == 0 && (propName.contains("NY") || (elementName.contains("NORM") && propName.endsWith("Y"))))
				nyIndex = i;
			else if (nzIndex == 0 && (propName.contains("NZ") || (elementName.contains("NORM") && propName.endsWith("Z"))))
				nzIndex = i;
			else if (rIndex == 0 && (propName.contains("RED") || (elementName.contains("COL") && propName.endsWith("R"))))
				rIndex = i;
			else if (gIndex == 0 && (propName.contains("GREEN") || (elementName.contains("COL") && propName.endsWith("G"))))
				gIndex = i;
			else if (bIndex == 0 && (propName.contains("BLUE") || (elementName.contains("COL") && propName.endsWith("B"))))
				bIndex = i;
			else if (iIndex == 0 && (propName.contains("INTENSITY") || propName.contains("GRAY") || propName.contains("GREY") || (elementName.contains("COL") && propName.endsWith("I"))))
				//iIndex = i; //DGM: we don't load the intensities as RGB colors anymore!
				sfPropIndexes.push_back(i);
			else if (elementName.contains("VERT") || elementName.contains("POINT"))
			{
				if (propName.contains("SCAL"))
					sfPropIndexes.push_back(i);
				else if (xIndex == 0 && propName.endsWith("X"))
					xIndex = i;
				else if (yIndex == 0 && propName.endsWith("Y"))
					yIndex = i;
				else if (zIndex == 0 && propName.endsWith("Z"))
					zIndex = i;
			}
			else if (propName.contains("SCAL") || propName.contains("VAL"))
				sfPropIndexes.push_back(i);
		}
	}

	//Combo box items for 'list' properties (vertex indexes, etc.)
	QStringList listPropsText;
	{
		listPropsText << QString("None");
		for (int i = 0; i < static_cast<int>(listProperties.size()); ++i)
		{
			plyProperty& pp = listProperties[i];
			QString itemText = QString("%0 - %1 [%2]").arg(meshElements[pp.elemIndex].elementName).arg(pp.propName).arg(e_ply_type_names[pp.type]);
			assert(pp.type==16); //we only want PLY_LIST here
			listPropsText << itemText;

			QString elementName = QString(meshElements[pp.elemIndex].elementName).toUpper();
			QString propName = QString(pp.propName).toUpper();

			if (elementName.contains("FACE") || elementName.contains("TRI"))
			{
				if (facesIndex == 0 && propName.contains("IND"))
					facesIndex = i+1;
				if (texCoordsIndex == 0 && propName.contains("COORD") && !textureFileNames.isEmpty()) //no need to assign this value if we don't have any texture!
					texCoordsIndex = i+1;
			}
		}
	}

	//Combo box items for 'single' properties (texture index, etc.)
	QStringList singlePropsText;
	{
		singlePropsText << QString("None");
		for (int i = 0; i < static_cast<int>(singleProperties.size()); ++i)
		{
			plyProperty& pp = singleProperties[i];
			QString itemText = QString("%0 - %1 [%2]").arg(meshElements[pp.elemIndex].elementName).arg(pp.propName).arg(e_ply_type_names[pp.type]);
			singlePropsText << itemText;

			QString elementName = QString(meshElements[pp.elemIndex].elementName).toUpper();
			QString propName = QString(pp.propName).toUpper();

			
			if (elementName.contains("FACE") || elementName.contains("TRI"))
			{
				if (texNumberIndex == 0 && propName.contains("TEXNUMBER") && !textureFileNames.isEmpty()) //no need to assign this value if we don't have any texture!
					texNumberIndex = i+1;
			}
		}
	}

	//combo-box max visible items
	int stdPropsCount = stdPropsText.count();
	int listPropsCount = listPropsText.count();
	int singlePropsCount = singlePropsText.count();

	//we need at least 2 coordinates!
	if (stdPropsCount < 2)
	{
		ccLog::Warning("[PLY] This ply file has less than 2 properties defined! (not even X and Y ;)");
		return CC_FERR_MALFORMED_FILE;
	}
	else if (stdPropsCount < 4 && !parameters.alwaysDisplayLoadDialog)
	{
		//brute force heuristic
		//(the first element is always 'None')
		xIndex = 1;
		yIndex = 2;
		zIndex = (stdPropsCount > 3 ? 3 : 0);
		facesIndex = (listPropsCount > 1 ? 1 : 0);
		texNumberIndex = (singlePropsCount > 1 ? 1 : 0);
	}
	else
	{
		//we count all assigned properties
		int assignedStdProperties = 0;
		{
			for (unsigned i = 0; i < nStdProp; ++i)
				if (stdPropIndexes[i] > 0)
					++assignedStdProperties;
		}

		int assignedListProperties = 0;
		{
			for (unsigned i = 0; i < nListProp; ++i)
				if (listPropIndexes[i] > 0)
					++assignedListProperties;
		}

		int assignedSingleProperties = 0;
		{
			for (unsigned i = 0; i < nSingleProp; ++i)
				if (singlePropIndexes[i] > 0)
					++assignedSingleProperties;
		}

		if (	parameters.alwaysDisplayLoadDialog
			||	stdPropsCount > assignedStdProperties+1		//+1 because of the first item in the combo box ('none')
			||	listPropsCount > assignedListProperties+1
			||	singlePropsCount > assignedSingleProperties+1)
		{
			PlyOpenDlg pod/*(MainWindow::TheInstance())*/;

			pod.plyTypeEdit->setText(e_ply_storage_mode_names[storage_mode]);
			pod.elementsEdit->setText(QString::number(pointElements.size()));
			pod.propertiesEdit->setText(QString::number(listProperties.size() + stdProperties.size() + singleProperties.size()));
			pod.textureCountEdit->setText(QString::number(textureFileNames.size()));

			//we fill all combo-boxes with all items
			pod.setDefaultComboItems(stdPropsText);
			pod.setListComboItems(listPropsText);
			pod.setSingleComboItems(singlePropsText);

			//try to restore previous context (if any)
			bool hasAPreviousContext = false;
			if (!pod.restorePreviousContext(hasAPreviousContext))
			{
				if (hasAPreviousContext)
					ccLog::Warning("[PLY] Too many differences with the previous file, we reset the dialog.");
				//Set default/guessed element
				pod.xComboBox->setCurrentIndex(xIndex);
				pod.yComboBox->setCurrentIndex(yIndex);
				pod.zComboBox->setCurrentIndex(zIndex);

				pod.rComboBox->setCurrentIndex(rIndex);
				pod.gComboBox->setCurrentIndex(gIndex);
				pod.bComboBox->setCurrentIndex(bIndex);

				pod.iComboBox->setCurrentIndex(iIndex);

				pod.sfComboBox->setCurrentIndex(sfPropIndexes.empty() ? 0 : sfPropIndexes.front());
				for (size_t j=1; j<sfPropIndexes.size(); ++j)
					pod.addSFComboBox(sfPropIndexes[j]);

				pod.nxComboBox->setCurrentIndex(nxIndex);
				pod.nyComboBox->setCurrentIndex(nyIndex);
				pod.nzComboBox->setCurrentIndex(nzIndex);

				pod.facesComboBox->setCurrentIndex(facesIndex);
				pod.textCoordsComboBox->setCurrentIndex(texCoordsIndex);
				pod.texIndexComboBox->setCurrentIndex(texNumberIndex);
			}

			//We show the dialog (or we try to skip it ;)
			if (parameters.alwaysDisplayLoadDialog
				&& !pod.canBeSkipped()
				&& !pod.exec())
			{
				ply_close(ply);
				return CC_FERR_CANCELED_BY_USER;
			}

			//Force events processing (to hide dialog)
			QCoreApplication::processEvents();

			xIndex = pod.xComboBox->currentIndex();
			yIndex = pod.yComboBox->currentIndex();
			zIndex = pod.zComboBox->currentIndex();
			nxIndex = pod.nxComboBox->currentIndex();
			nyIndex = pod.nyComboBox->currentIndex();
			nzIndex = pod.nzComboBox->currentIndex();
			rIndex = pod.rComboBox->currentIndex();
			gIndex = pod.gComboBox->currentIndex();
			bIndex = pod.bComboBox->currentIndex();
			iIndex = pod.iComboBox->currentIndex();
			facesIndex = pod.facesComboBox->currentIndex();
			texCoordsIndex = pod.textCoordsComboBox->currentIndex();
			texNumberIndex = pod.texIndexComboBox->currentIndex();

			//get (non null) SF properties
			sfPropIndexes.clear();
			{
				for (size_t j=0; j<pod.m_sfCombos.size(); ++j)
					if (pod.m_sfCombos[j]->currentIndex() > 0)
						sfPropIndexes.push_back(pod.m_sfCombos[j]->currentIndex());
			}
		}
	}

	/*************************/
	/***  Callbacks setup  ***/
	/*************************/

	//Main point cloud
	ccPointCloud* cloud = new ccPointCloud("unnamed - Cloud");

	/* POINTS (X,Y,Z) */

	unsigned numberOfPoints = 0;

	assert(xIndex != yIndex && xIndex != zIndex && yIndex != zIndex);

	//POINTS (X)
	if (xIndex > 0)
	{
		long flags = ELEM_POS_0; //X coordinate
		if (xIndex > yIndex && xIndex > zIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[xIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	//POINTS (Y)
	if (yIndex > 0)
	{
		long flags = ELEM_POS_1; //Y coordinate
		if (yIndex > xIndex && yIndex > zIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[yIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		if (numberOfPoints > 0)
		{
			if ((long)numberOfPoints != pointElements[pp.elemIndex].elementInstances)
			{
				ccLog::Warning("[PLY] Bad/uncompatible assignation of point properties!");
				delete cloud;
				ply_close(ply);
				return CC_FERR_BAD_ENTITY_TYPE;
			}
		}
		else numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	//POINTS (Z)
	if (zIndex > 0)
	{
		long flags = ELEM_POS_2; //Z coordinate
		if (zIndex > xIndex && zIndex > yIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[zIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		if (numberOfPoints > 0)
		{
			if (static_cast<long>(numberOfPoints) != pointElements[pp.elemIndex].elementInstances)
			{
				ccLog::Warning("[PLY] Bad/uncompatible assignation of point properties!");
				delete cloud;
				ply_close(ply);
				return CC_FERR_BAD_ENTITY_TYPE;
			}
		}
		else numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	if (numberOfPoints == 0 || !cloud->reserveThePointsTable(numberOfPoints))
	{
		delete cloud;
		ply_close(ply);
		return CC_FERR_NOT_ENOUGH_MEMORY;
	}

	/* NORMALS (X,Y,Z) */

	unsigned numberOfNormals = 0;

	assert(nxIndex == 0 || (nxIndex != nyIndex && nxIndex != nzIndex));
	assert(nyIndex == 0 || (nyIndex != nxIndex && nyIndex != nzIndex));
	assert(nzIndex == 0 || (nzIndex != nxIndex && nzIndex != nyIndex));

	//NORMALS (X)
	if (nxIndex > 0)
	{
		long flags = ELEM_POS_0; //Nx
		if (nxIndex > nyIndex && nxIndex > nzIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nxIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = pointElements[pp.elemIndex].elementInstances;
	}

	//NORMALS (Y)
	if (nyIndex > 0)
	{
		long flags = ELEM_POS_1; //Ny
		if (nyIndex > nxIndex && nyIndex > nzIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nyIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = std::max(numberOfNormals, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	//NORMALS (Z)
	if (nzIndex > 0)
	{
		long flags = ELEM_POS_2; //Nz
		if (nzIndex > nxIndex && nzIndex > nyIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nzIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = std::max(numberOfNormals, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	//We check that the number of normals corresponds to the number of points
	if (numberOfNormals > 0)
	{
		if (numberOfPoints != numberOfNormals)
		{
			ccLog::Warning("[PLY] The number of normals doesn't match the number of points!");
			delete cloud;
			ply_close(ply);
			return CC_FERR_BAD_ENTITY_TYPE;
		}
		if (!cloud->reserveTheNormsTable())
		{
			delete cloud;
			ply_close(ply);
			return CC_FERR_NOT_ENOUGH_MEMORY;
		}
		cloud->showNormals(true);
	}

	/* COLORS (R,G,B) */

	unsigned numberOfColors = 0;

	assert(rIndex == 0 || (rIndex != gIndex && rIndex != bIndex));
	assert(gIndex == 0 || (gIndex != rIndex && gIndex != bIndex));
	assert(bIndex == 0 || (bIndex != rIndex && bIndex != gIndex));

	if (rIndex > 0)
	{
		long flags = ELEM_POS_0; //R
		if (rIndex > gIndex && rIndex > bIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[rIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = pointElements[pp.elemIndex].elementInstances;
	}

	if (gIndex > 0)
	{
		long flags = ELEM_POS_1; //G
		if (gIndex > rIndex && gIndex > bIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[gIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = std::max(numberOfColors, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	if (bIndex > 0)
	{
		long flags = ELEM_POS_2; //B
		if (bIndex > rIndex && bIndex > gIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[bIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = std::max(numberOfColors, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	/* Intensity (I) */

	//INTENSITE (G)
	if (iIndex > 0)
	{
		if (numberOfColors > 0)
		{
			ccLog::Error("Can't import colors AND intensity (intensities will be ignored)!");
			ccLog::Warning("[PLY] intensities will be ignored");
		}
		else
		{
			plyProperty pp = stdProperties[iIndex-1];
			ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, grey_cb, cloud, 0);

			numberOfColors = pointElements[pp.elemIndex].elementInstances;
		}
	}

	//We check that the number of colors corresponds to the number of points
	if (numberOfColors > 0)
	{
		if (numberOfPoints != numberOfColors)
		{
			ccLog::Warning("The number of colors doesn't match the number of points!");
			delete cloud;
			ply_close(ply);
			return CC_FERR_BAD_ENTITY_TYPE;
		}
		if (!cloud->reserveTheRGBTable())
		{
			delete cloud;
			ply_close(ply);
			return CC_FERR_NOT_ENOUGH_MEMORY;
		}
		cloud->showColors(true);
	}

	/* SCALAR FIELDS (SF) */
	{
		for (size_t i=0; i<sfPropIndexes.size(); ++i)
		{
			int sfIndex = sfPropIndexes[i];
			plyProperty& pp = stdProperties[sfIndex-1];
			
			unsigned numberOfScalars = pointElements[pp.elemIndex].elementInstances;

			//does the number of scalars matches the number of points?
			if (numberOfPoints != numberOfScalars)
			{
				ccLog::Error(QString("Scalar field #%1: the number of scalars doesn't match the number of points (they will be ignored)!").arg(i+1));
				ccLog::Warning(QString("[PLY] Scalar field #%1 ignored!").arg(i+1));
				numberOfScalars = 0;
			}
			else 
			{
				QString qPropName(pp.propName);
				if (qPropName.startsWith("scalar_") && qPropName.length() > 7)
				{
					//remove the 'scalar_' prefix added when saving SF with CC!
					qPropName = qPropName.mid(7).replace('_',' ');
				}

				int sfIdx = cloud->addScalarField(qPrintable(qPropName));
				if (sfIdx >= 0)
				{
					CCLib::ScalarField* sf = cloud->getScalarField(sfIdx);
					assert(sf);
					if (sf->resize(numberOfScalars))
					{
						ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, scalar_cb, sf, 1);
					}
					else
					{
						cloud->deleteScalarField(sfIdx);
						sfIdx = -1;
					}
				}
				
				if (sfIdx < 0)
				{
					ccLog::Error(QString("Scalar field #%1: not enough memory to load scalar field (they will be ignored)!").arg(i+1));
					ccLog::Warning(QString("[PLY] Scalar field #%1 ignored!").arg(i+1));
				}
			}
		}
	}

	/* MESH FACETS (TRI) */

	ccMesh* mesh = 0;
	unsigned numberOfFacets = 0;

	if (facesIndex > 0)
	{
		plyProperty& pp = listProperties[facesIndex-1];
		assert(pp.type == 16); //we only accept PLY_LIST here!

		mesh = new ccMesh(cloud);

		numberOfFacets = meshElements[pp.elemIndex].elementInstances;

		if (!mesh->reserve(numberOfFacets))
		{
			ccLog::Error("Not enough memory to load facets (they will be ignored)!");
			ccLog::Warning("[PLY] Mesh ignored!");
			delete mesh;
			mesh = 0;
			numberOfFacets = 0;
		}
		else
		{
			ply_set_read_cb(ply, meshElements[pp.elemIndex].elementName, pp.propName, face_cb, mesh, 0);
		}
	}

	if (texCoordsIndex > 0)
	{
		plyProperty& pp = listProperties[texCoordsIndex-1];
		assert(pp.type == 16); //we only accept PLY_LIST here!

		texCoords = new TextureCoordsContainer();
		texCoords->link();

		long numberOfCoordinates = meshElements[pp.elemIndex].elementInstances;
		assert(numberOfCoordinates == numberOfFacets);

		if (!texCoords->reserve(numberOfCoordinates*3))
		{
			ccLog::Error("Not enough memory to load texture coordinates (they will be ignored)!");
			ccLog::Warning("[PLY] Texture coordinates ignored!");
			texCoords->release();
			texCoords = 0;
		}
		else
		{
			ply_set_read_cb(ply, meshElements[pp.elemIndex].elementName, pp.propName, texCoords_cb, texCoords, 0);
		}
	}

	if (texNumberIndex > 0)
	{
		plyProperty& pp = singleProperties[texNumberIndex-1];

		texIndexes = new ccMesh::triangleMaterialIndexesSet();
		texIndexes->link();

		long numberOfCoordinates = meshElements[pp.elemIndex].elementInstances;
		assert(numberOfCoordinates == numberOfFacets);

		if (!texIndexes->reserve(numberOfCoordinates))
		{
			ccLog::Error("Not enough memory to load texture indexes (they will be ignored)!");
			ccLog::Warning("[PLY] Texture indexes ignored!");
			texIndexes->release();
			texIndexes = 0;
		}
		else
		{
			s_maxTextureIndex = textureFileNames.size()-1;
			ply_set_read_cb(ply, meshElements[pp.elemIndex].elementName, pp.propName, texIndexes_cb, texIndexes, 0);
		}
	}

	ccProgressDialog* pDlg = 0;
	if (parameters.parentWidget)
	{
		pDlg = new ccProgressDialog(false, parameters.parentWidget);
		pDlg->setInfo(QObject::tr("Loading in progress..."));
		pDlg->setMethodTitle(QObject::tr("PLY file"));
		pDlg->setRange(0, 0);
		pDlg->start();
		QApplication::processEvents();
	}

	//let 'Rply' do the job;)
	int success = 0;
	try
	{
		success = ply_read(ply);
	}
	catch(...)
	{
		success = -1;
	}

	ply_close(ply);

	if (pDlg)
	{
		delete pDlg;
		pDlg = 0;
	}

	if (success < 1)
	{
		if (mesh)
			delete mesh;
		delete cloud;
		return CC_FERR_READING;
	}

	//we check mesh
	if (mesh)
	{
		if (mesh->size() == 0)
		{
			if (s_unsupportedPolygonType)
			{
				ccLog::Error("Mesh is not triangular! (unsupported)");
			}
			else
			{
				ccLog::Error("Mesh is empty!");
			}
			delete mesh;
			mesh = 0;
		}
		else
		{
			if (s_unsupportedPolygonType)
			{
				ccLog::Error("Some facets are not triangular! (unsupported)");
			}
		}
	}

	if (texCoords && (s_invalidTexCoordinates || s_texCoordCount != 3*mesh->size()))
	{
		ccLog::Error("Invalid texture coordinates! (they will be ignored)");
		texCoords->release();
		texCoords = 0;
	}

	if (texIndexes)
	{
		if (texIndexes->currentSize() < mesh->size())
		{
			ccLog::Error("Invalid texture indexes! (they will be ignored)");
			texIndexes->release();
			texIndexes = 0;
		}

		if (!texCoords)
		{
			ccLog::Error("No texture coordinates were loaded (texture indexes will be ignored)");
			texIndexes->release();
			texIndexes = 0;
		}
	}

	//we save parameters
	parameters = s_loadParameters;

	//we update the scalar field(s)
	{
		for (unsigned i=0; i<cloud->getNumberOfScalarFields(); ++i)
		{
			CCLib::ScalarField* sf = cloud->getScalarField(i);
			assert(sf);
			sf->computeMinAndMax();
			if (i == 0)
			{
				cloud->setCurrentDisplayedScalarField(0);
				cloud->showSF(true);
			}
		}
	}

	if (mesh)
	{
		assert(s_triCount > 0);
		//check number of loaded facets against 'theoretical' number
		if (s_triCount < numberOfFacets)
		{
			mesh->resize(s_triCount);
			ccLog::Warning("[PLY] Some facets couldn't be loaded!");
		}

		//check that vertex indices start at 0
		unsigned minVertIndex = numberOfPoints, maxVertIndex = 0;
		for (unsigned i = 0; i < s_triCount; ++i)
		{
			const CCLib::VerticesIndexes* tri = mesh->getTriangleVertIndexes(i);
			if (tri->i1 < minVertIndex)
				minVertIndex = tri->i1;
			else if (tri->i1 > maxVertIndex)
				maxVertIndex = tri->i1;
			if (tri->i2 < minVertIndex)
				minVertIndex = tri->i2;
			else if (tri->i2 > maxVertIndex)
				maxVertIndex = tri->i2;
			if (tri->i3 < minVertIndex)
				minVertIndex = tri->i3;
			else if (tri->i3 > maxVertIndex)
				maxVertIndex = tri->i3;
		}

		if (maxVertIndex >= numberOfPoints)
		{
			if (maxVertIndex == numberOfPoints && minVertIndex > 0)
			{
				ccLog::Warning("[PLY] Vertex indexes seem to be shifted (+1)! We will try to 'unshift' indices (otherwise file is corrupted...)");
				for (unsigned i=0;i<s_triCount;++i)
				{
					CCLib::VerticesIndexes* tri = mesh->getTriangleVertIndexes(i);
					--tri->i1;
					--tri->i2;
					--tri->i3;
				}
			}
			else //file is definitely corrupted!
			{
				ccLog::Warning("[PLY] Invalid vertex indices!");
				delete mesh;
				delete cloud;
				return CC_FERR_MALFORMED_FILE;
			}
		}

		mesh->addChild(cloud);
		cloud->setEnabled(false);
		cloud->setName("Vertices");
		//cloud->setLocked(true); //DGM: no need to lock it as it is only used by one mesh!

		//associated texture
		if (texCoords)
		{
			ccMaterialSet* materials = 0;
			if (!textureFileNames.isEmpty())
			{
				//try to load the materials
				materials = new ccMaterialSet("materials");

				QString texturePath = QFileInfo(filename).absolutePath() + QString('/');
				for (int ti=0; ti<textureFileNames.size(); ++ti)
				{
					QString textureFileName = textureFileNames[ti];
					QString textureFilePath = texturePath + textureFileName;
					ccMaterial::Shared material(new ccMaterial(textureFileName));
					if (material->loadAndSetTexture(textureFilePath))
					{
						const QImage texture = material->getTexture();
						ccLog::Print(QString("[PLY][Texture] Successfully loaded texture '%1' (%2x%3 pixels)").arg(textureFileName).arg(texture.width()).arg(texture.height()));
						material->setDiffuse(ccColor::bright);
						material->setSpecular(ccColor::darker);
						material->setAmbient(ccColor::darker);
						materials->push_back(material);
					}
					else
					{
						ccLog::Warning(QString("[PLY][Texture] Failed to load texture '%1'").arg(textureFilePath));
					}
				}

				if (materials->empty())
				{
					materials->release();
					materials = 0;
				}
			}
			else
			{
				ccLog::Warning("[PLY][Texture] Texture coordinates loaded without an associated image! (we look for the 'TextureFile' keyword in comments)");
			}

			if (materials)
			{
				//reserve memory for per triangle texture coordinates and optionally for material (texture) indexes
				if (mesh->reservePerTriangleTexCoordIndexes() && (texIndexes || mesh->reservePerTriangleMtlIndexes()))
				{
					mesh->setMaterialSet(materials);
					mesh->setTexCoordinatesTable(texCoords);
					if (texIndexes)
						mesh->setTriangleMtlIndexesTable(texIndexes);
					//generate simple per-triangle texture coordinates indexes
					for (unsigned i=0; i<mesh->size(); ++i)
					{
						if (!texIndexes)
							mesh->addTriangleMtlIndex(0);
						mesh->addTriangleTexCoordIndexes(i*3,i*3+1,i*3+2);
					}
					mesh->showMaterials(true);
				}
				else
				{
					ccLog::Warning("[PLY][Texture] Failed to reserve per-triangle texture coordinates! (not enough memory?)");
					materials->release();
					materials = 0;
				}
			}

			if (!materials)
			{
				//something went bad ;)
				if (texIndexes)
				{
					assert(!mesh->getTriangleMtlIndexesTable());
					texIndexes->release();
					texIndexes = 0;
				}
				else
				{
					mesh->removePerTriangleMtlIndexes();
				}
				mesh->removePerTriangleTexCoordIndexes();
				mesh->showMaterials(false);
			}
		}

		if (cloud->hasColors())
			mesh->showColors(true);
		if (cloud->hasDisplayedScalarField())
			mesh->showSF(true);
		if (cloud->hasNormals())
			mesh->showNormals(true);
		else
		{
			//DGM: normals can be per-vertex or per-triangle so it's better to let the user do it himself later
			//Moreover it's not always good idea if the user doesn't want normals (especially in ccViewer!)
			//mesh->computeNormals();
			ccLog::Warning("[PLY] Mesh has no normal! You can manually compute them (select it then call \"Edit > Normals > Compute\")");
		}

		if (mesh->hasMaterials())
			mesh->showNormals(false);

		container.addChild(mesh);
	}
	else
	{
		container.addChild(cloud);
	}

	if (texCoords)
	{
		texCoords->release();
		texCoords = 0;
	}
	if (texIndexes)
	{
		texIndexes->release();
		texIndexes = 0;
	}

	return CC_FERR_NO_ERROR;
}