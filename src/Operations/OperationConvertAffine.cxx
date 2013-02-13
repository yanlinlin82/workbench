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

#include "AffineFile.h"
#include "OperationConvertAffine.h"
#include "OperationException.h"

using namespace caret;
using namespace std;

AString OperationConvertAffine::getCommandSwitch()
{
    return "-convert-affine";
}

AString OperationConvertAffine::getShortDescription()
{
    return "CONVERT AN AFFINE FILE BETWEEN CONVENTIONS";
}

OperationParameters* OperationConvertAffine::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    
    OptionalParameter* fromWorld = ret->createOptionalParameter(1, "-from-world", "input is a NIFTI 'world' affine");
    fromWorld->addStringParameter(1, "input", "the input affine");
    
    OptionalParameter* fromFlirt = ret->createOptionalParameter(2, "-from-flirt", "input is a flirt matrix");
    fromFlirt->addStringParameter(1, "input", "the input affine");
    fromFlirt->addStringParameter(2, "source-volume", "the source volume used when generating the input affine");
    fromFlirt->addStringParameter(3, "target-volume", "the target volume used when generating the input affine");
    
    OptionalParameter* toWorld = ret->createOptionalParameter(3, "-to-world", "write output as a NIFTI 'world' affine");
    toWorld->addStringParameter(1, "output", "output - the output affine");//HACK: fake the output formatting, since we don't have a parameter for affine file (hard to do due to multiple on-disk formats)
    
    ParameterComponent* toFlirt = ret->createRepeatableParameter(4, "-to-flirt", "write output as a flirt matrix");
    toFlirt->addStringParameter(1, "output", "output - the output affine");
    toFlirt->addStringParameter(2, "source-volume", "the volume you want to apply the transform to");
    toFlirt->addStringParameter(3, "target-volume", "the target space you want the transformed volume to match");
    
    ret->setHelpText(
        AString("NIFTI world matrices can be used directly on mm coordinates via matrix multiplication, they use the NIFTI coordinate system, that is, ") +
        "positive X is right, positive Y is anterior, and positive Z is superior.\n\n" +
        "You must specify exactly one -from option, but you may specify multiple -to options, and any -to option that takes volumes may be specified more than once."
    );
    return ret;
}

void OperationConvertAffine::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    LevelProgress myProgress(myProgObj);
    AffineFile myAffine;
    bool haveInput = false;
    OptionalParameter* fromWorld = myParams->getOptionalParameter(1);
    if (fromWorld->m_present)
    {
        haveInput = true;
        myAffine.readWorld(fromWorld->getString(1));
    }
    OptionalParameter* fromFlirt = myParams->getOptionalParameter(2);
    if (fromFlirt->m_present)
    {
        if (haveInput) throw OperationException("only one -from option may be specified");
        haveInput = true;
        myAffine.readFlirt(fromFlirt->getString(1), fromFlirt->getString(2), fromFlirt->getString(3));
    }
    if (!haveInput) throw OperationException("you must specify a -from option");
    OptionalParameter* toWorld = myParams->getOptionalParameter(3);
    if (toWorld->m_present)
    {
        myAffine.writeWorld(toWorld->getString(1));
    }
    const vector<ParameterComponent*>& toFlirt = *(myParams->getRepeatableParameterInstances(4));//the return of this is a pointer so that it can return NULL if the key is wrong, after asserting
    int numFlirt = (int)toFlirt.size();//so, dereference immediately since it should be caught in debug via assert
    for (int i = 0; i < numFlirt; ++i)
    {
        myAffine.writeFlirt(toFlirt[i]->getString(1), toFlirt[i]->getString(2), toFlirt[i]->getString(3));
    }
}
