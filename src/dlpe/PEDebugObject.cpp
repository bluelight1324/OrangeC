/* Software License Agreement
 * 
 *     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version, with the addition of the 
 *     Orange C "Target Code" exception.
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

#include "PEObject.h"
#include "Utils.h"
#include "sqlite3.h"

#include <stdio.h>
#include <ctype.h>
#include <map>
#include <string.h>

void PEDebugObject::Setup(ObjInt &endVa, ObjInt &endPhys)
{
    if (virtual_addr == 0)
    {
        virtual_addr = endVa;
    }
    else
    {
        if (virtual_addr != endVa)
            Utils::fatal("Internal error");
    }
    raw_addr = endPhys;
    size = initSize = fileName.size() + 2 + 32;
    data = new unsigned char[512];
    memset(data,0,512);
    data[0] = 'L';
    data[1] = 'S';
    data[2] = '1';
    data[3] = '4';
    data[32] = fileName.size();
    strcpy((char *)data + 33, fileName.c_str());
    
    endVa = ObjectAlign(objectAlign, endVa + size);
    endPhys = ObjectAlign(fileAlign, endPhys + initSize);
}
int PEDebugObject::NullCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    return 0;
}
void PEDebugObject::SetDebugInfo(ObjString fileName, ObjInt base)
{
    sqlite3 *dbPointer = nullptr;
    if (sqlite3_open_v2(fileName.c_str(), &dbPointer, SQLITE_OPEN_READWRITE, nullptr) == SQLITE_OK)
    {
        sqlite3_busy_timeout(dbPointer, 400);
        char *zErrMsg  = 0;
        static char *cmd =     "INSERT INTO dbPropertyBag (property, value)"
                               " VALUES (\"ImageBase\", %d);";
        char realCmd[256];
        sprintf(realCmd, cmd, base);

        int rc = sqlite3_exec(dbPointer, realCmd, NullCallback, 0, &zErrMsg);
        if( rc!=SQLITE_OK )
        {
          fprintf(stderr, "SQL error: %s\n", zErrMsg);
          sqlite3_free(zErrMsg);
        }

    }
    if (dbPointer)
        sqlite3_close(dbPointer);
}
