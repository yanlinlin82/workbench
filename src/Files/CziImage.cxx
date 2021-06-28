
/*LICENSE_START*/
/*
 *  Copyright (C) 2021 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __CZI_IMAGE_DECLARE__
#include "CziImage.h"
#undef __CZI_IMAGE_DECLARE__

#include <QImage>

#include "BoundingBox.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CziImageFile.h"
#include "GraphicsPrimitiveV3fT3f.h"
#include "GraphicsUtilitiesOpenGL.h"
#include "ImageFile.h"
#include "RectangleTransform.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"
#include "VolumeSpace.h"

using namespace caret;


    
/**
 * \class caret::CziImage 
 * \brief Image read from CZI file
 * \ingroup Files
 */

/**
 * Constructor
 * @param parentCziImageFile
 *    The parent CZI Image File
 * @param image
 *    The QImage instance
 * @param fullResolutionLogicalRect
 *    Logical Rectangle for the full-resolution source image
 * @param logicalRect
 *    Logical rectangle defining region of source image that was read from the file
 * @param spatialInfo
 *    The spatial information
 */
CziImage::CziImage(const CziImageFile* parentCziImageFile,
                   QImage* image,
                   const QRectF& fullResolutionLogicalRect,
                   const QRectF& logicalRect,
                   MediaFile::SpatialInfo& spatialInfo)
: CaretObject(),
m_parentCziImageFile(parentCziImageFile),
m_image(image),
m_fullResolutionLogicalRect(fullResolutionLogicalRect),
m_logicalRect(logicalRect),
m_pixelToCoordinateTransform(spatialInfo.m_volumeSpace),
m_spatialBoundingBox(spatialInfo.m_boundingBox)
{
    CaretAssert(image);
    CaretAssert(spatialInfo.m_volumeSpace);
    CaretAssert(spatialInfo.m_boundingBox);

    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());
    
    m_pixelsRect = QRectF(0, 0, logicalRect.width() - 1, logicalRect.height() - 1);

    QRectF pixelTopLeftRect(0, 0, logicalRect.width() - 1, logicalRect.height() - 1);
    m_roiCoordsToRoiPixelTopLeftTransform.reset(new RectangleTransform(logicalRect,
                                                                       RectangleTransform::Origin::TOP_LEFT,
                                                                       pixelTopLeftRect,
                                                                       RectangleTransform::Origin::TOP_LEFT));
    
    QRectF fullImagePixelTopLeftRect(0, 0, m_fullResolutionLogicalRect.width() - 1, m_fullResolutionLogicalRect.height() - 1);
    m_roiPixelTopLeftToFullImagePixelTopLeftTransform.reset(new RectangleTransform(pixelTopLeftRect,
                                                                                   RectangleTransform::Origin::TOP_LEFT,
                                                                                   fullImagePixelTopLeftRect,
                                                                                   RectangleTransform::Origin::TOP_LEFT));
    
    RectangleTransform::testTransforms(*m_roiCoordsToRoiPixelTopLeftTransform,
                                       logicalRect,
                                       pixelTopLeftRect);
    RectangleTransform::testTransforms(*m_roiPixelTopLeftToFullImagePixelTopLeftTransform,
                                       pixelTopLeftRect,
                                       fullImagePixelTopLeftRect);
}

/**
 * Destructor.
 */
CziImage::~CziImage()
{
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
CziImage::toString() const
{
    return "CziImage";
}

/**
 * Transform a pixel index to a different pixel space
 * @param pixelIndex
 *    The pixel index
 * @param fromPixelCoordSpace
 *    Current space of the input pixel
 * @param toPixelCoordSpace
 *    Space to transform to
 * @return Pixel index in new space
 */
PixelIndex
CziImage::transformPixelIndexToSpace(const PixelIndex& pixelIndex,
                                     const CziPixelCoordSpaceEnum::Enum fromPixelCoordSpace,
                                     const CziPixelCoordSpaceEnum::Enum toPixelCoordSpace)
{
    if (fromPixelCoordSpace == toPixelCoordSpace) {
        return pixelIndex;
    }
    
    QRectF fromRect;
    RectangleTransform::Origin fromRectOrigin;
    switch (fromPixelCoordSpace) {
        case CziPixelCoordSpaceEnum::LOGICAL_TOP_LEFT:
            fromRect = m_logicalRect;
            fromRectOrigin = RectangleTransform::Origin::TOP_LEFT;
            break;
        case CziPixelCoordSpaceEnum::PIXEL_BOTTOM_LEFT:
            fromRect = m_pixelsRect;
            fromRectOrigin = RectangleTransform::Origin::BOTTOM_LEFT;
            break;
        case CziPixelCoordSpaceEnum::PIXEL_TOP_LEFT:
            fromRect = m_pixelsRect;
            fromRectOrigin = RectangleTransform::Origin::TOP_LEFT;
            break;
    }
    
    QRectF toRect;
    RectangleTransform::Origin toRectOrigin;
    switch (toPixelCoordSpace) {
        case CziPixelCoordSpaceEnum::LOGICAL_TOP_LEFT:
            toRect = m_logicalRect;
            toRectOrigin = RectangleTransform::Origin::TOP_LEFT;
            break;
        case CziPixelCoordSpaceEnum::PIXEL_BOTTOM_LEFT:
            toRect = m_pixelsRect;
            toRectOrigin = RectangleTransform::Origin::TOP_LEFT;
            break;
        case CziPixelCoordSpaceEnum::PIXEL_TOP_LEFT:
            toRect = m_pixelsRect;
            toRectOrigin = RectangleTransform::Origin::TOP_LEFT;
            break;
    }
    
    PixelIndex pixelIndexOut;

    RectangleTransform transform(fromRect,
                                 fromRectOrigin,
                                 toRect,
                                 toRectOrigin);
    if ( ! transform.isValid()) {
        CaretLogSevere("Creating rectangle transform failed: "
                       + transform.getErrorMessage());
        return pixelIndexOut;
    }
    
    float x(0.0), y(0.0);
    transform.transformSourceToTarget(pixelIndex.getI(), pixelIndex.getJ(),
                                      x, y);
    pixelIndexOut.setI(static_cast<int64_t>(x));
    pixelIndexOut.setJ(static_cast<int64_t>(y));
    
    return pixelIndexOut;
}

/**
 * @return The graphics primitive for drawing the image as a texture in media drawing model.
 */
GraphicsPrimitiveV3fT3f*
CziImage::getGraphicsPrimitiveForMediaDrawing() const
{
    if (m_image == NULL) {
        return NULL;
    }
    
    if (m_graphicsPrimitiveForMediaDrawing == NULL) {
        std::vector<uint8_t> bytesRGBA;
        int32_t width(0);
        int32_t height(0);
        
        /*
         * If image is too big for OpenGL texture limits, scale image to acceptable size
         */
        const int32_t maxTextureWidthHeight = GraphicsUtilitiesOpenGL::getTextureWidthHeightMaximumDimension();
        if (maxTextureWidthHeight > 0) {
            const int32_t excessWidth(m_image->width() - maxTextureWidthHeight);
            const int32_t excessHeight(m_image->height() - maxTextureWidthHeight);
            if ((excessWidth > 0)
                || (excessHeight > 0)) {
                if (excessWidth > excessHeight) {
                    CaretLogWarning(m_parentCziImageFile->getFileName()
                                    + " is too big for texture.  Maximum width/height is: "
                                    + AString::number(maxTextureWidthHeight)
                                    + " Image Width: "
                                    + AString::number(m_image->width())
                                    + " Image Height: "
                                    + AString::number(m_image->height()));
                }
            }
        }
        
        /*
         * Some images may use a color table so convert images
         * if there are not in preferred format prior to
         * getting colors of pixels
         */
        bool validRGBA(false);
        if (m_image->format() != QImage::Format_ARGB32) {
            QImage image = m_image->convertToFormat(QImage::Format_ARGB32);
            if (! image.isNull()) {
                ImageFile convImageFile;
                convImageFile.setFromQImage(image);
                validRGBA = convImageFile.getImageBytesRGBA(ImageFile::IMAGE_DATA_ORIGIN_AT_BOTTOM,
                                                            bytesRGBA,
                                                            width,
                                                            height);
            }
        }
        else {
            validRGBA = ImageFile::getImageBytesRGBA(m_image.get(),
                                                     ImageFile::IMAGE_DATA_ORIGIN_AT_BOTTOM,
                                                     bytesRGBA,
                                                     width,
                                                     height);
        }
        
        if (validRGBA) {
            GraphicsPrimitiveV3fT3f* primitive = GraphicsPrimitive::newPrimitiveV3fT3f(GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLE_STRIP,
                                                                                       &bytesRGBA[0],
                                                                                       width,
                                                                                       height,
                                                                                       GraphicsPrimitive::TextureWrappingType::CLAMP,
                                                                                       GraphicsPrimitive::TextureFilteringType::LINEAR,
                                                                                       GraphicsTextureMagnificationFilterEnum::LINEAR,
                                                                                       GraphicsTextureMinificationFilterEnum::LINEAR_MIPMAP_LINEAR);
            
            /*
             * Coordinates at EDGE of the pixels
             */
            const float minX = 0;
            const float maxX = width - 1;
            const float minY = 0;
            const float maxY = height - 1;
            
            /*
             * A Triangle Strip (consisting of two triangles) is used
             * for drawing the image.
             * The order of the vertices in the triangle strip is
             * Top Left, Bottom Left, Top Right, Bottom Right.
             */
            const float minTextureST(0.0);
            const float maxTextureST(1.0);
            primitive->addVertex(minX, maxY, minTextureST, maxTextureST);  /* Top Left */
            primitive->addVertex(minX, minY, minTextureST, minTextureST);  /* Bottom Left */
            primitive->addVertex(maxX, maxY, maxTextureST, maxTextureST);  /* Top Right */
            primitive->addVertex(maxX, minY, maxTextureST, minTextureST);  /* Bottom Right */
            
            m_graphicsPrimitiveForMediaDrawing.reset(primitive);
        }
    }
    
    return m_graphicsPrimitiveForMediaDrawing.get();
}

/**
 * Save information specific to this type of model to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param instanceName
 *    Name of instance in the scene.
 */
SceneClass*
CziImage::saveToScene(const SceneAttributes* sceneAttributes,
                                 const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "CziImage",
                                            1);
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
    
    // Uncomment if sub-classes must save to scene
    //saveSubClassDataToScene(sceneAttributes,
    //                        sceneClass);
    
    return sceneClass;
}

/**
 * Restore information specific to the type of model from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void
CziImage::restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);    
    
    //Uncomment if sub-classes must restore from scene
    //restoreSubClassDataFromScene(sceneAttributes,
    //                             sceneClass);
    
}

