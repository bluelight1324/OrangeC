/* Software License Agreement
 *
 *     Copyright(C) 1994-2020 David Lindauer, (LADSoft)
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

#include "UTF8.h"

int UTF8::upper[][2] = {
    {0x00061, 0x00041}, {0x00062, 0x00042}, {0x00063, 0x00043}, {0x00064, 0x00044}, {0x00065, 0x00045}, {0x00066, 0x00046},
    {0x00067, 0x00047}, {0x00068, 0x00048}, {0x00069, 0x00049}, {0x0006a, 0x0004a}, {0x0006b, 0x0004b}, {0x0006c, 0x0004c},
    {0x0006d, 0x0004d}, {0x0006e, 0x0004e}, {0x0006f, 0x0004f}, {0x00070, 0x00050}, {0x00071, 0x00051}, {0x00072, 0x00052},
    {0x00073, 0x00053}, {0x00074, 0x00054}, {0x00075, 0x00055}, {0x00076, 0x00056}, {0x00077, 0x00057}, {0x00078, 0x00058},
    {0x00079, 0x00059}, {0x0007a, 0x0005a}, {0x000e0, 0x000c0}, {0x000e1, 0x000c1}, {0x000e2, 0x000c2}, {0x000e3, 0x000c3},
    {0x000e4, 0x000c4}, {0x000e5, 0x000c5}, {0x000e6, 0x000c6}, {0x000e7, 0x000c7}, {0x000e8, 0x000c8}, {0x000e9, 0x000c9},
    {0x000ea, 0x000ca}, {0x000eb, 0x000cb}, {0x000ec, 0x000cc}, {0x000ed, 0x000cd}, {0x000ee, 0x000ce}, {0x000ef, 0x000cf},
    {0x000f0, 0x000d0}, {0x000f1, 0x000d1}, {0x000f2, 0x000d2}, {0x000f3, 0x000d3}, {0x000f4, 0x000d4}, {0x000f5, 0x000d5},
    {0x000f6, 0x000d6}, {0x000f8, 0x000d8}, {0x000f9, 0x000d9}, {0x000fa, 0x000da}, {0x000fb, 0x000db}, {0x000fc, 0x000dc},
    {0x000fd, 0x000dd}, {0x000fe, 0x000de}, {0x000ff, 0x00178}, {0x00101, 0x00100}, {0x00103, 0x00102}, {0x00105, 0x00104},
    {0x00107, 0x00106}, {0x00109, 0x00108}, {0x0010b, 0x0010a}, {0x0010d, 0x0010c}, {0x0010f, 0x0010e}, {0x00111, 0x00110},
    {0x00113, 0x00112}, {0x00115, 0x00114}, {0x00117, 0x00116}, {0x00119, 0x00118}, {0x0011b, 0x0011a}, {0x0011d, 0x0011c},
    {0x0011f, 0x0011e}, {0x00121, 0x00120}, {0x00123, 0x00122}, {0x00125, 0x00124}, {0x00127, 0x00126}, {0x00129, 0x00128},
    {0x0012b, 0x0012a}, {0x0012d, 0x0012c}, {0x0012f, 0x0012e}, {0x00131, 0x00049}, {0x00133, 0x00132}, {0x00135, 0x00134},
    {0x00137, 0x00136}, {0x0013a, 0x00139}, {0x0013c, 0x0013b}, {0x0013e, 0x0013d}, {0x00140, 0x0013f}, {0x00142, 0x00141},
    {0x00144, 0x00143}, {0x00146, 0x00145}, {0x00148, 0x00147}, {0x0014b, 0x0014a}, {0x0014d, 0x0014c}, {0x0014f, 0x0014e},
    {0x00151, 0x00150}, {0x00153, 0x00152}, {0x00155, 0x00154}, {0x00157, 0x00156}, {0x00159, 0x00158}, {0x0015b, 0x0015a},
    {0x0015d, 0x0015c}, {0x0015f, 0x0015e}, {0x00161, 0x00160}, {0x00163, 0x00162}, {0x00165, 0x00164}, {0x00167, 0x00166},
    {0x00169, 0x00168}, {0x0016b, 0x0016a}, {0x0016d, 0x0016c}, {0x0016f, 0x0016e}, {0x00171, 0x00170}, {0x00173, 0x00172},
    {0x00175, 0x00174}, {0x00177, 0x00176}, {0x0017a, 0x00179}, {0x0017c, 0x0017b}, {0x0017e, 0x0017d}, {0x0017f, 0x00053},
    {0x00183, 0x00182}, {0x00185, 0x00184}, {0x00188, 0x00187}, {0x0018c, 0x0018b}, {0x00192, 0x00191}, {0x00195, 0x001f6},
    {0x00199, 0x00198}, {0x0019e, 0x00220}, {0x001a1, 0x001a0}, {0x001a3, 0x001a2}, {0x001a5, 0x001a4}, {0x001a8, 0x001a7},
    {0x001ad, 0x001ac}, {0x001b0, 0x001af}, {0x001b4, 0x001b3}, {0x001b6, 0x001b5}, {0x001b9, 0x001b8}, {0x001bd, 0x001bc},
    {0x001bf, 0x001f7}, {0x001c4, 0x001c5}, {0x001c5, 0x001c5}, {0x001c6, 0x001c5}, {0x001c7, 0x001c8}, {0x001c8, 0x001c8},
    {0x001c9, 0x001c8}, {0x001ca, 0x001cb}, {0x001cb, 0x001cb}, {0x001cc, 0x001cb}, {0x001ce, 0x001cd}, {0x001d0, 0x001cf},
    {0x001d2, 0x001d1}, {0x001d4, 0x001d3}, {0x001d6, 0x001d5}, {0x001d8, 0x001d7}, {0x001da, 0x001d9}, {0x001dc, 0x001db},
    {0x001dd, 0x0018e}, {0x001df, 0x001de}, {0x001e1, 0x001e0}, {0x001e3, 0x001e2}, {0x001e5, 0x001e4}, {0x001e7, 0x001e6},
    {0x001e9, 0x001e8}, {0x001eb, 0x001ea}, {0x001ed, 0x001ec}, {0x001ef, 0x001ee}, {0x001f1, 0x001f2}, {0x001f2, 0x001f2},
    {0x001f3, 0x001f2}, {0x001f5, 0x001f4}, {0x001f9, 0x001f8}, {0x001fb, 0x001fa}, {0x001fd, 0x001fc}, {0x001ff, 0x001fe},
    {0x00201, 0x00200}, {0x00203, 0x00202}, {0x00205, 0x00204}, {0x00207, 0x00206}, {0x00209, 0x00208}, {0x0020b, 0x0020a},
    {0x0020d, 0x0020c}, {0x0020f, 0x0020e}, {0x00211, 0x00210}, {0x00213, 0x00212}, {0x00215, 0x00214}, {0x00217, 0x00216},
    {0x00219, 0x00218}, {0x0021b, 0x0021a}, {0x0021d, 0x0021c}, {0x0021f, 0x0021e}, {0x00223, 0x00222}, {0x00225, 0x00224},
    {0x00227, 0x00226}, {0x00229, 0x00228}, {0x0022b, 0x0022a}, {0x0022d, 0x0022c}, {0x0022f, 0x0022e}, {0x00231, 0x00230},
    {0x00233, 0x00232}, {0x00253, 0x00181}, {0x00254, 0x00186}, {0x00256, 0x00189}, {0x00257, 0x0018a}, {0x00259, 0x0018f},
    {0x0025b, 0x00190}, {0x00260, 0x00193}, {0x00263, 0x00194}, {0x00268, 0x00197}, {0x00269, 0x00196}, {0x0026f, 0x0019c},
    {0x00272, 0x0019d}, {0x00275, 0x0019f}, {0x00280, 0x001a6}, {0x00283, 0x001a9}, {0x00288, 0x001ae}, {0x0028a, 0x001b1},
    {0x0028b, 0x001b2}, {0x00292, 0x001b7}, {0x003ac, 0x00386}, {0x003ad, 0x00388}, {0x003ae, 0x00389}, {0x003af, 0x0038a},
    {0x003b1, 0x00391}, {0x003b2, 0x00392}, {0x003b3, 0x00393}, {0x003b4, 0x00394}, {0x003b5, 0x00395}, {0x003b6, 0x00396},
    {0x003b7, 0x00397}, {0x003b8, 0x00398}, {0x003b9, 0x00399}, {0x003ba, 0x0039a}, {0x003bb, 0x0039b}, {0x003bc, 0x0039c},
    {0x003bd, 0x0039d}, {0x003be, 0x0039e}, {0x003bf, 0x0039f}, {0x003c0, 0x003a0}, {0x003c1, 0x003a1}, {0x003c2, 0x003a3},
    {0x003c3, 0x003a3}, {0x003c4, 0x003a4}, {0x003c5, 0x003a5}, {0x003c6, 0x003a6}, {0x003c7, 0x003a7}, {0x003c8, 0x003a8},
    {0x003c9, 0x003a9}, {0x003ca, 0x003aa}, {0x003cb, 0x003ab}, {0x003cc, 0x0038c}, {0x003cd, 0x0038e}, {0x003ce, 0x0038f},
    {0x003d0, 0x00392}, {0x003d1, 0x00398}, {0x003d5, 0x003a6}, {0x003d6, 0x003a0}, {0x003d9, 0x003d8}, {0x003db, 0x003da},
    {0x003dd, 0x003dc}, {0x003df, 0x003de}, {0x003e1, 0x003e0}, {0x003e3, 0x003e2}, {0x003e5, 0x003e4}, {0x003e7, 0x003e6},
    {0x003e9, 0x003e8}, {0x003eb, 0x003ea}, {0x003ed, 0x003ec}, {0x003ef, 0x003ee}, {0x003f0, 0x0039a}, {0x003f1, 0x003a1},
    {0x003f2, 0x003f9}, {0x003f8, 0x003f7}, {0x003fb, 0x003fa}, {0x00430, 0x00410}, {0x00431, 0x00411}, {0x00432, 0x00412},
    {0x00433, 0x00413}, {0x00434, 0x00414}, {0x00435, 0x00415}, {0x00436, 0x00416}, {0x00437, 0x00417}, {0x00438, 0x00418},
    {0x00439, 0x00419}, {0x0043a, 0x0041a}, {0x0043b, 0x0041b}, {0x0043c, 0x0041c}, {0x0043d, 0x0041d}, {0x0043e, 0x0041e},
    {0x0043f, 0x0041f}, {0x00440, 0x00420}, {0x00441, 0x00421}, {0x00442, 0x00422}, {0x00443, 0x00423}, {0x00444, 0x00424},
    {0x00445, 0x00425}, {0x00446, 0x00426}, {0x00447, 0x00427}, {0x00448, 0x00428}, {0x00449, 0x00429}, {0x0044a, 0x0042a},
    {0x0044b, 0x0042b}, {0x0044c, 0x0042c}, {0x0044d, 0x0042d}, {0x0044e, 0x0042e}, {0x0044f, 0x0042f}, {0x00450, 0x00400},
    {0x00451, 0x00401}, {0x00452, 0x00402}, {0x00453, 0x00403}, {0x00454, 0x00404}, {0x00455, 0x00405}, {0x00456, 0x00406},
    {0x00457, 0x00407}, {0x00458, 0x00408}, {0x00459, 0x00409}, {0x0045a, 0x0040a}, {0x0045b, 0x0040b}, {0x0045c, 0x0040c},
    {0x0045d, 0x0040d}, {0x0045e, 0x0040e}, {0x0045f, 0x0040f}, {0x00461, 0x00460}, {0x00463, 0x00462}, {0x00465, 0x00464},
    {0x00467, 0x00466}, {0x00469, 0x00468}, {0x0046b, 0x0046a}, {0x0046d, 0x0046c}, {0x0046f, 0x0046e}, {0x00471, 0x00470},
    {0x00473, 0x00472}, {0x00475, 0x00474}, {0x00477, 0x00476}, {0x00479, 0x00478}, {0x0047b, 0x0047a}, {0x0047d, 0x0047c},
    {0x0047f, 0x0047e}, {0x00481, 0x00480}, {0x0048b, 0x0048a}, {0x0048d, 0x0048c}, {0x0048f, 0x0048e}, {0x00491, 0x00490},
    {0x00493, 0x00492}, {0x00495, 0x00494}, {0x00497, 0x00496}, {0x00499, 0x00498}, {0x0049b, 0x0049a}, {0x0049d, 0x0049c},
    {0x0049f, 0x0049e}, {0x004a1, 0x004a0}, {0x004a3, 0x004a2}, {0x004a5, 0x004a4}, {0x004a7, 0x004a6}, {0x004a9, 0x004a8},
    {0x004ab, 0x004aa}, {0x004ad, 0x004ac}, {0x004af, 0x004ae}, {0x004b1, 0x004b0}, {0x004b3, 0x004b2}, {0x004b5, 0x004b4},
    {0x004b7, 0x004b6}, {0x004b9, 0x004b8}, {0x004bb, 0x004ba}, {0x004bd, 0x004bc}, {0x004bf, 0x004be}, {0x004c2, 0x004c1},
    {0x004c4, 0x004c3}, {0x004c6, 0x004c5}, {0x004c8, 0x004c7}, {0x004ca, 0x004c9}, {0x004cc, 0x004cb}, {0x004ce, 0x004cd},
    {0x004d1, 0x004d0}, {0x004d3, 0x004d2}, {0x004d7, 0x004d6}, {0x004d9, 0x004d8}, {0x004db, 0x004da}, {0x004dd, 0x004dc},
    {0x004df, 0x004de}, {0x004e1, 0x004e0}, {0x004e3, 0x004e2}, {0x004e5, 0x004e4}, {0x004e7, 0x004e6}, {0x004e9, 0x004e8},
    {0x004eb, 0x004ea}, {0x004ed, 0x004ec}, {0x004ef, 0x004ee}, {0x004f1, 0x004f0}, {0x004f3, 0x004f2}, {0x004f5, 0x004f4},
    {0x004f9, 0x004f8}, {0x00501, 0x00500}, {0x00503, 0x00502}, {0x00505, 0x00504}, {0x00507, 0x00506}, {0x00509, 0x00508},
    {0x0050b, 0x0050a}, {0x0050d, 0x0050c}, {0x0050f, 0x0050e}, {0x00561, 0x00531}, {0x00562, 0x00532}, {0x00563, 0x00533},
    {0x00564, 0x00534}, {0x00565, 0x00535}, {0x00566, 0x00536}, {0x00567, 0x00537}, {0x00568, 0x00538}, {0x00569, 0x00539},
    {0x0056a, 0x0053a}, {0x0056b, 0x0053b}, {0x0056c, 0x0053c}, {0x0056d, 0x0053d}, {0x0056e, 0x0053e}, {0x0056f, 0x0053f},
    {0x00570, 0x00540}, {0x00571, 0x00541}, {0x00572, 0x00542}, {0x00573, 0x00543}, {0x00574, 0x00544}, {0x00575, 0x00545},
    {0x00576, 0x00546}, {0x00577, 0x00547}, {0x00578, 0x00548}, {0x00579, 0x00549}, {0x0057a, 0x0054a}, {0x0057b, 0x0054b},
    {0x0057c, 0x0054c}, {0x0057d, 0x0054d}, {0x0057e, 0x0054e}, {0x0057f, 0x0054f}, {0x00580, 0x00550}, {0x00581, 0x00551},
    {0x00582, 0x00552}, {0x00583, 0x00553}, {0x00584, 0x00554}, {0x00585, 0x00555}, {0x00586, 0x00556}, {0x01e01, 0x01e00},
    {0x01e03, 0x01e02}, {0x01e05, 0x01e04}, {0x01e07, 0x01e06}, {0x01e09, 0x01e08}, {0x01e0b, 0x01e0a}, {0x01e0d, 0x01e0c},
    {0x01e0f, 0x01e0e}, {0x01e11, 0x01e10}, {0x01e13, 0x01e12}, {0x01e15, 0x01e14}, {0x01e17, 0x01e16}, {0x01e19, 0x01e18},
    {0x01e1b, 0x01e1a}, {0x01e1d, 0x01e1c}, {0x01e1f, 0x01e1e}, {0x01e21, 0x01e20}, {0x01e23, 0x01e22}, {0x01e25, 0x01e24},
    {0x01e27, 0x01e26}, {0x01e29, 0x01e28}, {0x01e2b, 0x01e2a}, {0x01e2d, 0x01e2c}, {0x01e2f, 0x01e2e}, {0x01e31, 0x01e30},
    {0x01e33, 0x01e32}, {0x01e35, 0x01e34}, {0x01e37, 0x01e36}, {0x01e39, 0x01e38}, {0x01e3b, 0x01e3a}, {0x01e3d, 0x01e3c},
    {0x01e3f, 0x01e3e}, {0x01e41, 0x01e40}, {0x01e43, 0x01e42}, {0x01e45, 0x01e44}, {0x01e47, 0x01e46}, {0x01e49, 0x01e48},
    {0x01e4b, 0x01e4a}, {0x01e4d, 0x01e4c}, {0x01e4f, 0x01e4e}, {0x01e51, 0x01e50}, {0x01e53, 0x01e52}, {0x01e55, 0x01e54},
    {0x01e57, 0x01e56}, {0x01e59, 0x01e58}, {0x01e5b, 0x01e5a}, {0x01e5d, 0x01e5c}, {0x01e5f, 0x01e5e}, {0x01e61, 0x01e60},
    {0x01e63, 0x01e62}, {0x01e65, 0x01e64}, {0x01e67, 0x01e66}, {0x01e69, 0x01e68}, {0x01e6b, 0x01e6a}, {0x01e6d, 0x01e6c},
    {0x01e6f, 0x01e6e}, {0x01e71, 0x01e70}, {0x01e73, 0x01e72}, {0x01e75, 0x01e74}, {0x01e77, 0x01e76}, {0x01e79, 0x01e78},
    {0x01e7b, 0x01e7a}, {0x01e7d, 0x01e7c}, {0x01e7f, 0x01e7e}, {0x01e81, 0x01e80}, {0x01e83, 0x01e82}, {0x01e85, 0x01e84},
    {0x01e87, 0x01e86}, {0x01e89, 0x01e88}, {0x01e8b, 0x01e8a}, {0x01e8d, 0x01e8c}, {0x01e8f, 0x01e8e}, {0x01e91, 0x01e90},
    {0x01e93, 0x01e92}, {0x01e95, 0x01e94}, {0x01e9b, 0x01e60}, {0x01ea1, 0x01ea0}, {0x01ea3, 0x01ea2}, {0x01ea5, 0x01ea4},
    {0x01ea7, 0x01ea6}, {0x01ea9, 0x01ea8}, {0x01eab, 0x01eaa}, {0x01ead, 0x01eac}, {0x01eaf, 0x01eae}, {0x01eb1, 0x01eb0},
    {0x01eb3, 0x01eb2}, {0x01eb5, 0x01eb4}, {0x01eb7, 0x01eb6}, {0x01eb9, 0x01eb8}, {0x01ebb, 0x01eba}, {0x01ebd, 0x01ebc},
    {0x01ebf, 0x01ebe}, {0x01ec1, 0x01ec0}, {0x01ec3, 0x01ec2}, {0x01ec5, 0x01ec4}, {0x01ec7, 0x01ec6}, {0x01ec9, 0x01ec8},
    {0x01ecb, 0x01eca}, {0x01ecd, 0x01ecc}, {0x01ecf, 0x01ece}, {0x01ed1, 0x01ed0}, {0x01ed3, 0x01ed2}, {0x01ed5, 0x01ed4},
    {0x01ed7, 0x01ed6}, {0x01ed9, 0x01ed8}, {0x01edb, 0x01eda}, {0x01edd, 0x01edc}, {0x01edf, 0x01ede}, {0x01ee1, 0x01ee0},
    {0x01ee3, 0x01ee2}, {0x01ee5, 0x01ee4}, {0x01ee7, 0x01ee6}, {0x01ee9, 0x01ee8}, {0x01eeb, 0x01eea}, {0x01eed, 0x01eec},
    {0x01eef, 0x01eee}, {0x01ef1, 0x01ef0}, {0x01ef3, 0x01ef2}, {0x01ef5, 0x01ef4}, {0x01ef7, 0x01ef6}, {0x01ef9, 0x01ef8},
    {0x01f00, 0x01f08}, {0x01f01, 0x01f09}, {0x01f02, 0x01f0a}, {0x01f03, 0x01f0b}, {0x01f04, 0x01f0c}, {0x01f05, 0x01f0d},
    {0x01f06, 0x01f0e}, {0x01f07, 0x01f0f}, {0x01f10, 0x01f18}, {0x01f11, 0x01f19}, {0x01f12, 0x01f1a}, {0x01f13, 0x01f1b},
    {0x01f14, 0x01f1c}, {0x01f15, 0x01f1d}, {0x01f20, 0x01f28}, {0x01f21, 0x01f29}, {0x01f22, 0x01f2a}, {0x01f23, 0x01f2b},
    {0x01f24, 0x01f2c}, {0x01f25, 0x01f2d}, {0x01f26, 0x01f2e}, {0x01f27, 0x01f2f}, {0x01f30, 0x01f38}, {0x01f31, 0x01f39},
    {0x01f32, 0x01f3a}, {0x01f33, 0x01f3b}, {0x01f34, 0x01f3c}, {0x01f35, 0x01f3d}, {0x01f36, 0x01f3e}, {0x01f37, 0x01f3f},
    {0x01f40, 0x01f48}, {0x01f41, 0x01f49}, {0x01f42, 0x01f4a}, {0x01f43, 0x01f4b}, {0x01f44, 0x01f4c}, {0x01f45, 0x01f4d},
    {0x01f51, 0x01f59}, {0x01f53, 0x01f5b}, {0x01f55, 0x01f5d}, {0x01f57, 0x01f5f}, {0x01f60, 0x01f68}, {0x01f61, 0x01f69},
    {0x01f62, 0x01f6a}, {0x01f63, 0x01f6b}, {0x01f64, 0x01f6c}, {0x01f65, 0x01f6d}, {0x01f66, 0x01f6e}, {0x01f67, 0x01f6f},
    {0x01f70, 0x01fba}, {0x01f71, 0x01fbb}, {0x01f72, 0x01fc8}, {0x01f73, 0x01fc9}, {0x01f74, 0x01fca}, {0x01f75, 0x01fcb},
    {0x01f76, 0x01fda}, {0x01f77, 0x01fdb}, {0x01f78, 0x01ff8}, {0x01f79, 0x01ff9}, {0x01f7a, 0x01fea}, {0x01f7b, 0x01feb},
    {0x01f7c, 0x01ffa}, {0x01f7d, 0x01ffb}, {0x01f80, 0x01f88}, {0x01f81, 0x01f89}, {0x01f82, 0x01f8a}, {0x01f83, 0x01f8b},
    {0x01f84, 0x01f8c}, {0x01f85, 0x01f8d}, {0x01f86, 0x01f8e}, {0x01f87, 0x01f8f}, {0x01f90, 0x01f98}, {0x01f91, 0x01f99},
    {0x01f92, 0x01f9a}, {0x01f93, 0x01f9b}, {0x01f94, 0x01f9c}, {0x01f95, 0x01f9d}, {0x01f96, 0x01f9e}, {0x01f97, 0x01f9f},
    {0x01fa0, 0x01fa8}, {0x01fa1, 0x01fa9}, {0x01fa2, 0x01faa}, {0x01fa3, 0x01fab}, {0x01fa4, 0x01fac}, {0x01fa5, 0x01fad},
    {0x01fa6, 0x01fae}, {0x01fa7, 0x01faf}, {0x01fb0, 0x01fb8}, {0x01fb1, 0x01fb9}, {0x01fb3, 0x01fbc}, {0x01fc3, 0x01fcc},
    {0x01fd0, 0x01fd8}, {0x01fd1, 0x01fd9}, {0x01fe0, 0x01fe8}, {0x01fe1, 0x01fe9}, {0x01fe5, 0x01fec}, {0x01ff3, 0x01ffc},
    {0x024d0, 0x024b6}, {0x024d1, 0x024b7}, {0x024d2, 0x024b8}, {0x024d3, 0x024b9}, {0x024d4, 0x024ba}, {0x024d5, 0x024bb},
    {0x024d6, 0x024bc}, {0x024d7, 0x024bd}, {0x024d8, 0x024be}, {0x024d9, 0x024bf}, {0x024da, 0x024c0}, {0x024db, 0x024c1},
    {0x024dc, 0x024c2}, {0x024dd, 0x024c3}, {0x024de, 0x024c4}, {0x024df, 0x024c5}, {0x024e0, 0x024c6}, {0x024e1, 0x024c7},
    {0x024e2, 0x024c8}, {0x024e3, 0x024c9}, {0x024e4, 0x024ca}, {0x024e5, 0x024cb}, {0x024e6, 0x024cc}, {0x024e7, 0x024cd},
    {0x024e8, 0x024ce}, {0x024e9, 0x024cf}, {0x0ff41, 0x0ff21}, {0x0ff42, 0x0ff22}, {0x0ff43, 0x0ff23}, {0x0ff44, 0x0ff24},
    {0x0ff45, 0x0ff25}, {0x0ff46, 0x0ff26}, {0x0ff47, 0x0ff27}, {0x0ff48, 0x0ff28}, {0x0ff49, 0x0ff29}, {0x0ff4a, 0x0ff2a},
    {0x0ff4b, 0x0ff2b}, {0x0ff4c, 0x0ff2c}, {0x0ff4d, 0x0ff2d}, {0x0ff4e, 0x0ff2e}, {0x0ff4f, 0x0ff2f}, {0x0ff50, 0x0ff30},
    {0x0ff51, 0x0ff31}, {0x0ff52, 0x0ff32}, {0x0ff53, 0x0ff33}, {0x0ff54, 0x0ff34}, {0x0ff55, 0x0ff35}, {0x0ff56, 0x0ff36},
    {0x0ff57, 0x0ff37}, {0x0ff58, 0x0ff38}, {0x0ff59, 0x0ff39}, {0x0ff5a, 0x0ff3a}, {0x10428, 0x10400}, {0x10429, 0x10401},
    {0x1042a, 0x10402}, {0x1042b, 0x10403}, {0x1042c, 0x10404}, {0x1042d, 0x10405}, {0x1042e, 0x10406}, {0x1042f, 0x10407},
    {0x10430, 0x10408}, {0x10431, 0x10409}, {0x10432, 0x1040a}, {0x10433, 0x1040b}, {0x10434, 0x1040c}, {0x10435, 0x1040d},
    {0x10436, 0x1040e}, {0x10437, 0x1040f}, {0x10438, 0x10410}, {0x10439, 0x10411}, {0x1043a, 0x10412}, {0x1043b, 0x10413},
    {0x1043c, 0x10414}, {0x1043d, 0x10415}, {0x1043e, 0x10416}, {0x1043f, 0x10417}, {0x10440, 0x10418}, {0x10441, 0x10419},
    {0x10442, 0x1041a}, {0x10443, 0x1041b}, {0x10444, 0x1041c}, {0x10445, 0x1041d}, {0x10446, 0x1041e}, {0x10447, 0x1041f},
    {0x10448, 0x10420}, {0x10449, 0x10421}, {0x1044a, 0x10422}, {0x1044b, 0x10423}, {0x1044c, 0x10424}, {0x1044d, 0x10425},
    {0x1044e, 0x10426}, {0x1044f, 0x10427},
};

int UTF8::ToUpper(int val)
{
    if (val < 0x7f)
        return ::toupper(val);
    int top = sizeof(upper) / sizeof(upper[0]);
    int bottom = -1;
    while (top - bottom > 1)
    {
        int mid = (top + bottom) / 2;
        if (val < upper[mid][0])
        {
            top = mid;
        }
        else
        {
            bottom = mid;
        }
    }
    if (bottom < 0)
        bottom = 0;
    if (upper[bottom][0] != val)
        return val;
    return upper[bottom][1];
}
