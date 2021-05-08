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

#include "Font.h"
#include "RCFile.h"
#include "ResFile.h"
#include "ResourceData.h"
#include <stdexcept>

Font::Font(const ResourceId& Id, const ResourceInfo& info) : Resource(eFont, Id, info), data(nullptr) {}
Font::~Font() {}
void Font::SetData(ResourceData* rdata) { data.reset(rdata); }
void Font::WriteRes(ResFile& resFile)
{
    Resource::WriteRes(resFile);
    if (data)
        data->WriteRes(resFile);
    resFile.Release();
}
void Font::ReadRC(RCFile& rcFile)
{
    resInfo.SetFlags(resInfo.GetFlags() | ResourceInfo::Pure);
    resInfo.ReadRC(rcFile, false);
    data = std::make_unique<ResourceData>();
    data->ReadRC(rcFile);
    rcFile.NeedEol();
    // check version and size
    /*
        if *n != 0x200 && n != 0x300 || s != rd->GetLen() || s < 0x180)
        {
            delete rd;
            throw std::runtime_error("Invalid font file");
        }
    */
}
