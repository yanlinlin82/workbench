/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "OperationCiftiMath.h"
#include "OperationException.h"

#include "CaretLogger.h"
#include "CaretMathExpression.h"
#include "CiftiFile.h"

//prepare for 3+ dimensions early
#include "CiftiXML.h"

using namespace caret;
using namespace std;

AString OperationCiftiMath::getCommandSwitch()
{
    return "-cifti-math";
}

AString OperationCiftiMath::getShortDescription()
{
    return "EVALUATE EXPRESSION ON CIFTI FILES";
}

OperationParameters* OperationCiftiMath::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    
    ret->addStringParameter(1, "expression", "the expression to evaluate, in quotes");
    
    ret->addCiftiOutputParameter(2, "cifti-out", "the output cifti file");
    
    ParameterComponent* varOpt = ret->createRepeatableParameter(3, "-var", "a cifti file to use as a variable");
    varOpt->addStringParameter(1, "name", "the name of the variable, as used in the expression");
    varOpt->addCiftiParameter(2, "cifti", "the cifti file to use as this variable");
    ParameterComponent* selectOpt = varOpt->createRepeatableParameter(3, "-select", "select a single index from a dimension");//repeatable option to repeatable option
    selectOpt->addIntegerParameter(1, "dim", "the dimension to select from (1-based)");
    selectOpt->addIntegerParameter(2, "index", "the index to use (1-based)");
    selectOpt->createOptionalParameter(3, "-repeat", "repeat the selected values for each index of output in this dimension");//with a repeat option
    
    OptionalParameter* fixNanOpt = ret->createOptionalParameter(4, "-fixnan", "replace NaN results with a value");
    fixNanOpt->addDoubleParameter(1, "replace", "value to replace NaN with");
    
    ret->createOptionalParameter(5, "-override-mapping-check", "don't check the mappings for compatibility, only check length");
    
    AString myText = AString("This command evaluates <expression> at each (row, column) location independently.  ") +
                             "There must be at least one -var option (to get the output layout from), even if the <name> specified in it isn't used in <expression>.\n\n" +
                             "To select a single column from a file, use -select 1 <index>, where <index> is 1-based.  " +
                             "To select a single row, use -select 2 <index>.  " +
                             "Where -select is not used, the cifti files must have compatible mappings (e.g., brain models and parcels mappings must match exactly except for parcel names).  " +
                             "Use -override-mapping-check to skip this checking.\n\n" +
                             "Filenames are not valid in <expression>, use a variable name and a -var option with matching <name> to specify an input file.  " +
                             "The format of <expression> is as follows:\n\n";
    myText += CaretMathExpression::getExpressionHelpInfo();
    ret->setHelpText(myText);
    return ret;
}

void OperationCiftiMath::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    LevelProgress myProgress(myProgObj);
    AString expression = myParams->getString(1);
    CaretMathExpression myExpr(expression);
    const vector<AString>& myVarNames = myExpr.getVarNames();
    CiftiFile* myCiftiOut = myParams->getOutputCifti(2);
    const vector<ParameterComponent*>& myVarOpts = *(myParams->getRepeatableParameterInstances(3));
    OptionalParameter* fixNanOpt = myParams->getOptionalParameter(4);
    bool nanfix = false;
    float nanfixval = 0;
    if (fixNanOpt->m_present)
    {
        nanfix = true;
        nanfixval = (float)fixNanOpt->getDouble(1);
    }
    bool overrideMapCheck = myParams->getOptionalParameter(5)->m_present;
    int numInputs = myVarOpts.size();
    int numVars = myVarNames.size();
    vector<CiftiFile*> varCiftiFiles(numVars, (CiftiFile*)NULL);
    if (numInputs == 0) throw OperationException("you must specify at least one input file (-var), even if the expression doesn't use a variable");
    CiftiFile* first = myVarOpts[0]->getCifti(2);
    CiftiXML outXML, tempXML;
    QString xmlText;
    vector<int64_t> outDims;//don't even assume 2 dimensions, in case someone makes a 1-d cifti
    vector<vector<int64_t> > selectInfo(numVars);
    for (int i = 0; i < numInputs; ++i)
    {
        AString varName = myVarOpts[i]->getString(1);
        double constVal;
        if (CaretMathExpression::getNamedConstant(varName, constVal))
        {
            throw OperationException("'" + varName + "' is a named constant equal to " + AString::number(constVal, 'g', 15) + ", please use a different variable name");
        }
        myVarOpts[i]->getCifti(2)->getCiftiXMLOld().writeXML(xmlText);//transitional code until the new xml object replaces the old
        tempXML.readXML(xmlText);//however, cifti-1 doesn't contain the length of series dimensions, so check for this
        if (tempXML.getDimensionLength(CiftiXML::ALONG_ROW) < 1)
        {
            CiftiSeriesMap tempMap = tempXML.getSeriesMap(CiftiXML::ALONG_ROW);
            tempMap.setLength(first->getCiftiXMLOld().getDimensionLength(CiftiXMLOld::ALONG_ROW));
            tempXML.setMap(CiftiXML::ALONG_ROW, tempMap);
        }
        if (tempXML.getDimensionLength(CiftiXML::ALONG_COLUMN) < 1)
        {
            CiftiSeriesMap tempMap = tempXML.getSeriesMap(CiftiXML::ALONG_COLUMN);
            tempMap.setLength(first->getCiftiXMLOld().getDimensionLength(CiftiXMLOld::ALONG_COLUMN));
            tempXML.setMap(CiftiXML::ALONG_COLUMN, tempMap);
        }//end transitional code
        int thisNumDims = tempXML.getNumberOfDimensions();
        vector<int64_t> thisSelectInfo(thisNumDims, -1);
        vector<bool> thisRepeat(thisNumDims, false);
        const vector<ParameterComponent*>& thisSelectOpts = *(myVarOpts[i]->getRepeatableParameterInstances(3));
        for (int j = 0; j < (int)thisSelectOpts.size(); ++j)
        {
            int dim = (int)thisSelectOpts[j]->getInteger(1) - 1;
            int64_t thisIndex = thisSelectOpts[j]->getInteger(2) - 1;
            if (dim >= (int)thisSelectInfo.size())
            {
                if (thisIndex != 0) throw OperationException("-select used  for variable '" + varName + "' with index other than 1 on nonexistent dimension");
                thisSelectInfo.resize(dim + 1, -1);
                thisRepeat.resize(dim + 1, false);
            }
            thisSelectInfo[dim] = thisIndex;
            thisRepeat[dim] = thisSelectOpts[j]->getOptionalParameter(3)->m_present;
        }
        bool found = false;
        for (int j = 0; j < numVars; ++j)
        {
            if (varName == myVarNames[j])
            {
                if (varCiftiFiles[j] != NULL) throw OperationException("variable '" + varName + "' specified more than once");
                found = true;
                varCiftiFiles[j] = myVarOpts[i]->getCifti(2);
                selectInfo[j] = thisSelectInfo;//copy selection info
                break;
            }
        }
        if (!found && (numVars != 0 || numInputs != 1))//supress warning when a single -var is used with a constant expression, as required per help
        {
            CaretLogWarning("variable '" + varName + "' not used in expression");
        }
        int newNumDims = (int)max(thisSelectInfo.size(), outDims.size());//now, to figure out the output dimensions with -select and -repeat
        for (int j = 0; j < newNumDims; ++j)
        {
            if (j >= (int)outDims.size())//need to expand output dimensions
            {
                outXML.setNumberOfDimensions(j + 1);//does not clear existing mappings
                outDims.push_back(-1);//unknown length
            }
            if (j >= (int)thisSelectInfo.size())//need to expand input info
            {
                thisSelectInfo.push_back(-1);//use "all" indices, but there is only 1 (virtual) index, pushing 0 should have same effect
                thisRepeat.push_back(false);//repeat not specified
            }
            if (outDims[j] == -1)//if we don't know the output length yet, put it in if we have it (-repeat not specified)
            {
                if (thisSelectInfo[j] == -1)//no -select for this dimension, use all maps
                {
                    if (j < thisNumDims)
                    {
                        outDims[j] = tempXML.getDimensionLength(j);
                        outXML.setMap(j, *(tempXML.getMap(j)));//copy the mapping type, since this input defines this dimension
                    } else {//if higher dimension than the file has, transparently say it is of length 1, and don't use the mapping
                        outDims[j] = 1;
                    }
                } else {//-select was used
                    if (!thisRepeat[j])//if -repeat wasn't used, output length is 1
                    {
                        outDims[j] = 1;
                    }
                }
            } else {
                if (thisSelectInfo[j] == -1)//-select was not used
                {
                    if (j < thisNumDims)
                    {
                        if (outDims[j] != tempXML.getDimensionLength(j))
                        {
                            throw OperationException("variable '" + varName + "' has length " + AString::number(tempXML.getDimensionLength(j)) +
                                                    " for dimension " + AString::number(j + 1) + " while previous -var options require a length of " + AString::number(outDims[j]));
                        }
                        if (outXML.getMap(j) == NULL)//dimension was set to 1 by -select, but didn't set a mapping, so borrow from here
                        {
                            outXML.setMap(j, *(tempXML.getMap(j)));
                        } else {//test mapping types for compatibility since -select wasn't used
                            if (!overrideMapCheck && !outXML.getMap(j)->approximateMatch(*(tempXML.getMap(j))))
                            {
                                throw OperationException("mismatch in spatial output mapping for variable '" + varName + "', dimension " + AString::number(j + 1));
                            }
                        }
                    } else {
                        if (outDims[j] != 1)
                        {
                            throw OperationException(AString("variable '" + varName + "' is of lower dimensionality than output, ") +
                                                            "and the length of output dimension " + AString::number(j + 1) + " is " +
                                                            AString::number(outDims[j]) + ", you might want to use -select with -repeat");
                        }
                    }
                } else {
                    if (!thisRepeat[j])
                    {
                        if (outDims[j] != 1)
                        {
                            throw OperationException("variable '" + varName + "' uses -select for dimension " + AString::number(j + 1) +
                                                     ", but previous -var options require a length of " + AString::number(outDims[j]));
                        }
                    }
                }
            }
        }
    }
    if (numVars > 0 && varCiftiFiles[0] != NULL) first = varCiftiFiles[0];
    for (int i = 0; i < numVars; ++i)
    {
        if (varCiftiFiles[i] == NULL) throw OperationException("no -var option specified for variable '" + myVarNames[i] + "'");
    }
    CiftiScalarsMap dummyMap;//make an empty length-1 scalar map for dimensions we don't have a mapping for
    dummyMap.setLength(1);
    for (int i = 0; i < outXML.getNumberOfDimensions(); ++i)
    {
        if (outDims[i] == -1) throw OperationException("all -var options used -select and -repeat for dimension " +
                                                       AString::number(i + 1) + ", there is no file to get the dimension length from");
        if (outXML.getMap(i) == NULL)//-select was used in all variables for this dimension, so we don't have a mapping
        {
            outXML.setMap(i, dummyMap);//so, make it a length-1 scalar with no name and empty metadata
        }
    }
    if (outXML.getNumberOfDimensions() != 2)//transitional code back to old XML and CiftiMatrix
    {
        throw OperationException("output must have exactly 2 dimensions");
    }
    CiftiXMLOld outOldXML;
    outOldXML.readXML(outXML.writeXMLToString(CiftiVersion(1, 0)));//force it to write as 1.0 so old XML understands it
    if (outOldXML.getDimensionLength(CiftiXMLOld::ALONG_ROW) < 1)//it doesn't know timeseries length, so set it manually if needed
    {
        outOldXML.setRowNumberOfTimepoints(outXML.getDimensionLength(CiftiXML::ALONG_ROW));
    }
    if (outOldXML.getDimensionLength(CiftiXMLOld::ALONG_COLUMN) < 1)//ditto
    {
        outOldXML.setColumnNumberOfTimepoints(outXML.getDimensionLength(CiftiXML::ALONG_COLUMN));
    }
    myCiftiOut->setCiftiXML(outOldXML);
    int numRows = outOldXML.getNumberOfRows(), numOutCols = outOldXML.getNumberOfColumns();
    vector<float> values(numVars), scratchRow(numOutCols);
    vector<vector<float> > inputRows(numVars);
    for (int v = 0; v < numVars; ++v)//HACK: this code ONLY works in the 2D case, rework from here to the end when allowing 3+ dims
    {
        inputRows[v].resize(varCiftiFiles[v]->getCiftiXMLOld().getNumberOfColumns());
        if (selectInfo[v][1] != -1)//if -select 2 (dimension 1) is used, read the row only once
        {
            varCiftiFiles[v]->getRow(inputRows[v].data(), selectInfo[v][1]);
        }
    }
    for (int i = 0; i < numRows; ++i)
    {
        for (int v = 0; v < numVars; ++v)
        {
            if (selectInfo[v][1] == -1)//only request rows in the loop for files that did not have -select 1
            {
                varCiftiFiles[v]->getRow(inputRows[v].data(), i);
            }
        }
        for (int j = 0; j < numOutCols; ++j)
        {
            for (int v = 0; v < numVars; ++v)
            {
                if (selectInfo[v][0] == -1)
                {
                    values[v] = inputRows[v][j];
                } else {
                    values[v] = inputRows[v][selectInfo[v][0]];
                }
            }
            scratchRow[j] = (float)myExpr.evaluate(values);
            if (nanfix && scratchRow[j] != scratchRow[j])
            {
                scratchRow[j] = nanfixval;
            }
        }
        myCiftiOut->setRow(scratchRow.data(), i);
    }
}
