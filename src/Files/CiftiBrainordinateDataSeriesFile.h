#ifndef __CIFTI_BRAINORDINATE_DATA_SERIES_FILE_H__
#define __CIFTI_BRAINORDINATE_DATA_SERIES_FILE_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
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

#include "BrainConstants.h"
#include "ChartableLineSeriesBrainordinateInterface.h"
#include "CiftiMappableDataFile.h"

namespace caret {
    class CiftiConnectivityMatrixDenseDynamicFile;
    class PaletteFile;

    class CiftiBrainordinateDataSeriesFile :
    public CiftiMappableDataFile,
    public ChartableLineSeriesBrainordinateInterface {
        
    public:
        CiftiBrainordinateDataSeriesFile();
        
        virtual ~CiftiBrainordinateDataSeriesFile();
        
        virtual bool isLineSeriesChartingEnabled(const int32_t tabIndex) const;
        
        virtual void setLineSeriesChartingEnabled(const int32_t tabIndex,
                                        const bool enabled);
        
        virtual bool isLineSeriesChartingSupported() const;
        
        virtual ChartDataCartesian* loadLineSeriesChartDataForSurfaceNode(const StructureEnum::Enum structure,
                                                                                   const int32_t nodeIndex);
        
        virtual ChartDataCartesian* loadAverageLineSeriesChartDataForSurfaceNodes(const StructureEnum::Enum structure,
                                                               const std::vector<int32_t>& nodeIndices);
        
        virtual ChartDataCartesian* loadLineSeriesChartDataForVoxelAtCoordinate(const float xyz[3]);
        
        
        virtual void getSupportedLineSeriesChartDataTypes(std::vector<ChartVersionOneDataTypeEnum::Enum>& chartDataTypesOut) const;
        
        CiftiConnectivityMatrixDenseDynamicFile* getConnectivityMatrixDenseDynamicFile();

        //const CiftiConnectivityMatrixDenseDynamicFile* getConnectivityMatrixDenseDynamicFile() const;
        
        virtual void clear();
        
        virtual void readFile(const AString& ciftiMapFileName);
        
        virtual void writeFile(const AString& filename);
        
        virtual bool isModifiedPaletteColorMapping() const;
        
    private:
        CiftiBrainordinateDataSeriesFile(const CiftiBrainordinateDataSeriesFile&);

        CiftiBrainordinateDataSeriesFile& operator=(const CiftiBrainordinateDataSeriesFile&);
        
        virtual void saveFileDataToScene(const SceneAttributes* sceneAttributes,
                                         SceneClass* sceneClass);
        
        virtual void restoreFileDataFromScene(const SceneAttributes* sceneAttributes,
                                              const SceneClass* sceneClass);
    public:

        // ADD_NEW_METHODS_HERE

    private:
        // ADD_NEW_MEMBERS_HERE

        void initializeDenseDynamicFile();
        
        bool m_chartingEnabledForTab[BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS];
        
        CiftiConnectivityMatrixDenseDynamicFile* m_lazyInitializedDenseDynamicFile;
        
        static const AString s_paletteColorMappingNameInMetaData;
    };
    
#ifdef __CIFTI_BRAINORDINATE_DATA_SERIES_FILE_DECLARE__
    const AString CiftiBrainordinateDataSeriesFile::s_paletteColorMappingNameInMetaData = "__DYNAMIC_FILE_PALETTE_COLOR_MAPPING__";
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __CIFTI_BRAINORDINATE_DATA_SERIES_FILE_DECLARE__

} // namespace
#endif  //__CIFTI_BRAINORDINATE_DATA_SERIES_FILE_H__
