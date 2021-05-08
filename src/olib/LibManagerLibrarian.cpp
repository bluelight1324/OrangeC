/* Software License Agreement
 * 
 *     Copyright(C) 1994-2021 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 * 
 */

#include "LibManager.h"
#include "ObjIeee.h"
#include "ObjFactory.h"
#include <cstring>

int LibManager::SaveLibrary()
{
    InitHeader();
    ObjIeeeIndexManager im1;
    ObjFactory fact1(&im1);
    if (!files.ReadFiles(stream, &fact1))
        return CANNOT_READ;
    dictionary.CreateDictionary(files);
    // can't do more reading
    Close();
    header.filesInModule = files.size();
    FILE* ostr = fopen(name.c_str(), "wb");
    if (!ostr)
    {
        return CANNOT_CREATE;
    }
    if (fwrite(&header, sizeof(header), 1, ostr) != 1)
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (!Align(ostr, 16))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    header.namesOffset = ftell(ostr);
    if (!files.WriteNames(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (!Align(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    header.filesOffset = ftell(ostr);
    if (!files.WriteFiles(ostr, ALIGN))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (!Align(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    header.offsetsOffset = ftell(ostr);
    if (!files.WriteOffsets(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (!Align(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    header.dictionaryOffset = ftell(ostr);
    header.dictionaryBlocks = 0;
    if (!dictionary.Write(ostr))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (fseek(ostr, 0, SEEK_SET))
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    if (fwrite(&header, sizeof(header), 1, ostr) != 1)
    {
        fclose(ostr);
        return CANNOT_WRITE;
    }
    fclose(ostr);
    return SUCCESS;
}
bool LibManager::Align(FILE* ostr, ObjInt align)
{
    char buf[ALIGN];
    memset(buf, 0, align);
    int n = ftell(ostr);
    if (n % align)
        if (fwrite(buf, align - (n % align), 1, ostr) != 1)
            return false;
    return true;
}
